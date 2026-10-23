import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("package_setup", ROOT / "tools/package_setup.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)


def fixture(root):
    files = setup.REQUIRED | {"THIRD-PARTY-NOTICES/README.txt", "data/assets/index.json",
                              "data/libraries/lib.jar", "data/versions/version.json"}
    records = []
    for relative in files:
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        data = relative.encode("utf-8")
        path.write_bytes(data)
        records.append({"path": relative, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    (root / "FILES.sha256.json").write_text(json.dumps(records), encoding="utf-8")
    return records


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.package = self.root / "source"
        self.package.mkdir()
        self.records = fixture(self.package)

    def save(self):
        (self.package / "FILES.sha256.json").write_text(json.dumps(self.records), encoding="utf-8")

    def test_snapshot_ignores_unlisted_personal_data(self):
        personal = self.package / "data/game/saves/world/level.dat"
        personal.parent.mkdir(parents=True)
        personal.write_text("private world")
        records = setup.validate_payload(self.package)
        target = self.root / "snapshot"
        setup.write_inputs(self.package, target, records)
        self.assertFalse((target / "payload/data/game").exists())
        self.assertEqual(setup.digest(target / "payload/hustmcl_desktop.exe"), setup.digest(self.package / "hustmcl_desktop.exe"))
        self.assertNotIn('Source: "*', (target / "files.iss").read_text(encoding="utf-8-sig"))
        with self.assertRaises(FileExistsError):
            setup.write_inputs(self.package, target, records)

    def test_bad_paths_and_personal_paths_rejected(self):
        for path in ("../bad", "data/../bad", "/runtime/java", "C:/bad", "runtime\\java", "runtime//java",
                     "runtime/NUL.txt", "runtime/foo.", "runtime/foo ", "runtime/{app}", 'runtime/a"b',
                     "data/game/saves/world/level.dat", "data/launcher-logs/log.json", "unknown.txt"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                setup.release_path(path)

    def test_case_insensitive_duplicate_rejected(self):
        duplicate = dict(next(r for r in self.records if r["path"] == "THIRD-PARTY-NOTICES/README.txt"))
        duplicate["path"] = "THIRD-PARTY-NOTICES/readme.TXT"
        self.records.append(duplicate)
        self.save()
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            setup.validate_payload(self.package)

    def test_checksum_mismatch_rejected(self):
        path = self.package / self.records[0]["path"]
        path.write_bytes(b"modified")
        with self.assertRaisesRegex(ValueError, "checksum"):
            setup.validate_payload(self.package)

    def test_missing_file_rejected(self):
        path = self.package / self.records[0]["path"]
        path.rename(self.root / "missing-file-backup")
        with self.assertRaises(FileNotFoundError):
            setup.validate_payload(self.package)

    def test_invalid_size_or_digest_rejected(self):
        original = dict(self.records[0])
        for field, value in (("size", True), ("size", -1), ("sha256", "not-a-hash")):
            self.records[0] = dict(original, **{field: value})
            self.save()
            with self.subTest(field=field, value=value), self.assertRaisesRegex(ValueError, "Invalid size/digest"):
                setup.validate_payload(self.package)

    def test_changed_between_validation_and_snapshot_rejected(self):
        records = setup.validate_payload(self.package)
        (self.package / records[0]["path"]).write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "changed during snapshot"):
            setup.write_inputs(self.package, self.root / "snapshot", records)

    def test_required_file_missing_from_manifest_rejected(self):
        self.records = [r for r in self.records if r["path"] != "runtime/bin/java.exe"]
        self.save()
        with self.assertRaisesRegex(ValueError, "Required"):
            setup.validate_payload(self.package)

    def test_new_launcher_only_replaces_private_snapshot(self):
        import struct
        launcher = self.root / "new-launcher.exe"
        body = bytearray(256)
        body[:2] = b"MZ"
        struct.pack_into("<I", body, 60, 128)
        body[128:132] = b"PE\0\0"
        struct.pack_into("<H", body, 132, 0x8664)
        launcher.write_bytes(body)
        records = setup.launcher_records(setup.validate_payload(self.package), launcher)
        target = self.root / "snapshot"
        original = (self.package / "hustmcl_desktop.exe").read_bytes()
        setup.write_inputs(self.package, target, records, {"hustmcl_desktop.exe": launcher})
        self.assertEqual((target / "payload/hustmcl_desktop.exe").read_bytes(), body)
        self.assertEqual((self.package / "hustmcl_desktop.exe").read_bytes(), original)
        struct.pack_into("<H", body, 132, 0xAA64)
        launcher.write_bytes(body)
        with self.assertRaisesRegex(ValueError, "x64"):
            setup.launcher_records(records, launcher)

    def test_public_package_excludes_game_and_java_and_requires_public_build(self):
        import struct
        from unittest.mock import patch
        launcher = self.root / "public.exe"
        body = bytearray(256)
        body[:2] = b"MZ"
        struct.pack_into("<I", body, 60, 128)
        body[128:132] = b"PE\0\0"
        struct.pack_into("<H", body, 132, 0x8664)
        launcher.write_bytes(body)
        output = self.root / "public-build"
        with self.assertRaisesRegex(ValueError, "not built for public"):
            setup.build(self.package, output, self.root / "compiler", ROOT / "tools/hustmcl_setup.iss", launcher, "public")
        launcher.write_bytes(body + "0.1.0-public-preview".encode("utf-16le"))
        def compile_fixture(command, **kwargs):
            self.assertIn("/DInstallId=HustMCL.PublicPreview", command)
            (output / "HustMCL-Setup-Windows-x64.exe").write_bytes(b"fixture")
        with patch.object(setup.subprocess, "run", side_effect=compile_fixture):
            setup.build(self.package, output, self.root / "compiler", ROOT / "tools/hustmcl_setup.iss", launcher, "public")
        self.assertFalse((output / "payload/data").exists())
        self.assertFalse((output / "payload/runtime").exists())
        self.assertTrue((output / "payload/Qt6Core.dll").exists())
        self.assertEqual((output / "payload/使用说明.txt").read_bytes(), (ROOT / "tools/public-readme.txt").read_bytes())

    def test_junction_parent_rejected(self):
        import subprocess
        # Junction creation needs no symbolic-link privilege on Windows.
        real = self.root / "real-runtime"
        (self.package / "runtime").rename(real)
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(self.package / "runtime"), str(real)], capture_output=True)
        if result.returncode:
            self.fail("Cannot create junction fixture")
        with self.assertRaisesRegex(ValueError, "Reparse"):
            setup.validate_payload(self.package)

    def test_modpack_build_is_isolated_and_includes_miniz_license(self):
        from unittest.mock import patch
        output = self.root / "modpack-build"
        original_manifest = (self.package / "FILES.sha256.json").read_bytes()

        def compile_fixture(command, **kwargs):
            self.assertIn("/DInstallId=HustMCL.ModpackTrial", command)
            self.assertIn("/DInstallDirName=HustMCL-Modpacks", command)
            self.assertIn("/DShortcutName=HustMCL Modpacks", command)
            (output / "HustMCL-Setup-Windows-x64.exe").write_bytes(b"fixture installer")

        with patch.object(setup.subprocess, "run", side_effect=compile_fixture):
            setup.build(self.package, output, self.root / "compiler.exe", ROOT / "tools/hustmcl_setup.iss", channel="modpacks")
        notice = output / "payload/THIRD-PARTY-NOTICES/miniz-LICENSE.txt"
        self.assertEqual(notice.read_bytes(), (ROOT / "third_party/miniz/LICENSE").read_bytes())
        self.assertIn(setup.digest(notice) + " THIRD-PARTY-NOTICES/miniz-LICENSE.txt",
                      (output / "payload.sha256").read_text(encoding="utf-8-sig"))
        self.assertEqual((self.package / "FILES.sha256.json").read_bytes(), original_manifest)
        self.assertFalse((self.package / "THIRD-PARTY-NOTICES/miniz-LICENSE.txt").exists())
        self.assertEqual(json.loads((output / "build.json").read_text())["channel"], "modpacks")


if __name__ == "__main__":
    unittest.main()
