# Build Beta 1 (experimental)

This is a separate beta profile. No hardware acceptance is implied by a successful build.

```sh
git clone --branch beta1 --recurse-submodules https://github.com/joaopedr00000/plus-vita.git
cd plus-vita
cc -std=c11 -Wall -Wextra -Werror vita-client/tests/test_cloud_experimental.c -o /tmp/plus-vita-cloud-test
/tmp/plus-vita-cloud-test
cmake -S vita-client -B build/beta1 -DCMAKE_BUILD_TYPE=Release \
  -DPLUS_VITA_RELEASE_CANDIDATE=ON -DPLUS_VITA_RELEASE_VERSION=v002 \
  -DPLUS_VITA_PS5_EXPERIMENTAL=ON -DPLUS_VITA_BETA1=ON
cmake --build build/beta1 --parallel
python3 tools/publication_gate.py --version beta1 --vpk build/beta1/plus_vita_smoke.vpk
python3 tools/test-publication-gate.py build/beta1/plus_vita_smoke.vpk --version beta1
```

VitaSDK and its transport packages are required; see vita-client/README.md. Do not distribute a VPK if the build or privacy checks fail. Install the beta only after backing up shared app data. The separate application ID avoids replacing the original executable but does not isolate existing account and log paths. Never include account JSON in a release.
