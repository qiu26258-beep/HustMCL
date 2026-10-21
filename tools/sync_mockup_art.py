"""Embed the shared JPEG in the two portable SVG review files (stdlib only)."""
from __future__ import annotations

import argparse
import base64
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "resources/mockups/oil-ui/assets/xiaoke-campus.jpg"
IMAGE = re.compile(r'(<image\b[^>]*\bid="xiaoke-art"[^>]*\bhref=")[^"]*(")')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check without writing")
    args = parser.parse_args()
    data = ART.read_bytes()
    if not data.startswith(b"\xff\xd8\xff"):
        raise ValueError(f"Expected a JPEG: {ART}")
    uri = "data:image/jpeg;base64," + base64.b64encode(data).decode("ascii")
    stale = False
    for name in ("home.svg", "install.svg"):
        path = ROOT / "resources/mockups" / name
        text = path.read_text(encoding="utf-8")
        updated, count = IMAGE.subn(lambda match: match[1] + uri + match[2], text)
        if count != 1:
            raise ValueError(f"Expected one xiaoke-art image in {path}, found {count}")
        if text != updated:
            if args.check:
                print(f"STALE {name}: run python tools/sync_mockup_art.py")
                stale = True
            else:
                path.write_text(updated, encoding="utf-8", newline="\n")
                print(f"UPDATED {name}")
        else:
            print(f"OK {name}")
    return int(stale)


if __name__ == "__main__":
    raise SystemExit(main())
