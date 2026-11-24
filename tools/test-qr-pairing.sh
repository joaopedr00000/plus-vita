#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT
node tools/test-qr-pairing.mjs > "$temp/fixture"
cc -std=c11 -Wall -Wextra -Werror vita-client/src/platform/qr_pair_crypto.c vita-client/tests/test_qr_pair_crypto.c -lmbedcrypto -o "$temp/test"
"$temp/test" < "$temp/fixture"
