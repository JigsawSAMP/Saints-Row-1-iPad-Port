# Third-party notices

This source package fetches upstream projects during builds instead of bundling their full checkouts or binaries. Original Apple host/build glue uses the root MIT license. Adapted SaintsReborn code and the SaintsReborn guest-hook patch retain the upstream MIT notice. ReXGlue SDK patches retain the SDK's BSD-3-Clause license (including Xenia notices). MoltenVK and Vulkan Loader patches retain their respective source licenses. Required notices are preserved under `licenses/`.

- SaintsReborn: https://github.com/whompay/SaintsReborn — `licenses/SaintsReborn-MIT.txt` and `licenses/SaintsReborn-THIRD-PARTY-NOTICES.md`.
- ReXGlue SDK / Xenia-derived components: https://github.com/rexglue/rexglue-sdk — `licenses/ReXGlue-BSD-3-Clause.txt`.
- MoltenVK: https://github.com/KhronosGroup/MoltenVK — `licenses/MoltenVK-Apache-2.0.txt`.
- Vulkan Loader: https://github.com/KhronosGroup/Vulkan-Loader — `licenses/Vulkan-Loader-LICENSE.txt`.

The fetched SDK brings additional dependencies such as SDL, FFmpeg, fmt, spdlog, Vulkan headers and shader tools. Their licenses remain in the fetched dependency sources. Redistributors of built binaries must review and comply with all applicable upstream notices; this source package does not include those binaries.

No commercial game content, disc image, game executable, save data or generated game-derived guest source is redistributed here. The source licenses do not grant rights to Saints Row content.
