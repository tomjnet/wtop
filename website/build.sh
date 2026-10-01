#!/usr/bin/env bash
# Builds a minified copy of the website into website/build/ (git-ignored).
# The Pages workflow runs this and publishes website/build/.
#
#   bash website/build.sh          # then open website/build/index.html
#
# Needs Node.js (npx). Tool versions are pinned so builds are reproducible.
set -euo pipefail

ESBUILD=esbuild@0.28.2
HTML_MINIFIER=html-minifier-terser@7.2.0

src=$(cd "$(dirname "$0")" && pwd)
out="$src/build"

rm -rf "$out"
mkdir -p "$out"
cp -r "$src/assets" "$out/"

# JS: strip whitespace/comments and mangle local names. CSS: minify.
npx --yes "$ESBUILD" "$src/main.js" --minify --target=es2017 \
  --legal-comments=none --outfile="$out/main.js" --log-level=warning
npx --yes "$ESBUILD" "$src/style.css" --minify \
  --outfile="$out/style.css" --log-level=warning

npx --yes "$HTML_MINIFIER" "$src/index.html" -o "$out/index.html" \
  --collapse-whitespace --conservative-collapse \
  --remove-comments \
  --remove-redundant-attributes \
  --remove-script-type-attributes \
  --remove-style-link-type-attributes \
  --use-short-doctype \
  --minify-css true \
  --minify-js true

# Summary: source vs. built size of the text files.
for f in index.html main.js style.css; do
  printf '%-11s %6d -> %6d bytes\n' "$f" "$(wc -c < "$src/$f")" "$(wc -c < "$out/$f")"
done
