# Build the iPad port

## Prerequisites

Use an Apple Silicon Mac with full Xcode and an iPadOS SDK. The development build used Xcode 26.6; the Xcode app target currently requires iPadOS 26.5 or later. The native CMake target uses an 18.0 deployment target, but this is not a claim that the complete app supports iPadOS 18. Earlier Xcode/iPadOS versions have not been validated.

Install Python 3, Git, CMake (3.25+ for the wrapper; use a recent version compatible with the pinned SDK), and Ninja. Ensure `xcodebuild`, `xcrun`, `cmake`, `ninja`, `git` and `python3` are on PATH. Select full Xcode as the active developer directory and complete Xcode's initial setup/license acceptance. Dependencies are fetched from GitHub during preparation; builds need several GB of free disk space in addition to game assets.

The scripts preserve existing checkouts rather than resetting them. Run commands from the repository root. Upstream revisions are pinned:

| Component | Revision |
| --- | --- |
| SaintsReborn | `1f5fd63f199f4fb0adc2185977612c6bb8856812` |
| ReXGlue SDK | `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518` |

SDK dependency submodules follow the pinned SDK checkout. `prepare` applies SaintsReborn's SDK patch first, then the Apple SDK, MoltenVK, Vulkan Loader and guest-hook patches.

## 1. Supply and extract your own game

Place exactly one compatible Saints Row Xbox 360 ISO in the repository root. The supported World disc's `default.xex` has SHA-1 `c2646093ff3926141fcf4fd630e876873d1caece`. Another executable revision is rejected; do not bypass the check without updating the guest manifest and hooks.

```sh
python3 PortSupport/port.py prepare
```

This fetches pinned upstream sources/submodules, applies patches, builds the local extractor and extracts assets to `SaintsReborn/dist/game`. Console updates and disc filler are skipped. An already extracted matching `default.xex` can be reused. Game content must stay out of GitHub; the supplied `.gitignore` excludes the ISO, fetched sources and extracted/generated directories.

## 2. Recompile and build native libraries

```sh
python3 PortSupport/port.py host
python3 PortSupport/port.py ios
python3 PortSupport/port.py input-app
```

`host` builds the recompiler and generates ARM64-targeted C++ guest sources. `ios` builds the game hooks, Apple input bridge, runtime, GPU plugin and MoltenVK. The historical `input-app` name now builds the actual native game app, not a controls-only demo. Without a team/device it builds an unsigned app in `PortBuild/iPadDerivedData`; it does not install it.

This package's patches were checked against clean pinned source snapshots. A completely new download-to-device build of the sanitized package has not been completed; the underlying development tree was built and installed successfully. Build errors on a different toolchain should be reported with sanitized logs.

## 3. Configure your own signing

Connect and unlock the iPad, trust the Mac, enable Developer Mode and add your own Apple account in Xcode → Settings → Accounts. Discover the connected device identifier with `xcrun devicectl list devices` or Xcode's device window. Use your own team ID and a bundle ID available to your team; the public Xcode project contains no team and defaults to `org.example.saintsrow.ipad`.

```sh
python3 PortSupport/port.py input-app --team YOUR_TEAM_ID --device YOUR_DEVICE_ID --bundle-id YOUR_BUNDLE_ID
```

Replace each placeholder before running it. Signing uses a temporary directory outside the checkout and removes that directory automatically. Do not commit credentials, signing profiles or private keys. Alternatively open `Saints Row 1/Saints Row 1.xcodeproj`, choose your team and unique bundle ID under Signing & Capabilities, and build/run after the native libraries exist.

To replace an existing installation while retaining its Documents/save data, use the **same existing bundle ID and signing team**. A different bundle ID creates a separate app/container. This public package intentionally does not contain the original private bundle ID or team ID. Never uninstall/offload the app as a renewal step.

## 4. Transfer game assets once

After installation, transfer the extracted game folder into the app's Documents container:

```sh
python3 PortSupport/port.py install-assets --device YOUR_DEVICE_ID --bundle-id YOUR_BUNDLE_ID
```

The destination is `Documents/game`, with `default.xex` directly inside that folder. Transfer can take several minutes. Do not launch until it finishes. Relaunch Saints Row afterwards. Subsequent same-ID signed updates reuse existing device assets; no Mac streaming is required. The script does not erase the app's Documents or saves.

## 5. Co-op testing

Use a PC build from the same pinned SaintsReborn revision (WhompaysCoop identifies itself as 1.63). A protocol label alone does not guarantee binary compatibility across revisions. Start with direct IPv4 hosting on a reachable LAN, allow iPad local-network access and make UDP 27015 reachable on the host. Use the native Multiplayer list to host/join a campaign save. No EOS join codes, Internet relay or automatic NAT traversal are provided. Real cross-platform gameplay, mission synchronization and disconnect recovery remain unverified.

## Diagnostics and tests

Portable input bridge test:

```sh
mkdir -p PortBuild/tools
clang++ -std=c++23 -O1 -g -fsanitize=address,undefined -pthread -I"Saints Row 1/Saints Row 1" PortSupport/tests/input_bridge.cpp "Saints Row 1/Saints Row 1/SRInput.cpp" -o PortBuild/tools/input_bridge_test
PortBuild/tools/input_bridge_test
```

Co-op transport test, after the iOS native build:

```sh
python3 PortSupport/tests/run_coop_transport.py
```

The test runs the actual adapted mod on the Mac with AddressSanitizer and checks host/join, V3 handshake, chat packet layout and disconnect. It is not a Windows/iPad gameplay test.

Additional test sources cover retained experimental mobile input and SDK POSIX wait behavior. Device logs live in the app's Documents and are bounded/rotated. Remove personal paths, addresses and account/device identifiers before sharing logs. Do not publish saves or game assets.

When editing SDK/game patches, `python3 PortSupport/export_patches.py` exports and verifies changes relative to the pinned, upstream-patched SDK. Keep patched sources until that succeeds.
