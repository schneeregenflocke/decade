#!/usr/bin/env bash
# Refreshes this directory from the upstream it mirrors, Björn Ottosson's misc
# tree: https://github.com/bottosson/bottosson.github.io/tree/master/misc
#
# It holds ok_color.h, the licence texts that cover it, and the reference colour
# picker the header was written for. Upstream ships no releases and no tags, so
# the mirror travels in the repository and this script replaces it.
#
# Encodings differ between the files — ok_color.h arrives as ISO-8859-1 while the
# licence texts are already UTF-8 — so each file gets transcoded unless it
# decodes as UTF-8 on its own.
set -euo pipefail

readonly repository="bottosson/bottosson.github.io"
readonly branch="master"
readonly upstream_directory="misc"

target_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly target_directory

readonly tree_url="https://api.github.com/repos/${repository}/git/trees/${branch}?recursive=1"

mapfile -t upstream_paths < <(
	curl -fsSL "${tree_url}" |
		jq -r --arg directory "${upstream_directory}/" '
			.tree[]
			| select(.type == "blob")
			| select(.path | startswith($directory))
			| .path
		'
)

if [[ ${#upstream_paths[@]} -eq 0 ]]; then
	echo "no files found under ${upstream_directory}/ — did the upstream layout change?" >&2
	exit 1
fi

for upstream_path in "${upstream_paths[@]}"; do
	relative_path="${upstream_path#"${upstream_directory}/"}"
	target_path="${target_directory}/${relative_path}"

	mkdir -p "$(dirname "${target_path}")"
	curl -fsSL "https://raw.githubusercontent.com/${repository}/${branch}/${upstream_path}" \
		-o "${target_path}"

	if iconv -f UTF-8 -t UTF-8 "${target_path}" >/dev/null 2>&1; then
		echo "fetched     ${relative_path}"
	else
		iconv -f ISO-8859-1 -t UTF-8 "${target_path}" -o "${target_path}.utf8"
		mv "${target_path}.utf8" "${target_path}"
		echo "transcoded  ${relative_path}"
	fi
done

echo "${#upstream_paths[@]} files mirrored from ${repository}/${upstream_directory}"
