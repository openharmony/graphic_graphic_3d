# LumePng image loader fuzz test

libFuzzer harness for `PNGPlugin::ImageLoaderPng` (i.e. `src/png/image_loader_png.cpp`).
Targets every public entry point of the loader (`Load(array_view, flags)`,
`Load(IFile&, flags)`, the two 3D-texture overloads, both `LoadAnimatedImage`
overloads, `CanLoad`, `GetSupportedTypes`) plus the wrapper-level size /
channel-count / 3D-divisibility guards above libpng.

Builds three ways from the same harness source. Same structural layout as
LumeEngine's `image_astc_fuzzer` / `image_ktx_fuzzer`, Lume3D's
`gltf2loader_fuzzer` and LumeJpg's `jpg_image_loader_fuzzer`; additionally
builds standalone on Windows Clang behind platform guards so local iteration
on a Windows dev box works the same way.

- **OHOS GN** (`BUILD.gn` + `project.xml`) — production target, `ohos_fuzztest`.
- **Standalone CMake** (`CMakeLists.txt`) — Linux or Windows Clang without the
  OHOS build pipeline. Fast local iteration + coverage.

---

## Quick start — Linux

```sh
sudo apt install -y clang lld cmake ninja-build build-essential nasm

cd LumePng
git submodule update --init --recursive

cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_PNG_BUILD_ENGINE=OFF \
  -DPNG_FUZZ_TESTS_ENABLED=ON

cmake --build build-fuzz --target png_image_loader_fuzzer

build-fuzz/test/fuzztest/png_image_loader_fuzzer \
  -dict=png.dict -max_total_time=300 -print_final_stats=1 \
  build-fuzz/test/fuzztest/corpus/
```

Defaults to `-fsanitize=fuzzer,address -fsanitize-recover=address` on Linux so
heap-overflow / UAF / OOB-read bugs are caught instead of silently corrupting
memory. Override with `-DFUZZ_SANITIZERS=fuzzer` if you only want crash-finding
throughput. ASan adds ~2× CPU and ~3× memory.

## Quick start — Windows (Clang + Ninja)

```powershell
cd D:\GitCode\LumePng
git submodule update --init --recursive

cmake -S . -B build-fuzz -G Ninja `
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" `
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DCORE_PNG_BUILD_ENGINE=OFF `
  -DPNG_FUZZ_TESTS_ENABLED=ON

cmake --build build-fuzz --target png_image_loader_fuzzer

.\build-fuzz\test\fuzztest\png_image_loader_fuzzer.exe `
  -dict=png.dict -max_total_time=300 -print_final_stats=1 `
  .\build-fuzz\test\fuzztest\corpus\
```

`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` is needed because Clang's prebuilt
libFuzzer runtime on Windows is `/MT_StaticRelease`; without it `lld-link`
reports a `RuntimeLibrary` mismatch. Windows defaults to `-fsanitize=fuzzer`
(no ASan) — opt in with `-DFUZZ_SANITIZERS=fuzzer,address` once you've
confirmed every link input is `/MT`.

## OHOS GN build (production target)

```sh
gn gen out/png_fuzz --args='is_debug=false'
ninja -C out/png_fuzz \
  foundation/graphic/graphic_3d/lume/LumePng/test/fuzztest:fuzztest
```

The `ohos_fuzztest("PngImageLoaderFuzzTest")` target uses the same
`project.xml` config (`max_len=131072`, `max_total_time=300`, `rss_limit_mb=4096`)
as Lume3D's gltf fuzzer and LumeJpg's jpg fuzzer.

---

## What the harness exercises

Per input, **all** of these are invoked (so one seed file feeds every
overload, not just one):

1. `loader->Load(array_view, flags)` raw bytes — full decode path.
2. `loader->Load(array_view, flags, rowCount, columnCount)` — 3D texture
   variant, drives `Resolve3DTextureInfo`, `Handle3DTexture`,
   `Copy3DTextureBytes`.
3. `loader->Load(IFile&, flags)` via in-harness `MemoryFile`.
4. `loader->Load(IFile&, flags, rowCount, columnCount)` — 3D + IFile.
5. `loader->LoadAnimatedImage(IFile&, flags)`.
6. `loader->LoadAnimatedImage(array_view, flags)`.
7. `loader->CanLoad(array_view)` — wraps libpng's `png_sig_cmp`.
8. Synthetic PNG mode: signature + IHDR (fuzz-controlled
   width/height/bit-depth/color-type/interlace) + PLTE if palette + IDAT
   (fuzz bytes) + IEND. Uses libz's `crc32` so chunks pass libpng's
   structural checks; the IDAT compressed data is almost always invalid,
   driving libpng's `error_handler` → `longjmp` cleanup branch.
9. Fake-huge-length `MemoryFile` → `"File too big."` early return.
10. Short-read `MemoryFile` → `"Reading file failed."` branch.

`loadFlags` cycles deterministically through 10 variants derived from a hash
of the input (preserves libFuzzer's coverage model) including
`PREMULTIPLY_ALPHA | FORCE_GRAYSCALE` (only combination that hits
`PremultiplyAlpha`'s `channelCount != 4 && != 2` early return) and
`METADATA_ONLY | FORCE_GRAYSCALE`.

`rowCount` and `columnCount` for the 3D modes are also derived from the hash
(1 – 16), so the fuzzer hits valid divisors of the seed's dimensions plus
invalid combinations.

## Seed corpus

Lives in tree at `png_image_loader_fuzzer/corpus/`. The OHOS `ohos_fuzztest`
template auto-bundles it via `fuzz_config_file`; the standalone CMake build
stages it next to the binary at `POST_BUILD`. All seeds are LumePng-owned,
~2 KB total:

| File | Purpose |
|---|---|
| `minimal_rgb_1x1.png` | Smallest valid PNG path. |
| `baseline_gray_8x8.png` | Grayscale (1-channel) decode path. |
| `baseline_rgb_8x8.png` | RGB (3-channel) decode path. |
| `baseline_rgba_8x8.png` | RGBA (4-channel) + alpha path; triggers `PremultiplyAlpha`. |
| `palette_8x8.png` | `PLTE` chunk → `png_set_palette_to_rgb` path. |
| `interlaced_rgb_16x16.png` | Adam7 interlaced decode path. |
| `16bpc_gray_8x8.png` | 16-bit-per-channel + `png_set_scale_16` path. |
| `texture3d_12x16_rgba.png` | 12 × 16 divides cleanly into 3 × 4 for the 3D texture overload. |

Add more by dropping PNGs into `png_image_loader_fuzzer/corpus/` and
re-running `cmake --build` — no reconfigure needed.

## Running

| Flag | Effect |
|---|---|
| `-max_total_time=N` | Stop after N seconds. |
| `-runs=N` | Stop after N executions. `0` replays the corpus and exits. |
| `-max_len=N` | Cap mutated input size to N bytes. |
| `-rss_limit_mb=N` | Kill on OOM. Default 2048. |
| `-timeout=N` | Kill on hang. Default 1200 s. |
| `-jobs=N -workers=K` | Run N total fuzzing jobs, K in parallel. |
| `-artifact_prefix=DIR/` | Where to drop `crash-*` / `timeout-*` / `leak-*` artifacts. |
| `-dict=png.dict` | **Strongly recommended** — speeds up format discovery dramatically. |
| `-print_final_stats=1` | Summary at exit. |

When ASan is enabled (Linux default), LeakSanitizer triggers at process exit
and will end the fuzz session on the first leak. If you'd rather collect
crashes / OOB-reads now and triage leaks later, set
`ASAN_OPTIONS=detect_leaks=0`. Crashes and OOB accesses are still reported
in real time.

Replay a saved finding:
```sh
./png_image_loader_fuzzer path/to/crash-<sha1>
```

## Coverage measurement

Source-based coverage is opt-in to keep production builds untouched:

```sh
cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_PNG_BUILD_ENGINE=OFF \
  -DPNG_FUZZ_TESTS_ENABLED=ON \
  -DPNG_FUZZ_COVERAGE=ON
cmake --build build-fuzz --target png_image_loader_fuzzer

cd build-fuzz/test/fuzztest
rm -rf prof merged.profdata cov_html
mkdir prof
LLVM_PROFILE_FILE="prof/cov-%p-%m.profraw" \
  ./png_image_loader_fuzzer -dict=png.dict -max_total_time=180 corpus/

llvm-profdata merge -sparse prof/*.profraw -o merged.profdata
llvm-cov report ./png_image_loader_fuzzer -instr-profile=merged.profdata
llvm-cov show ./png_image_loader_fuzzer -instr-profile=merged.profdata \
  -show-branches=count -show-line-counts-or-regions \
  -format=html -output-dir=cov_html \
  ../../../src/png/image_loader_png.cpp
```

Open `cov_html/index.html` for the annotated source view.

## Sanitizers

- **Linux / macOS / OHOS GN:** `-fsanitize=fuzzer,address -fsanitize-recover=address`
  by default. ASan catches heap-overflow, UAF, double-free, etc.
  `recover` lets the fuzz loop continue past a finding instead of aborting.
- **Windows:** `-fsanitize=fuzzer` only by default. ASan on Windows requires
  the whole link graph to be `/MT`; we already enforce that, but the linker is
  fussier in practice so ASan is opt-in.

Override the default with the cache variable:

```sh
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer            # crashes/hangs/OOMs only
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer,address    # + memory bugs
cmake -B build-fuzz ... -DFUZZ_SANITIZERS=fuzzer,undefined  # + UB
```

## Layout

```
test/fuzztest/
  BUILD.gn                                   -- group target for the GN build
  CMakeLists.txt                             -- standalone CMake build (Linux + Windows)
  README.md                                  -- this file
  fuzz_consumer.h                            -- shared input-bytes helper
  fuzz_prelude.h                             -- -include into every TU: no-ops
                                                CORE_LOG_*/CORE_ASSERT so the
                                                fuzz target can build without
                                                the engine logger/plugin register
  png_image_loader_fuzzer/
    BUILD.gn                                 -- ohos_fuzztest target
    corpus/                                  -- in-tree LumePng-owned PNG seeds
    png.dict                                 -- libFuzzer PNG-marker dictionary
    png_image_loader_fuzzer.cpp              -- LLVMFuzzerTestOneInput
    png_image_loader_fuzzer.h
    project.xml                              -- max_len / max_time / rss_limit
```
