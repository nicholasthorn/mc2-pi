#!/bin/bash
# Runs the libFuzzer targets (tests/unit/fuzz, built with -DMC2_FUZZ=ON) side
# by side for a fixed time each, keeping what they learn in a corpus directory
# from run to run. Used by .github/workflows/fuzz.yml; works the same locally.
#
# Each target fuzzes from its corpus directory (new inputs are written there)
# plus the seeds in tests/unit/fuzz/corpus/<target>, which it only reads. A
# crash, sanitizer report, hang or out-of-memory stops that target and leaves
# the input in <out-dir>/<target>/ (crash-*, timeout-*, oom-*); its log is
# <out-dir>/<target>.log. With --minimize, each corpus is then cut down to the
# inputs that still add coverage, so it does not grow without end.
#
# Prints a summary (also to $GITHUB_STEP_SUMMARY when set) and exits 1 if any
# target found something, 2 for bad arguments.
#
# usage: scripts/fuzz.sh --build-dir DIR [--seconds N] [--corpus-dir DIR]
#                        [--out-dir DIR] [--minimize] [target...]
#   --corpus-dir, --out-dir default to fuzz-corpus/ and fuzz-out/ in the build dir
#   targets: fit csv tga abl (default: all four)

set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir=""
seconds=300
corpus_dir=""
out_dir=""
minimize=0
targets=()

while [ $# -gt 0 ]; do
	case "$1" in
		--build-dir)  build_dir="$2"; shift 2 ;;
		--seconds)    seconds="$2"; shift 2 ;;
		--corpus-dir) corpus_dir="$2"; shift 2 ;;
		--out-dir)    out_dir="$2"; shift 2 ;;
		--minimize)   minimize=1; shift ;;
		-h|--help)    sed -n '2,20p' "$0"; exit 0 ;;
		-*)           echo "fuzz.sh: unknown option $1" >&2; exit 2 ;;
		*)            targets+=("$1"); shift ;;
	esac
done
[ ${#targets[@]} -eq 0 ] && targets=(fit csv tga abl)
if [ -z "$build_dir" ]; then
	echo "fuzz.sh: --build-dir is required" >&2
	exit 2
fi
bin_dir="$build_dir/out/tests/unit"
# by default under the build directory, out of the source tree
[ -z "$corpus_dir" ] && corpus_dir="$build_dir/fuzz-corpus"
[ -z "$out_dir" ] && out_dir="$build_dir/fuzz-out"
seeds_dir="$root/tests/unit/fuzz"
for t in "${targets[@]}"; do
	if [ ! -x "$bin_dir/mc2_fuzz_$t" ]; then
		echo "fuzz.sh: no $bin_dir/mc2_fuzz_$t (configure with -DMC2_FUZZ=ON and build it)" >&2
		exit 2
	fi
done

mkdir -p "$corpus_dir" "$out_dir"
corpus_dir="$(cd "$corpus_dir" && pwd)"
out_dir="$(cd "$out_dir" && pwd)"

# UBSan reports and carries on by default; a finding must stop the target.
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"

run_target() {
	local t="$1"
	mkdir -p "$corpus_dir/$t" "$out_dir/$t"
	rm -f "$out_dir/$t"/crash-* "$out_dir/$t"/timeout-* "$out_dir/$t"/oom-* "$out_dir/$t"/leak-*
	# Leaks are not what these hunt: a rejected input (STOP, ABL syntax error)
	# leaves what it allocated, and the harness turns leak checks off for it.
	"$bin_dir/mc2_fuzz_$t" "$corpus_dir/$t" "$seeds_dir/corpus/$t" \
		-dict="$seeds_dir/$t.dict" -max_len=8192 -max_total_time="$seconds" \
		-timeout=10 -rss_limit_mb=2560 -detect_leaks=0 -print_final_stats=1 \
		-artifact_prefix="$out_dir/$t/" > "$out_dir/$t.log" 2>&1
	echo $? > "$out_dir/$t.rc"
}

echo "fuzz.sh: ${targets[*]} for ${seconds}s each, corpus $corpus_dir"
for t in "${targets[@]}"; do
	run_target "$t" &
done
wait

if [ "$minimize" = 1 ]; then
	for t in "${targets[@]}"; do
		# a target that crashed keeps its corpus as it was: the merge would
		# stop at the same input
		[ "$(cat "$out_dir/$t.rc")" = 0 ] || continue
		rm -rf "$corpus_dir/$t.min"
		mkdir -p "$corpus_dir/$t.min"
		if "$bin_dir/mc2_fuzz_$t" -merge=1 -max_len=8192 -timeout=10 -rss_limit_mb=2560 -detect_leaks=0 \
				"$corpus_dir/$t.min" "$corpus_dir/$t" > "$out_dir/$t.merge.log" 2>&1; then
			rm -rf "$corpus_dir/$t"
			mv "$corpus_dir/$t.min" "$corpus_dir/$t"
		else
			rm -rf "$corpus_dir/$t.min"
		fi
	done
fi

failed=0
summary="| Target | Result | Runs | Corpus | First report |
|---|---|---|---|---|
"
for t in "${targets[@]}"; do
	rc="$(cat "$out_dir/$t.rc")"
	runs="$(grep -o 'stat::number_of_executed_units: [0-9]*' "$out_dir/$t.log" | grep -o '[0-9]*$' | tail -1)"
	files="$(find "$corpus_dir/$t" -type f | wc -l)"
	result="clean"
	report=""
	if [ "$rc" != 0 ]; then
		failed=1
		result="**found something** (exit $rc)"
		# what went wrong, and the input it left: the sanitizer's SUMMARY line
		# names the kind and the source line; a UBSan finding has its own line
		report="$(grep -m1 -E '^SUMMARY:' "$out_dir/$t.log")"
		[ -z "$report" ] && report="$(grep -m1 -E 'runtime error:|ERROR: ' "$out_dir/$t.log")"
		# no sanitizer line: it did not get going (a missing corpus, say)
		[ -z "$report" ] && report="$(grep -v '^$' "$out_dir/$t.log" | tail -1)"
		report="$(echo "$report" | sed "s|$root/||g; s/|/\\\\|/g" | cut -c1-200)"
		input="$(ls "$out_dir/$t" 2>/dev/null | head -1)"
		[ -n "$input" ] && report="$report (input: \`$input\`)"
	fi
	summary="$summary| $t | $result | ${runs:-?} | $files | $report |
"
done

echo
echo "$summary"
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
	{
		echo "### Fuzzing, ${seconds}s per target"
		echo
		echo "$summary"
		[ "$failed" = 1 ] && echo "Inputs and logs are in the run's \`fuzz-findings\` artifact. Reproduce with \`mc2_fuzz_<target> <input>\` (tests/unit/fuzz/README.md)."
	} >> "$GITHUB_STEP_SUMMARY"
fi
exit "$failed"
