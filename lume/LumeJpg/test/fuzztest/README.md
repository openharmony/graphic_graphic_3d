# LumeJpg image loader fuzz test

libFuzzer harness for `JPGPlugin::ImageLoaderJPG` (i.e. `src/jpg/image_loader_jpg.cpp`).
Targets every public entry point of the loader: `Load(array_view, flags)`,
`Load(IFile&, flags)`, `LoadAnimatedImage(...)`, `CanLoad(...)`, plus the size /
channel-count guards above libjpeg-turbo.

Builds three ways, sharing the same harness source:

- **OHOS GN** (`BUILD.gn` + `project.xml`) — production target. Built and run
  via the same `ohos_fuzztest` template Lume3D uses.
- **Standalone CMake** (`CMakeLists.txt`) — runs under any Clang toolchain on
  Linux or Windows without the OHOS build pipeline. Useful for fast local
  iteration, CI smoke runs and coverage measurement.

---

## Quick start — Linux

```sh
sudo apt install -y clang lld cmake ninja-build build-essential nasm

cd LumeJpg
git submodule update --init --recursive

cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_JPG_BUILD_ENGINE=OFF \
  -DJPG_FUZZ_TESTS_ENABLED=ON

cmake --build build-fuzz --target jpg_image_loader_fuzzer

build-fuzz/test/fuzztest/jpg_image_loader_fuzzer \
  -dict=jpg.dict -max_total_time=300 -print_final_stats=1 \
  build-fuzz/test/fuzztest/corpus/
```

That's it — no override flags, no patches. Submodules at `LumeBase/` and
`LumeEngine/` are picked up automatically by the existing `find_package`
machinery. The build defaults to **`-fsanitize=fuzzer,address`** on Linux so
heap-overflow / UAF bugs are caught, with `-fsanitize-recover=address` so one
finding doesn't end the session.

If you're only after raw crash-finding throughput and don't need ASan, drop
it with `-DFUZZ_SANITIZERS=fuzzer`. ASan adds ~2× CPU and ~3× memory cost.

## Quick start — Windows (Clang + Ninja)

Requires LLVM (`clang.exe`, `clang++.exe`, `llvm-cov.exe`, `llvm-profdata.exe`)
on `PATH`, plus CMake and Ninja.

```powershell
cd D:\GitCode\LumeJpg
git submodule update --init --recursive

cmake -S . -B build-fuzz -G Ninja `
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" `
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DCORE_JPG_BUILD_ENGINE=OFF `
  -DJPG_FUZZ_TESTS_ENABLED=ON
```

`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` is needed because Clang's prebuilt
libFuzzer runtime on Windows is `/MT_StaticRelease`; without it, `lld-link`
reports a `RuntimeLibrary` mismatch.

If the submodule paths haven't been initialised, add
`-DBASE_ROOT_DIRECTORY=<path-to-LumeBase>` and
`-DCORE_ROOT_DIRECTORY=<path-to-LumeEngine>`.

```powershell
cmake --build build-fuzz --target jpg_image_loader_fuzzer

.\build-fuzz\test\fuzztest\jpg_image_loader_fuzzer.exe `
  -dict=jpg.dict -max_total_time=300 -print_final_stats=1 `
  .\build-fuzz\test\fuzztest\corpus\
```

Windows defaults to **`-fsanitize=fuzzer`** (no ASan) because Clang's prebuilt
ASan runtime is a `/MT` static lib that's tricky to mix with the rest of the
graph here. Opt in with `-DFUZZ_SANITIZERS=fuzzer,address` if you've confirmed
every link input is `/MT`.

### Known Windows-only hiccup

libjpeg-turbo's NSIS-packaging script looks for `win/${INST_ID}/projectTargets-release.cmake.in`,
where `INST_ID` is only set under MSVC/MinGW. With plain Clang on Windows
`INST_ID` is empty and the configure fails. If you hit it, run once:

```powershell
Copy-Item `
  build-fuzz\unpacked\libjpeg-turbo-3.0.2_*\win\gcc\projectTargets-release.cmake.in `
  build-fuzz\unpacked\libjpeg-turbo-3.0.2_*\win\projectTargets-release.cmake.in
```

Then rebuild. (Upstream issue, not specific to this harness.)

## OHOS GN build (production target)

```sh
gn gen out/jpg_fuzz --args='is_debug=false'
ninja -C out/jpg_fuzz \
  foundation/graphic/graphic_3d/lume/LumeJpg/test/fuzztest:fuzztest
```

The `ohos_fuzztest("JpgImageLoaderFuzzTest")` target uses the same
`project.xml` config (`max_len=131072`, `max_total_time=300`, `rss_limit_mb=4096`)
as Lume3D's gltf fuzzer.

---

## Running

libFuzzer flags worth knowing:

| Flag | Effect |
|---|---|
| `-max_total_time=N` | Stop after N seconds. |
| `-runs=N` | Stop after N executions. `0` replays the corpus and exits. |
| `-max_len=N` | Cap mutated input size to N bytes (default discovered from corpus). |
| `-rss_limit_mb=N` | Kill on OOM. Default 2048. |
| `-timeout=N` | Kill on hang. Default 1200 s. |
| `-jobs=N -workers=K` | Run N total fuzzing jobs, K in parallel. |
| `-artifact_prefix=DIR/` | Where to drop `crash-*`, `timeout-*`, `oom-*` artifacts. |
| `-print_final_stats=1` | Summary at exit. |
| `-help=1` | Full flag list. |
| `-dict=jpg.dict` | Use the JPEG marker dictionary shipped alongside the binary. **Strongly recommended** — speeds up format discovery by 10×+. |

When ASan is enabled (the Linux default), LeakSanitizer triggers at
process exit and will end the fuzz session on the first leak it finds.
If you'd rather collect crashes/OOB-reads now and triage leaks later,
set `ASAN_OPTIONS=detect_leaks=0` to suppress leak reports for the
session. Crashes and out-of-bounds accesses are still reported in
real time regardless.

Replay a saved finding:

```sh
./jpg_image_loader_fuzzer path/to/crash-<sha1>
```

Minimise the corpus (useful before committing new seeds):

```sh
./jpg_image_loader_fuzzer -merge=1 minimised_corpus/ build-fuzz/test/fuzztest/corpus/
```

---

## Coverage measurement

Source-based coverage is opt-in to keep production builds untouched:

```sh
cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_JPG_BUILD_ENGINE=OFF \
  -DJPG_FUZZ_TESTS_ENABLED=ON \
  -DJPG_FUZZ_COVERAGE=ON
cmake --build build-fuzz --target jpg_image_loader_fuzzer
```

Run a session emitting profile data:

```sh
cd build-fuzz/test/fuzztest
rm -rf prof merged.profdata cov_html
mkdir prof
LLVM_PROFILE_FILE="prof/cov-%p-%m.profraw" \
  ./jpg_image_loader_fuzzer -max_total_time=180 corpus/

llvm-profdata merge -sparse prof/*.profraw -o merged.profdata
llvm-cov report ./jpg_image_loader_fuzzer -instr-profile=merged.profdata
llvm-cov show ./jpg_image_loader_fuzzer -instr-profile=merged.profdata \
  -show-branches=count -show-line-counts-or-regions \
  -format=html -output-dir=cov_html \
  ../../../src/jpg/image_loader_jpg.cpp
```

Open `cov_html/index.html` in a browser for the annotated source view.

Reference figures from a 2-minute Release+coverage+dict session against the
in-tree seed corpus (`image_loader_jpg.cpp` only):

| Metric | Result |
|---|---|
| Functions | 100.00 % |
| Lines | 90.86 % |
| Regions | 92.72 % |
| Branches | 89.06 % |

The remaining lines are unreachable defensive code paths (16-bpc premultiply
that can't fire because `JSAMPLE` is `uint8_t`, `ErrorExit(nullptr)` forbidden
by the libjpeg API contract, `FillRowPointers` OOB guards whose math precludes
the condition, `Loading image failed` which only triggers on a raw `new`
returning null, plus a couple of libjpeg "non-OK without longjmp" branches
that the real decoder never produces). This is the reachable ceiling through
the loader's public API — every actual decode path is exercised.

## Sanitizers

The default sanitizer set depends on the platform:

- **Linux / macOS / OHOS GN:** `-fsanitize=fuzzer,address -fsanitize-recover=address`.
  ASan catches heap-overflow, use-after-free, double-free, stack-buffer-overflow
  and similar memory-corruption bugs that libFuzzer alone would miss. `recover`
  lets the fuzz loop continue past a finding instead of aborting.
- **Windows:** `-fsanitize=fuzzer` only. Clang's prebuilt ASan on Windows
  requires the whole link graph to be `/MT`; we already enforce that, but the
  linker is fussier in practice so ASan is opt-in via `-DFUZZ_SANITIZERS=fuzzer,address`.

Override the default with the cache variable:

```sh
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer            # crashes/hangs/OOMs only
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer,address    # + memory bugs
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer,undefined  # + UB
```

ASan costs ~2× CPU and ~3× memory. Worth it for a CI fuzz job; turn off for
quick local crash-finding iterations.

## Seed corpus

Lives in tree at `jpg_image_loader_fuzzer/corpus/` (same layout as
LumeEngine's `image_astc_fuzzer/corpus/`). The OHOS `ohos_fuzztest`
template auto-bundles it via `fuzz_config_file`; the standalone CMake
build stages it next to the binary at `POST_BUILD`. Current seeds are
all LumeJpg-owned, ~6 KB total:

| File | Purpose |
|---|---|
| `VULN-032_jpg_large_dimensions.jpg` | Regression case for the large-dimensions crash. |
| `minimal_rgb_1x1.jpg` | Smallest valid JPEG path. |
| `baseline_gray_8x8.jpg` | Grayscale (1-component) decode path. |
| `baseline_rgb_8x8.jpg` | RGB → JCS_EXT_RGBA conversion path. |
| `baseline_rgb_64x64.jpg` | Full scanline-read loop, larger buffer alloc. |
| `progressive_rgb_32x32.jpg` | Progressive (multi-scan) decode path inside libjpeg. |

Add more by dropping JPEGs into `jpg_image_loader_fuzzer/corpus/` and
re-running `cmake --build` — no reconfigure needed. New findings
discovered during fuzzing land in the build's `corpus/` directory;
periodically `-merge=1` them back and commit the survivors if they
cover new ground.

## What the harness exercises

Per input, **all** of these are invoked (so a single seed file feeds every
overload, not just one):

1. `loader->Load(array_view, flags)` with raw fuzz bytes — drives the full
   decode path for real JPEG seeds.
2. `loader->Load(array_view, flags)` with a fuzz-synthesised JPEG whose
   width / height / precision / channel-count come from the input bytes —
   targets the wrapper's `MAX_IMAGE_EXTENT` / `IMG_SIZE_LIMIT_2GB` / channel
   count guards above libjpeg.
3. `loader->CanLoad(array_view)` — exercises the 10-byte sniff (DQT, JFIF,
   Exif, XMP, ICC_PROFILE, MPF, Adobe).
4. `loader->Load(IFile&, flags)` backed by an in-harness `MemoryFile` —
   covers the IFile size guard, buffer alloc and `file.Read` path.
5. `loader->LoadAnimatedImage(IFile&, flags)` — covers the animated
   overload + `ResultFailureAnimated` helper.
6. `loader->LoadAnimatedImage(array_view, flags)` — companion overload.
7. `loader->Load(IFile&, flags)` with a `MemoryFile` that lies about its
   length (`GetLength() > INT_MAX`) — hits the "File too big." early return.
8. `loader->Load(IFile&, flags)` with a `MemoryFile` whose `Read` returns
   short — hits the "Reading file failed." branch.

`loadFlags` cycles deterministically through 10 variants derived from a
hash of the input bytes (so the same input always hits the same path,
preserving libFuzzer's coverage model). The variants include
`PREMULTIPLY_ALPHA | FORCE_GRAYSCALE` (the only combination that hits
`PremultiplyAlpha`'s `channelCount != 4 && != 2` early return) and
`METADATA_ONLY | FORCE_GRAYSCALE`.

## Layout

Mirrors the layout that LumeEngine's `image_astc_fuzzer` / `image_ktx_fuzzer`
and Lume3D's `gltf2loader_fuzzer` use — same `BUILD.gn` shape, `project.xml`
config, dictionary next to the binary, prelude force-include via the same
flag form. The only LumeJpg-specific addition is the Windows-compatibility
plumbing in `CMakeLists.txt`.

```
test/fuzztest/
  BUILD.gn                                 -- group target for the GN build
  CMakeLists.txt                           -- standalone CMake build (Linux + Windows)
  README.md                                -- this file
  fuzz_consumer.h                          -- shared input-bytes helper
  fuzz_prelude.h                           -- -include into every TU: no-ops
                                              CORE_LOG_*/CORE_ASSERT so the
                                              fuzz target can build without
                                              the engine logger/plugin register
                                              (same pattern as LumeEngine)
  jpg_image_loader_fuzzer/
    BUILD.gn                               -- ohos_fuzztest target
    corpus/                                -- in-tree LumeJpg-owned seed JPEGs
                                              (regression + format variants);
                                              auto-bundled by ohos_fuzztest and
                                              copied next to the binary by the
                                              CMake build at POST_BUILD
    jpg.dict                               -- libFuzzer JPEG-marker dictionary;
                                              staged the same way as corpus
    jpg_image_loader_fuzzer.cpp            -- LLVMFuzzerTestOneInput
    jpg_image_loader_fuzzer.h              -- header for the project name
    project.xml                            -- max_len / max_time / rss_limit
```
