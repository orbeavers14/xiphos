#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Create a NEW development bundle with its non-Apple Mach-O dependency closure.

Requires explicit --development-only. Does not alter source binaries or project
OS policy. Notices/relink material, TLS/locale resources, release signing and
older-OS execution remain separate gates; output is not a redistributable release.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import plistlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

spec = importlib.util.spec_from_file_location('runtime_audit', Path(__file__).with_name('runtime-audit.py'))
audit_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit_module)

def run(*args):
    subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def build(source, destination, allow_floor):
    source = source.resolve(strict=True)
    destination = destination.parent.resolve(strict=True) / destination.name
    if destination.exists() or destination.is_symlink() or audit_module.inside(destination, source) or audit_module.inside(source, destination):
        raise ValueError('Choose a new app destination separate from the original')
    if source.suffix != '.app' or destination.suffix != '.app':
        raise ValueError('Source and destination must be .app bundles')
    report = audit_module.audit(source)
    unsupported = [issue for issue in report['issues'] if issue['kind'] not in ('external_sdk_dependency', 'minimum_os_mismatch')]
    if unsupported:
        raise ValueError('Resolve missing dependencies/architectures before bundling')
    resolutions = {}
    for edge in report['edges']:
        key = (edge['image'], edge['name'])
        if key in resolutions and resolutions[key] != edge['resolved']:
            raise ValueError('Context-dependent runtime dependency cannot be safely rewritten')
        resolutions[key] = edge['resolved']

    if report['observed_minimum_os'] != report['declared_minimum_os'] and not allow_floor:
        raise ValueError('Dependencies require a newer OS; rebuild SDKs or explicitly allow a development-only floor')
    for path in source.rglob('*'):
        if path.is_symlink() and not audit_module.inside(path.resolve(strict=True), source):
            raise ValueError('Source bundle has an external symlink')
    if len(report['images']) > 256 or sum(Path(row['path']).stat().st_size for row in report['images']) > 1024**3:
        raise ValueError('Runtime closure exceeds supported bounds')
    with tempfile.TemporaryDirectory(prefix='.xiphos-runtime-', dir=str(destination.parent)) as temporary:
        stage = Path(temporary) / destination.name
        shutil.copytree(source, stage, symlinks=True)
        frameworks = stage / 'Contents/Frameworks'
        frameworks.mkdir(parents=True, exist_ok=True)
        copied, names = {}, {}
        for row in report['images']:
            original = Path(row['path'])
            if audit_module.inside(original, source):
                copied[str(original)] = stage / original.relative_to(source)
            else:
                digest = hashlib.sha256(original.read_bytes()).hexdigest()
                name = digest[:16] + original.suffix
                target = frameworks / name
                if target.exists():
                    raise ValueError('Runtime filename collision')
                shutil.copy2(original, target)
                target.chmod(target.stat().st_mode | 0o200)
                copied[str(original)] = target
                names[str(original)] = name
        changed = set()
        for edge in report['edges']:
            target = copied[edge['image']]
            resolved = copied[edge['resolved']]
            name = '@executable_path/' + os.path.relpath(str(resolved), str(stage / 'Contents/MacOS'))
            key = (str(target), edge['name'], name)
            if key not in changed:
                run('/usr/bin/install_name_tool', '-change', edge['name'], name, str(target))
                changed.add(key)
        for original, name in names.items():
            run('/usr/bin/install_name_tool', '-id', name, str(copied[original]))
        info_path = stage / 'Contents/Info.plist'
        with info_path.open('rb') as file:
            info = plistlib.load(file)
        info['LSMinimumSystemVersion'] = report['observed_minimum_os']
        with info_path.open('wb') as file:
            plistlib.dump(info, file)
        resources = stage / 'Contents/Resources'
        resources.mkdir(exist_ok=True)
        (resources / 'DevelopmentRuntime.json').write_text(json.dumps({
            'development_only': True, 'redistribution_ready': False,
            'minimum_os': report['observed_minimum_os'],
            'dependencies': sorted(Path(name).name for name in names.values()),
            'remaining_gates': ['notices/source/relink material', 'TLS trust and locale/runtime resources',
                                'release signing/notarization', 'physical supported-OS execution']}, indent=2))
        for path in copied.values():
            run('/usr/bin/codesign', '--force', '--sign', '-', str(path))
        run('/usr/bin/codesign', '--force', '--sign', '-', str(stage))
        run('/usr/bin/codesign', '--verify', '--deep', '--strict', str(stage))
        verified = audit_module.audit(stage)
        if not verified['standalone_runtime_ready']:
            raise ValueError('Staged runtime audit failed after rewriting')
        if destination.exists() or destination.is_symlink():
            raise ValueError('Destination appeared during bundling')
        result = {'bundle': str(destination), 'bundled_sdk_libraries': len(names),
                  'minimum_os': report['observed_minimum_os'], 'development_only': True}
        stage.rename(destination)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--development-only', action='store_true', required=True)
    parser.add_argument('--allow-development-os-floor', action='store_true')
    args = parser.parse_args()
    try:
        result = build(args.source, args.destination, args.allow_development_os_floor)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(json.dumps({'bundle_error': str(error)}))
        return 1
    print(json.dumps(result, indent=2))
    return 0

if __name__ == '__main__':
    sys.exit(main())
