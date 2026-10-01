#!/usr/bin/env python3
"""Check generated documentation and SDK metadata after the docs target.

Copyright (C) 2026 Qore Technologies, s.r.o.
"""
import json
from pathlib import Path
import sys
import unittest

BUILD = Path(sys.argv[1] if len(sys.argv) > 1 else "build")


class MagicDocumentationTest(unittest.TestCase):
    def test_hashdecl_namespace_matches_runtime_registration(self):
        metadata = json.loads((BUILD / "magic.meta.json").read_text())
        declarations = [item for item in metadata["hashdecls"] if item["name"] == "MagicFileInfo"]
        self.assertEqual(1, len(declarations))
        self.assertEqual("Qore::Magic", declarations[0]["namespace_path"])

    def test_release_notes_are_generated_and_linked(self):
        html = BUILD / "docs/magic/html"
        notes = (html / "magicreleasenotes.html").read_text()
        index = (html / "index.html").read_text()
        self.assertIn('id="magic_2_0_0"', notes)
        self.assertIn('href="magicreleasenotes.html"', index)
        self.assertIn("structQore_1_1Magic_1_1MagicFileInfo.html", notes)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
