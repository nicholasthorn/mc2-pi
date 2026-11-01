#!/bin/bash
# CI gate: does this push need the build, tests and smoke again? Prints
# "skip=true" or "skip=false"; the workflows (ci.yml, smoke.yml) skip their
# jobs on true. Skips when, apart from docs (*.md, planning/, .claude/, docs,
# devdoc.txt), BEFORE and AFTER carry the same code:
#
#   - every file changed between them is docs; or
#   - with --base REF (a PR's base branch): the PR's own changes, base to
#     branch, are the same patch at both, so the push only merged REF in
#     (plus docs, e.g. a PROGRESS.md conflict). The combination with the code
#     that came in from REF is then first tested by REF's own run after the
#     merge, which is the accepted risk.
#
# false (run everything) when BEFORE is empty, all zeros (new branch), missing,
# or not an ancestor of AFTER (force push), or when nothing changed (re-run).
# With --workflow FILE (in .github/workflows), also false unless that workflow
# succeeded on BEFORE, so a skip never covers up a cancelled or failed run.
# That check calls the GitHub API through gh (GH_TOKEN, GITHUB_REPOSITORY);
# without it the script is git only.
#
# usage: scripts/ci_skip.sh BEFORE AFTER [--base REF] [--workflow FILE]

before="$1"
after="$2"
shift 2
base=""
workflow=""
while [ $# -gt 0 ]; do
    case "$1" in
        --base)     base="$2"; shift 2 ;;
        --workflow) workflow="$2"; shift 2 ;;
        *) echo "ci_skip: unknown option $1" >&2; exit 2 ;;
    esac
done

NOT_DOCS=(-- . ':(exclude)*.md' ':(exclude)planning' ':(exclude).claude'
          ':(exclude)docs' ':(exclude)devdoc.txt')

answer() { echo "skip=$1"; exit 0; }

# The PR's own non-docs changes at commit $1, as a patch id (empty if none).
own_patch() {
    local mb
    mb=$(git merge-base "$base" "$1") || return 1
    git diff "$mb" "$1" "${NOT_DOCS[@]}" | git patch-id --stable | cut -d' ' -f1
}

case "$before" in ""|*[!0]*) ;; *) answer false ;; esac
[ -n "$before" ] && [ -n "$after" ] || answer false
git cat-file -e "$before^{commit}" 2>/dev/null || answer false
git merge-base --is-ancestor "$before" "$after" 2>/dev/null || answer false
[ -n "$(git diff --name-only "$before" "$after")" ] || answer false

if [ -n "$(git diff --name-only "$before" "$after" "${NOT_DOCS[@]}")" ]; then
    [ -n "$base" ] || answer false
    git cat-file -e "$base^{commit}" 2>/dev/null || answer false
    p_before=$(own_patch "$before") || answer false
    p_after=$(own_patch "$after") || answer false
    [ "$p_before" = "$p_after" ] || answer false
fi

if [ -n "$workflow" ]; then
    ok=$(gh api "repos/$GITHUB_REPOSITORY/actions/workflows/$workflow/runs?head_sha=$before&status=success" \
         --jq .total_count 2>/dev/null) || ok=0
    [ "${ok:-0}" -gt 0 ] 2>/dev/null || answer false
fi
answer true
