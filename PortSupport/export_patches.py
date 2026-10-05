#!/usr/bin/env python3
"""Save Apple changes relative to SaintsReborn's already-patched SDK."""
from pathlib import Path
import difflib
import hashlib
import json
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SDK = ROOT / 'PortDependencies/rexglue-sdk'
PATCHES = ROOT / 'PortSupport/patches'
# Include every edited SDK source, not just a hand-maintained subset. The
# baseline already contains SaintsReborn's SDK patch, so unchanged upstream
# edits disappear from the Apple delta automatically.
changed = subprocess.check_output(['git', 'diff', '--name-only'], cwd=SDK, text=True).splitlines()
added = subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard'], cwd=SDK, text=True).splitlines()
FILES = sorted({name for name in changed + added if (SDK / name).is_file()
                and name != '.ipad-port-patches.json'})
PATCHES.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='sdk-baseline-', dir=ROOT / 'PortBuild') as temp:
    base = Path(temp)
    with (base / 'source.tar').open('wb') as archive:
        subprocess.run(['git', 'archive', 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'], cwd=SDK, stdout=archive, check=True)
    subprocess.run(['tar', '-xf', str(base / 'source.tar'), '-C', base], check=True)
    subprocess.run(['git', 'apply', str(ROOT / 'SaintsReborn/patches/rexglue-sdk.patch')], cwd=base, check=True)
    chunks = []
    for name in FILES:
        old = (base / name).read_text() if (base / name).exists() else ''
        new = (SDK / name).read_text()
        if old == new:
            continue
        header = 'diff --git a/' + name + ' b/' + name + '\n'
        if not (base / name).exists():
            header += 'new file mode 100644\n'
        chunks.append(header + ''.join(difflib.unified_diff(old.splitlines(keepends=True), new.splitlines(keepends=True),
            fromfile='a/' + name if old else '/dev/null', tofile='b/' + name)))
    patch = PATCHES / 'rexglue-apple.patch'
    patch.write_text(''.join(chunks))
    subprocess.run(['git', 'apply', '--check', str(patch)], cwd=base, check=True)
    subprocess.run(['git', 'apply', str(patch)], cwd=base, check=True)
    for name in FILES:
        assert (base / name).read_bytes() == (SDK / name).read_bytes(), name
for sub, name in [('moltenvk', 'moltenvk-ios.patch'), ('vulkan-loader', 'vulkan-loader-ios.patch')]:
    diff = subprocess.check_output(['git', 'diff'], cwd=SDK / 'thirdparty' / sub, text=True)
    (PATCHES / name).write_text(diff)
game_diff = subprocess.check_output(['git', 'diff', '--', 'project/src/stubs.cpp'], cwd=ROOT / 'SaintsReborn', text=True)
(PATCHES / 'saintsreborn-apple.patch').write_text(game_diff)
patches = [PATCHES / 'saintsreborn-apple.patch', ROOT / 'SaintsReborn/patches/rexglue-sdk.patch', PATCHES / 'rexglue-apple.patch',
           PATCHES / 'moltenvk-ios.patch', PATCHES / 'vulkan-loader-ios.patch']
(SDK / '.ipad-port-patches.json').write_text(json.dumps({str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                                       for p in patches}, indent=2) + '\n')
print('Apple patch round-trip verified; preparation state recorded.')
