"""P0-01 module wiring checks; the full boundary gate belongs to P0-02."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ModuleLayoutTests(unittest.TestCase):
    def test_only_ue_host_is_runtime_module(self):
        project = json.loads((ROOT / "QiantongCore.uproject").read_text(encoding="utf-8-sig"))
        self.assertEqual([m["Name"] for m in project["Modules"]], ["QiantongUE"])
        for target in (ROOT / "Source").glob("*.Target.cs"):
            self.assertIn('ExtraModuleNames.Add("QiantongUE")', target.read_text())

    def test_core_is_external_library_without_ue_dependencies(self):
        rules = (ROOT / "Source/QiantongCore/QiantongCore.Build.cs").read_text()
        self.assertIn("ModuleType.External", rules)
        self.assertNotIn("DependencyModuleNames", rules)
        self.assertFalse((ROOT / "Source/QiantongCore/QiantongCore.cpp").exists())

    def test_host_owns_registration_and_links_core(self):
        rules = (ROOT / "Source/QiantongUE/QiantongUE.Build.cs").read_text()
        self.assertIn('"QiantongCore"', rules)
        source = (ROOT / "Source/QiantongUE/Private/QiantongUE.cpp").read_text()
        self.assertIn("IMPLEMENT_PRIMARY_GAME_MODULE", source)
        self.assertIn("Qiantong::CoreVersion()", source)


if __name__ == "__main__":
    unittest.main()
