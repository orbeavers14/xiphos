# SPDX-License-Identifier: GPL-2.0-or-later
import importlib.util, tempfile, unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('catalogs',Path(__file__).with_name('build-native-catalogs.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class CatalogImport(unittest.TestCase):
    def test_exact_unambiguous_nonfuzzy_singular_only(self):
        text='''msgid "Long "
"message"
msgstr "Longue "
"phrase"

#, fuzzy
msgid "Draft"
msgstr "Brouillon"

msgid "item"
msgid_plural "items"
msgstr[0] "article"
msgstr[1] "articles"

msgctxt "first"
msgid "Same"
msgstr "Un"

msgctxt "second"
msgid "Same"
msgstr "Deux"
'''
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'fixture.po';path.write_text(text)
            self.assertEqual(module.catalog(path),{'Long message':'Longue phrase'})
    def test_outputs_deterministic(self):
        self.assertEqual(module.outputs(),module.outputs())
if __name__=='__main__':unittest.main()
