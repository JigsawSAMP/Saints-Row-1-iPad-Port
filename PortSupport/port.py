#!/usr/bin/env python3
"""Build the experimental native Saints Row iPad port from pinned sources."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SDK = ROOT / 'PortDependencies/rexglue-sdk'
GAME = ROOT / 'SaintsReborn'
BUILD = ROOT / 'PortBuild'
SDK_PIN = 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'
GAME_PIN = '1f5fd63f199f4fb0adc2185977612c6bb8856812'
XEX_SHA1 = 'c2646093ff3926141fcf4fd630e876873d1caece'


def run(args, cwd=ROOT):
    print('+', ' '.join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], cwd=cwd, check=True)


def pinned_checkout(url, directory, pin):
    if not directory.exists():
        directory.parent.mkdir(parents=True, exist_ok=True)
        run(['git', 'clone', '--no-checkout', '--depth', '1', url, directory])
        run(['git', 'fetch', '--depth', '1', 'origin', pin], cwd=directory)
        run(['git', 'checkout', '--detach', pin], cwd=directory)
    actual = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=directory, text=True).strip()
    if actual != pin:
        raise RuntimeError('Unexpected revision in ' + str(directory) + '; preserving the checkout.')


def prepare():
    BUILD.mkdir(exist_ok=True)
    pinned_checkout('https://github.com/whompay/SaintsReborn.git', GAME, GAME_PIN)
    pinned_checkout('https://github.com/rexglue/rexglue-sdk.git', SDK, SDK_PIN)
    state_file = SDK / '.ipad-port-patches.json'
    patches = [
        (SDK, GAME / 'patches/rexglue-sdk.patch'),
        (SDK, ROOT / 'PortSupport/patches/rexglue-apple.patch'),
        (SDK / 'thirdparty/moltenvk', ROOT / 'PortSupport/patches/moltenvk-ios.patch'),
        (SDK / 'thirdparty/vulkan-loader', ROOT / 'PortSupport/patches/vulkan-loader-ios.patch'),
        (GAME, ROOT / 'PortSupport/patches/saintsreborn-apple.patch'),
    ]
    signatures = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for _, p in patches}
    if state_file.exists():
        if json.loads(state_file.read_text()) != signatures:
            raise RuntimeError('Patches changed; update the existing SDK manually to preserve local edits.')
    else:
        run(['git', 'submodule', 'update', '--init', '--recursive', '--depth', '1', '--jobs', '4'], cwd=SDK)
        for checkout, patch in patches:
            run(['git', 'apply', '--check', patch], cwd=checkout)
            run(['git', 'apply', '--whitespace=nowarn', patch], cwd=checkout)
        state_file.write_text(json.dumps(signatures, indent=2) + '\n')
    (BUILD / 'tools').mkdir(exist_ok=True)
    extractor = BUILD / 'tools/xiso_extract'
    run(['xcrun', 'clang++', '-std=c++20', '-O2', GAME / 'tools/xiso_extract/xiso_extract.cpp', '-o', extractor])
    xex = GAME / 'dist/game/default.xex'
    if not xex.exists():
        images = list(ROOT.glob('*.iso'))
        if len(images) != 1:
            raise RuntimeError('Expected one ISO in the workspace; extraction was not started.')
        run([extractor, images[0], GAME / 'dist/game', '--skip-name', '$SystemUpdate',
             '--skip-name', 'layer1filler.bin', '--skip-name', 'layer2filler.bin'])
    if hashlib.sha1(xex.read_bytes()).hexdigest() != XEX_SHA1:
        raise RuntimeError('This default.xex is not the supported disc revision.')


def host():
    BUILD.mkdir(exist_ok=True)
    run(['cmake', '-UCPM_DIRECTORY', '-UCPM_FILE', '-S', SDK, '-B', BUILD / 'host-sdk', '-G', 'Ninja',
         '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=/usr/bin/clang',
         '-DCMAKE_CXX_COMPILER=/usr/bin/clang++', '-DCMAKE_OSX_ARCHITECTURES=arm64',
         '-DREXGLUE_BUILD_TOOLS=ON', '-DREXGLUE_ENABLE_TRACY=OFF', '-DREXGLUE_ENABLE_PERF_COUNTERS=OFF',
         '-DREX_OUT_DIR=' + str(BUILD / 'host-products'),
         '-DCPM_SOURCE_CACHE=' + str(BUILD / 'cpm-cache'),
         '-DCMAKE_INSTALL_PREFIX=' + str(BUILD / 'sdk-install')])
    run(['cmake', '--build', BUILD / 'host-sdk', '--target', 'rexglue', '-j', '4'])
    run([BUILD / 'host-products/rexglue', 'codegen', GAME / 'config/saintsrow_manifest.toml', '--ignore-stamp'])


def ios():
    run(['cmake', '-S', ROOT / 'PortSupport', '-B', BUILD / 'ipad-port', '-G', 'Ninja',
         '-DCMAKE_SYSTEM_NAME=iOS', '-DCMAKE_OSX_SYSROOT=iphoneos', '-DCMAKE_OSX_ARCHITECTURES=arm64',
         '-DCMAKE_SYSTEM_PROCESSOR=arm64', '-DCMAKE_OSX_DEPLOYMENT_TARGET=18.0',
         '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=/usr/bin/clang', '-DCMAKE_CXX_COMPILER=/usr/bin/clang++'])
    run(['cmake', '--build', BUILD / 'ipad-port', '--target', 'sr_guest_code', 'sr_apple_input', 'sr_game_hooks', 'SRGameBridge', 'rexgpu-xenos', 'MoltenVK', '-j', '4'])


def input_app(team, device, bundle_id):
    # The Apple signing team is provided explicitly; never select an arbitrary work account.
    command = ['xcodebuild', '-project', ROOT / 'Saints Row 1/Saints Row 1.xcodeproj',
               '-scheme', 'Saints Row 1', '-configuration', 'Debug', '-sdk', 'iphoneos',
               '-derivedDataPath', BUILD / 'iPadDerivedData', 'CODE_SIGNING_ALLOWED=NO', 'PRODUCT_BUNDLE_IDENTIFIER=' + bundle_id, 'build']
    run(command)
    if not team or not device:
        print('Unsigned native-development app built. Supply --team and --device to install. Gameplay and cross-play require hardware validation.')
        return
    # A temporary directory outside iCloud Documents avoids file-provider Finder
    # metadata that makes codesign reject app bundles. It is removed on all exits.
    with tempfile.TemporaryDirectory(prefix='saintsrow-native-build-') as scratch:
        derived = Path(scratch) / 'DerivedData'
        command = ['xcodebuild', '-project', ROOT / 'Saints Row 1/Saints Row 1.xcodeproj',
                   '-scheme', 'Saints Row 1', '-configuration', 'Debug', '-destination', 'id=' + device,
                   '-derivedDataPath', derived, '-allowProvisioningUpdates', '-allowProvisioningDeviceRegistration',
                   'DEVELOPMENT_TEAM=' + team, 'PRODUCT_BUNDLE_IDENTIFIER=' + bundle_id, 'build']
        run(command)
        run(['xcrun', 'devicectl', 'device', 'install', 'app', '--device', device,
             derived / 'Build/Products/Debug-iphoneos/Saints Row 1.app'])


def install_assets(device, bundle_id):
    if not device:
        raise RuntimeError('Supply --device for asset transfer.')
    assets = GAME / 'dist/game'
    if not (assets / 'default.xex').exists():
        raise RuntimeError('Run prepare first. Game assets are not included in this repository.')
    run(['xcrun', 'devicectl', 'device', 'copy', 'to', '--device', device,
         '--domain-type', 'appDataContainer', '--domain-identifier', bundle_id,
         '--source', assets, '--destination', 'Documents/game'])


def clean():
    # Only disposable outputs; retain both source checkouts, input app source,
    # portability patches, tests and the user's original ISO.
    for directory in [BUILD, GAME / 'dist', GAME / 'build', SDK / 'out']:
        if directory.is_dir():
            shutil.rmtree(directory)
            print('Removed', directory)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('step', choices=['prepare', 'host', 'ios', 'input-app', 'install-assets', 'clean'])
    parser.add_argument('--team')
    parser.add_argument('--device')
    parser.add_argument('--bundle-id', default='org.example.saintsrow.ipad')
    args = parser.parse_args()
    if args.step == 'input-app':
        input_app(args.team, args.device, args.bundle_id)
    elif args.step == 'install-assets':
        install_assets(args.device, args.bundle_id)
    else:
        globals()[args.step]()


if __name__ == '__main__':
    main()
