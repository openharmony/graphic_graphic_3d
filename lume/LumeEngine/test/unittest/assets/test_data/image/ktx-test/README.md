KTX test files
==============

This repository provides test files for the [KTX] texture format.
The files are distributed under the [CC0], you are free to use
them without restriction.


Tools
-----

The test data is maintained in JSON files, which are
converted to the final KTX files with [ktxjuggle].
Many of the source files are manually edited, to provide
comprehensive test cases with custom mipmaps, metadata,
or deliberate errors.

The binary pixel data was produced with:

- [PVRTexToolCLI] 4.22 (raw, PVRTC, ETC)
- [astcenc] 1.7 (ASTC)
- [CompressonatorCLI] 3.1.307 (BC)



[CC0]: https://creativecommons.org/publicdomain/zero/1.0
[KTX]: https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
[KTX2]: https://github.com/KhronosGroup/KTX-Specification

[astcenc]: https://github.com/ARM-software/astc-encoder
[CompressonatorCLI]: https://github.com/GPUOpen-Tools/Compressonator
[ktxjuggle]: https://github.com/sevmeyer/ktxjuggle
[PVRTexToolCLI]: https://www.imgtec.com/developers/powervr-sdk-tools/pvrtextool/
