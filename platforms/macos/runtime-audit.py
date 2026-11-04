#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Read-only Mach-O dependency/minimum-OS audit. JSON contains local SDK paths.

No copying, loading, signing, downloads or application changes occur. A nonzero
exit means the artifact is not ready for standalone distribution; development
SDK linkage is reported rather than silently accepted as portable packaging.
"""
import argparse
import json
import plistlib
import re
import subprocess
import sys
from pathlib import Path

SYSTEM_ROOTS = ('/System/Library/', '/usr/lib/', '/Library/Apple/System/Library/')

def system(path):
    return str(path).startswith(SYSTEM_ROOTS)

def inside(path, parent):
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False

def command(*args):
    value = subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if len(value.stdout) > 8 * 1024 * 1024:
        raise ValueError('Unexpectedly large Mach-O metadata')
    return value.stdout

def version(text):
    return tuple(int(n) for n in text.split('.')) + (0,) * (3 - len(text.split('.')))

def metadata(path):
    loads = command('/usr/bin/otool', '-l', str(path))
    blocks = re.split(r'\nLoad command \d+\n', loads)
    rpaths, identities, minimums = [], set(), []
    for block in blocks:
        if re.search(r'\bcmd LC_RPATH\b', block):
            rpaths.extend(re.findall(r'\n\s+path (.*?) \(offset \d+\)', block))
        if re.search(r'\bcmd LC_ID_DYLIB\b', block):
            identities.update(re.findall(r'\n\s+name (.*?) \(offset \d+\)', block))
        if re.search(r'\bcmd LC_BUILD_VERSION\b', block):
            platforms = re.findall(r'\n\s+platform (\S+)', block)
            if any(value.upper() not in ('1', 'MACOS') for value in platforms):
                raise ValueError('Dependency uses a non-macOS Mach-O platform')
            minimums.extend(re.findall(r'\n\s+minos ([\d.]+)', block))
        if re.search(r'\bcmd LC_VERSION_MIN_MACOSX\b', block):
            minimums.extend(re.findall(r'\n\s+version ([\d.]+)', block))
    dependencies = []
    for line in command('/usr/bin/otool', '-L', str(path)).splitlines():
        match = re.match(r'\s+(.+?) \(compatibility version ', line)
        if match and match[1] not in identities:
            dependencies.append(match[1])
    arches = command('/usr/bin/lipo', '-archs', str(path)).split()
    return dependencies, rpaths, max(minimums, key=version, default=None), arches

def expand(path, loader, executable):
    return path.replace('@loader_path', str(loader.parent)).replace('@executable_path', str(executable.parent))

def audit(bundle):
    bundle = bundle.resolve(strict=True)
    with (bundle / 'Contents/Info.plist').open('rb') as file:
        info = plistlib.load(file)
    executable = (bundle / 'Contents/MacOS' / info['CFBundleExecutable']).resolve(strict=True)
    if not inside(executable, bundle):
        raise ValueError('Bundle executable escapes the selected bundle')
    declared = info.get('LSMinimumSystemVersion')
    if not declared or not re.fullmatch(r'\d+(?:\.\d+){0,2}', declared):
        raise ValueError('Bundle must declare its minimum macOS version')
    cache, visited, images, issues = {}, set(), {}, []
    edges = []
    queue = [(executable, ())]
    required_arches = set(metadata(executable)[3])
    while queue:
        path, inherited = queue.pop(0)
        key = (str(path), inherited)
        if key in visited:
            continue
        visited.add(key)
        if len(visited) > 4096 or len(images) > 256:
            raise ValueError('Dependency graph exceeds supported audit bounds')
        if path not in cache:
            cache[path] = metadata(path)
        dependencies, rpaths, minimum, arches = cache[path]
        if path not in images:
            images[path] = {'path': str(path), 'minimum_os': minimum, 'architectures': arches,
                            'inside_bundle': inside(path, bundle)}
            if not inside(path, bundle):
                issues.append({'kind': 'external_sdk_dependency', 'path': str(path)})
            if not minimum:
                issues.append({'kind': 'unknown_minimum_os', 'path': str(path)})
            if minimum and version(minimum) > version(declared):
                issues.append({'kind': 'minimum_os_mismatch', 'path': str(path),
                               'required': minimum, 'declared': declared})
            if not required_arches.issubset(arches):
                issues.append({'kind': 'missing_architecture', 'path': str(path)})
        search = tuple(dict.fromkeys([expand(r, path, executable) for r in rpaths] + list(inherited)))
        for dependency in dependencies:
            if system(dependency):
                continue
            if dependency.startswith('@rpath/'):
                candidates = [Path(r) / dependency[len('@rpath/'):] for r in search if not r.startswith('@')]
            else:
                candidates = [Path(expand(dependency, path, executable))]
            resolved = None
            for candidate in candidates:
                if system(candidate):
                    resolved = candidate  # Apple libraries can live only in the dyld cache.
                    break
                if candidate.is_absolute() and candidate.is_file():
                    resolved = candidate.resolve(strict=True)
                    break
            if resolved is None:
                issues.append({'kind': 'unresolved_dependency', 'image': str(path), 'name': dependency})
            elif not system(resolved):
                edges.append({"image": str(path), "name": dependency, "resolved": str(resolved)})
                queue.append((resolved, search))
    floor = max([declared] + [row['minimum_os'] for row in images.values() if row['minimum_os']], key=version)
    return {'bundle': str(bundle), 'declared_minimum_os': declared, 'observed_minimum_os': floor,
            'standalone_runtime_ready': not issues, 'images': sorted(images.values(), key=lambda row: row['path']),
            'issues': issues, 'edges': edges,
            'scope': 'Mach-O linkage only; excludes notices, runtime-loaded plugins/TLS trust, signing, notarization and physical older-OS execution'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('bundle', type=Path)
    args = parser.parse_args()
    try:
        report = audit(args.bundle)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(json.dumps({'audit_error': str(error)}))
        return 2
    print(json.dumps(report, indent=2))
    return 0 if report['standalone_runtime_ready'] else 1

if __name__ == '__main__':
    sys.exit(main())
