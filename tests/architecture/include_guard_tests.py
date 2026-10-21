from pathlib import Path
import importlib.util
from tempfile import TemporaryDirectory
import unittest

spec = importlib.util.spec_from_file_location(
    "check_core_includes", Path(__file__).resolve().parents[2] / "tools/check_core_includes.py")
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)


class IncludeGuardTests(unittest.TestCase):
    def check_source(self, source):
        # All temporary inputs stay in the build directory, never in user data.
        with TemporaryDirectory(prefix="include-guard-", dir=Path.cwd()) as directory:
            root = Path(directory)
            (root / "sample.cpp").write_text(source, encoding="utf-8")
            return guard.violations(root)

    def test_allowed_value_includes(self):
        self.assertEqual([], self.check_source(
            '#include <optional>\n#include <hustmcl/core/java_runtime.h>\n#include <QtCore/QString>\n'))

    def test_reject_external_and_side_effect_dependencies(self):
        for header in ("QWidget", "QtWidgets/QWidget", "QtNetwork/QNetworkAccessManager", "windows.h",
                       "fstream", "QtCore/QFile", "hustmcl/application/install.h", "../ui/window.h"):
            with self.subTest(header=header):
                self.assertTrue(self.check_source(f'#include <{header}>\n'))

    def test_comments_are_not_dependencies(self):
        self.assertEqual([], self.check_source('/*\n#include <QWidget>\n*/\n// #include <windows.h>\n'))

    def test_macro_include_cannot_hide_dependency(self):
        self.assertTrue(self.check_source('#define HEADER <QWidget>\n#include HEADER\n'))


if __name__ == "__main__":
    unittest.main()
