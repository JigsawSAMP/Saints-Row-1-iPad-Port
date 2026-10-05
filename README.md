# Saints Row 1 — experimental iPad port

A native iPadOS adaptation of [whompay/SaintsReborn](https://github.com/whompay/SaintsReborn). **The PC port and campaign co-op mod are upstream work by Whompay and Saints Reborn contributors.** This repository contains the Apple host, controller bridge, reproducible portability patches and build scripts. It is not affiliated with the game publisher or an official release.

The game executes locally on iPad through recompiled Xbox 360 guest code, ReXGlue, Vulkan/Xenos, MoltenVK/Metal and SDL audio. It does not stream from a PC.

## Current state

- Story startup, character creation and early gameplay have run on a physical Apple Silicon iPad. A complete story playthrough has not been verified.
- Physical controllers use the original Xbox action mappings and right-stick camera. The active app has no touch gameplay overlay or mouse/swipe camera input. Experimental touch configuration source remains for future development.
- Landscape presentation fills the screen by stretching the original 16:9 picture. Aspect-correct iPad camera/HUD rendering is not implemented.
- Options → iPad Display selects Performance (640×360), Balanced (960×540), or Quality (1280×720). Relaunch to apply. Single-pass rendering, MSAA off and shadows off are used.
- This remains an experimental port. Low FPS and intermittent freezes have been reported; stable 60 FPS is not achieved or promised. Recent fixes address ARM64 ordering, CPU diagnostic/wait overhead and UI-blocking GPU fence waits.
- A guest-native Multiplayer menu wraps campaign co-op Host, Direct Connect and Disconnect. Enter an IPv4 address with D-pad left/right (select octet), up/down (change value), A (join), B (cancel).
- Co-op preserves the pinned upstream V3 protocol, packed packets and two-player campaign model. Native transport loopback tests pass; a complete Windows/iPad gameplay session is **unverified**. Epic Online Services, relay and NAT traversal are absent. UDP 27015 must be reachable.
- Server Browser and Host Server are disabled placeholders for future session backends. Custom gamemodes and general console multiplayer are not implemented.

## Build and installation

See [BUILD.md](BUILD.md) for prerequisites, pinned revisions, game extraction, native builds, signing, installation and tests. You need your own compatible game disc image and Apple signing identity. **No game executable, assets, ISO, generated guest code, signed app, certificates, device identifiers or personal signing details are included.**

See [CLEANUP.md](CLEANUP.md) before removing development files and [UPLOAD.md](UPLOAD.md) for publishing this source-only package to GitHub.

## Credits and licensing

- [SaintsReborn](https://github.com/whompay/SaintsReborn): PC recompiled port, game hooks, menu reference and WhompaysCoop. MIT notice retained in [licenses/SaintsReborn-MIT.txt](licenses/SaintsReborn-MIT.txt).
- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), including Xenia-derived components: runtime, recompilation, Xbox kernel and graphics. BSD-3-Clause notice retained.
- [MoltenVK](https://github.com/KhronosGroup/MoltenVK) and [Vulkan Loader](https://github.com/KhronosGroup/Vulkan-Loader): Vulkan/Metal and loading components. Their notices are retained.
- [GTA SA reference repository](https://github.com/isweikton/gtasa-android-sourcecode): inspected during the earlier mobile-input experiment. No GTA source or assets are copied or linked here.

Original port glue is MIT-licensed. SDK/dependency patches retain their respective upstream licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Game content and game-derived generated files are not covered by these source licenses and are excluded from this package.
