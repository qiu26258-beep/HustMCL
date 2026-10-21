"""Check the core's canonical includes; the CMake check separately guards link dependencies.

This is a dependency guard, not proof that arbitrary C++ code has no side effects.
Keep the small allowlist explicit when a core use case needs another value-only header.
"""

from pathlib import Path
import re
import sys

STANDARD_HEADERS = {
    "algorithm", "array", "bit", "cstddef", "cstdint", "filesystem", "functional",
    "limits", "map", "memory", "optional", "set", "span", "string", "string_view",
    "tuple", "type_traits", "unordered_map", "unordered_set", "utility", "variant", "vector",
}
QT_VALUE_HEADERS = {
    "QByteArray", "QJsonArray", "QJsonDocument", "QJsonObject", "QJsonParseError",
    "QJsonValue", "QList", "QMap", "QString", "QStringList", "QStringView", "QVector",
}


def violations(root: Path) -> list[str]:
    errors = []
    for path in sorted(root.rglob("*")):
        if path.suffix not in {".h", ".hpp", ".cpp"}:
            continue
        source = path.read_text(encoding="utf-8-sig")
        source = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
        for line in source.splitlines():
            if not re.match(r"\s*#\s*include\b", line):
                continue
            match = re.fullmatch(r'\s*#\s*include\s*[<"]([^>"]+)[>"]\s*', line)
            header = match.group(1) if match else "<macro or malformed include>"
            own = header.startswith("hustmcl/core/") and ".." not in header.split("/")
            qt = header.startswith("QtCore/") and header.removeprefix("QtCore/") in QT_VALUE_HEADERS
            if not (own or qt or header in STANDARD_HEADERS):
                errors.append(f"{path.relative_to(root)}: forbidden core include: {header}")
    return errors


if __name__ == "__main__":
    root = Path(sys.argv[1])
    if not root.is_dir() or not any(root.rglob("*.cpp")):
        sys.exit("Expected a nonempty core source directory")
    errors = violations(root)
    if errors:
        sys.exit("\n".join(errors))
    print("PASS core include boundaries")
