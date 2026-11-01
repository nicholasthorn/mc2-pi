#!/bin/bash
# CI gate: was this push documentation only? Prints "docs_only=true" when
# every file changed between BEFORE and AFTER is documentation, otherwise
# "docs_only=false". The workflows (ci.yml, smoke.yml) skip their jobs on true.
#
# false (run everything) when BEFORE is empty, all zeros (new branch), missing,
# or not an ancestor of AFTER (force push), or when nothing changed (re-run).
# With WORKFLOW (a file name in .github/workflows), also false unless that
# workflow succeeded on BEFORE, so a docs commit pushed after a cancelled or
# failed run never covers it up. That check calls the GitHub API through gh
# (GH_TOKEN, GITHUB_REPOSITORY); without WORKFLOW the script is git only.
#
# usage: scripts/ci_docs_only.sh BEFORE AFTER [WORKFLOW]

before="$1"
after="$2"
workflow="$3"

# Markdown anywhere, planning/, agent rules and skills, alariq's text notes.
DOCS='(\.md$|^planning/|^\.claude/|^docs$|^devdoc\.txt$)'

answer() { echo "docs_only=$1"; exit 0; }

case "$before" in ""|*[!0]*) ;; *) answer false ;; esac
[ -n "$before" ] && [ -n "$after" ] || answer false
git cat-file -e "$before^{commit}" 2>/dev/null || answer false
git merge-base --is-ancestor "$before" "$after" 2>/dev/null || answer false

changed=$(git diff --name-only "$before" "$after") || answer false
[ -n "$changed" ] || answer false
if echo "$changed" | grep -qvE "$DOCS"; then
    answer false
fi

if [ -n "$workflow" ]; then
    ok=$(gh api "repos/$GITHUB_REPOSITORY/actions/workflows/$workflow/runs?head_sha=$before&status=success" \
         --jq .total_count 2>/dev/null) || ok=0
    [ "${ok:-0}" -gt 0 ] 2>/dev/null || answer false
fi
answer true
