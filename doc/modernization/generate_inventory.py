#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate disposable source evidence appendices under ignored build/."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[2]
out = root / 'build/modernization'
out.mkdir(parents=True, exist_ok=True)
lines = ['source\twidget_class\twidget_id\tlabel']
for file in sorted((root / 'ui').glob('*.gtkbuilder')):
    for obj in ET.parse(file).iter('object'):
        for prop in obj.findall('property'):
            if prop.get('name') in ('label', 'title') and prop.text:
                lines.append('\t'.join((str(file.relative_to(root)),
                    obj.get('class', ''), obj.get('id', ''),
                    ' '.join(prop.text.split()) or '(empty)')))
(out / 'ui-command-inventory.tsv').write_text('\n'.join(lines) + '\n')
lines = ['source\tsection_id\ttitle']
ns = {'m': 'http://projectmallard.org/1.0/'}
for file in sorted((root / 'help/C').glob('*.page')):
    for obj in ET.parse(file).iter():
        if obj.tag in ('{' + ns['m'] + '}page', '{' + ns['m'] + '}section'):
            title = obj.find('m:title', ns)
            if title is not None:
                lines.append('\t'.join((str(file.relative_to(root)),
                    obj.get('id', ''), ' '.join(''.join(title.itertext()).split()))))
(out / 'manual-section-inventory.tsv').write_text('\n'.join(lines) + '\n')
