#!/bin/sh
# Build the TLS backend explicitly, without replacing the SDK's system curl.
# Version and hash match the public VitaSDK curl-mbedtls 8.23.0 recipe.
set -eu
: "${VITASDK:?VITASDK is required}"
prefix="${1:?Absolute install directory required}"
case "$prefix" in /*) ;; *) echo "Use an absolute installation path" >&2; exit 1;; esac
version=8.23.0
sha=8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fL --retry 3 --proto '=https' "https://github.com/curl/curl/releases/download/curl-8_23_0/curl-$version.tar.xz" -o "$tmp/curl.tar.xz"
printf '%s  %s\n' "$sha" "$tmp/curl.tar.xz" | sha256sum -c -
tar -xJf "$tmp/curl.tar.xz" -C "$tmp"
cmake -S "$tmp/curl-$version" -B "$tmp/build" \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_CURL_EXE=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF \
  -DENABLE_IPV6=OFF -DCURL_DISABLE_SOCKETPAIR=ON -DHAVE_FCNTL_O_NONBLOCK=OFF \
  -DENABLE_THREADED_RESOLVER=OFF -DBUILD_LIBCURL_DOCS=OFF -DBUILD_MISC_DOCS=OFF \
  -DENABLE_CURL_MANUAL=OFF -DCURL_CA_BUNDLE=none -DCURL_CA_PATH=none \
  -DCURL_USE_LIBPSL=OFF -DCURL_USE_OPENSSL=OFF -DCURL_USE_MBEDTLS=ON \
  -DHAVE_PIPE2=0 -DCMAKE_DISABLE_FIND_PACKAGE_Threads=ON \
  -DUSE_NGHTTP2=OFF -DCURL_USE_LIBSSH2=OFF -DCURL_USE_LIBSSH=OFF
cmake --build "$tmp/build" --parallel 2
cmake --install "$tmp/build"
test -s "$prefix/lib/libcurl.a"
test -s "$prefix/include/curl/curl.h"
# Inspect target archive symbols, not host curl's version.
arm-vita-eabi-nm "$prefix/lib/libcurl.a" > "$tmp/symbols"
grep -q 'mbedtls_ssl_handshake' "$tmp/symbols"
if grep -Eq '[[:space:]]U[[:space:]]+SSL_connect$' "$tmp/symbols"; then
  echo "Unexpected OpenSSL transport" >&2; exit 1
fi
echo "Verified target curl archive uses Mbed TLS"
