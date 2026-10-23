"""Build a local trial installer from an explicitly verified GUI package.

Only manifest-listed release files enter a fresh, private build snapshot.
No game downloads, account reads, installation or cleanup is performed here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import struct
import subprocess


ROOT_FILES = {"hustmcl_desktop.exe", "hustmcl.ico", "使用说明.txt"}
ROOT_DLLS = {"D3Dcompiler_47.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll",
             "libwinpthread-1.dll", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Network.dll", "Qt6Widgets.dll"}
DIRECTORIES = {"runtime", "platforms", "imageformats", "networkinformation", "styles",
               "tls", "generic", "THIRD-PARTY-NOTICES"}
OMITTED = {"创建桌面快捷方式.vbs"}
REQUIRED = {"hustmcl_desktop.exe", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll",
            "Qt6Network.dll", "platforms/qwindows.dll", "runtime/bin/java.exe"}
REPARSE = 0x400
CHANNELS = {
    "trial": (),
    "market": ("HustMCL.MarketTrial", "HustMCL Market Trial", "HustMCL-Market", "HustMCL Market"),
    "modpacks": ("HustMCL.ModpackTrial", "HustMCL Modpacks", "HustMCL-Modpacks", "HustMCL Modpacks"),
    "public": ("HustMCL.PublicPreview", "HustMCL Preview", "HustMCL-Preview", "HustMCL Preview"),
}


def digest(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(block)
    return sha.hexdigest()


def regular_path(path: Path) -> None:
    # Check every parent, including the caller's package directory.
    for item in (path, *path.parents):
        info = item.lstat()
        if item.is_symlink() or getattr(info, "st_file_attributes", 0) & REPARSE:
            raise ValueError(f"Reparse points are not release inputs: {item}")
    if not path.is_file():
        raise ValueError(f"Not a regular file: {path}")


def release_path(value: str) -> str:
    if not isinstance(value, str) or not value or "\\" in value:
        raise ValueError("Manifest paths must use relative forward slashes")
    parts = value.split("/")
    if any(part in {"", ".", ".."} or part[-1:] in {".", " "} for part in parts):
        raise ValueError(f"Noncanonical manifest path: {value}")
    if any(ord(c) < 32 or c in ':*?"<>|{};' for c in value):
        raise ValueError(f"Unsafe manifest path: {value}")
    for part in parts:
        if part.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *(
                f"{prefix}{n}" for prefix in ("COM", "LPT") for n in range(1, 10))}:
            raise ValueError(f"Reserved Windows path: {value}")
    if value in ROOT_FILES | ROOT_DLLS | OMITTED:
        return value
    if len(parts) > 1 and parts[0] in DIRECTORIES:
        return value
    if len(parts) > 2 and parts[0] == "data" and parts[1] in {"assets", "libraries", "versions"}:
        return value
    raise ValueError(f"File is outside the release allowlist: {value}")


def validate_payload(package: Path) -> list[dict]:
    manifest = package / "FILES.sha256.json"
    regular_path(manifest)
    records = json.loads(manifest.read_text(encoding="utf-8-sig"))
    if not isinstance(records, list) or not records or len(records) > 100_000:
        raise ValueError("Invalid manifest list")
    seen: set[str] = set()
    result = []
    for record in records:
        if not isinstance(record, dict):
            raise ValueError("Invalid manifest entry")
        relative = release_path(record.get("path"))
        key = relative.casefold()
        if key in seen or any(key.startswith(prior + "/") or prior.startswith(key + "/") for prior in seen):
            raise ValueError(f"Duplicate or conflicting manifest path: {relative}")
        seen.add(key)
        size, sha = record.get("size"), record.get("sha256")
        if type(size) is not int or size < 0 or not isinstance(sha, str) or not re.fullmatch(r"[a-fA-F0-9]{64}", sha):
            raise ValueError(f"Invalid size/digest: {relative}")
        source = package / relative
        regular_path(source)
        if source.stat().st_size != size or digest(source) != sha.lower():
            raise ValueError(f"Payload checksum mismatch: {relative}")
        if relative not in OMITTED:
            result.append({"path": relative, "size": size, "sha256": sha.lower()})
    paths = {entry["path"] for entry in result}
    if not REQUIRED <= paths or not any(p.startswith("THIRD-PARTY-NOTICES/") for p in paths):
        raise ValueError("Required executable, runtime or notices missing")
    if not all(any(p.startswith("data/" + folder + "/") for p in paths) for folder in ("assets", "libraries", "versions")):
        raise ValueError("Game payload is incomplete")
    return sorted(result, key=lambda record: record["path"].casefold())


def write_inputs(package: Path, output: Path, records: list[dict], overrides: dict[str, Path] | None = None) -> Path:
    """Snapshot validated bytes, then generate exact compiler inputs."""
    output.mkdir(parents=True, exist_ok=False)
    snapshot = output / "payload"
    lines = []
    for record in records:
        relative = record["path"]
        source, target = (overrides or {}).get(relative, package / relative), snapshot / relative
        regular_path(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        if target.stat().st_size != record["size"] or digest(target) != record["sha256"]:
            raise ValueError(f"Payload changed during snapshot: {relative}")
        parent = str(PurePosixPath(relative).parent).replace("/", "\\")
        destination = "{app}" + ("\\" + parent if parent != "." else "")
        lines.append(f'Source: "{target}"; DestDir: "{destination}"; Flags: ignoreversion; Check: NeedsPayload')
    (output / "files.iss").write_text("\n".join(lines) + "\n", encoding="utf-8-sig")
    manifest = output / "payload.sha256"
    # ASCII hash followed by UTF-8 path. Inno's LoadStringsFromFile handles BOM.
    manifest.write_text("\n".join(f'{r["sha256"]} {r["path"]}' for r in records) + "\n", encoding="utf-8-sig")
    return manifest


def launcher_records(records: list[dict], launcher: Path) -> list[dict]:
    regular_path(launcher)
    with launcher.open("rb") as stream:
        header = stream.read(64)
        if len(header) != 64 or header[:2] != b"MZ":
            raise ValueError("Launcher is not a Windows executable")
        offset = struct.unpack_from("<I", header, 60)[0]
        if offset > 1024 * 1024:
            raise ValueError("Invalid launcher PE header")
        stream.seek(offset)
        pe = stream.read(6)
        if pe[:4] != b"PE\0\0" or len(pe) != 6 or struct.unpack_from("<H", pe, 4)[0] != 0x8664:
            raise ValueError("Launcher must target Windows x64")
    return [dict(record, size=launcher.stat().st_size, sha256=digest(launcher))
            if record["path"] == "hustmcl_desktop.exe" else dict(record) for record in records]


def build(package: Path, output: Path, compiler: Path, script: Path, launcher: Path | None = None, channel: str = "trial") -> Path:
    for path in (package, output, compiler, script):
        if any(c in str(path) for c in '\r\n"{};'):
            raise ValueError("Unsupported compiler input path")
    records = validate_payload(package)
    overrides = {}
    if launcher:
        records = launcher_records(records, launcher)
        overrides["hustmcl_desktop.exe"] = launcher
    if channel not in CHANNELS:
        raise ValueError("Unknown installer channel")
    if channel == "public":
        if launcher is None:
            raise ValueError("Public distribution requires an explicit public launcher")
        if "0.1.0-public-preview".encode("utf-16le") not in launcher.read_bytes():
            raise ValueError("Launcher was not built for public distribution")
        # No Minecraft payload or bundled JRE. Java is acquired per instance.
        records = [r for r in records if not r["path"].startswith(("data/", "runtime/"))
                   and r["path"] not in {"使用说明.txt", "THIRD-PARTY-NOTICES/README.txt"}]
        for relative, name in (("使用说明.txt", "public-readme.txt"),
                               ("THIRD-PARTY-NOTICES/README.txt", "public-notices.txt")):
            source = Path(__file__).resolve().parent / name
            regular_path(source)
            records.append({"path": relative, "size": source.stat().st_size, "sha256": digest(source)})
            overrides[relative] = source
    # The new launcher statically links miniz. Include its exact upstream
    # license in this private snapshot without modifying the verified source.
    license_source = Path(__file__).resolve().parent.parent / "third_party/miniz/LICENSE"
    regular_path(license_source)
    license_path = "THIRD-PARTY-NOTICES/miniz-LICENSE.txt"
    records = [record for record in records if record["path"].casefold() != license_path.casefold()]
    records.append({"path": license_path, "size": license_source.stat().st_size, "sha256": digest(license_source)})
    overrides[license_path] = license_source
    manifest = write_inputs(package, output, records, overrides)
    command = [str(compiler), f"/DBuildInputs={output}", f"/DPackageId={digest(manifest)}", str(script)]
    if CHANNELS[channel]:
        command[1:1] = [f"/D{key}={value}" for key, value in zip(
            ("InstallId", "AppDisplayName", "InstallDirName", "ShortcutName"), CHANNELS[channel])]
    with (output / "compiler.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    exe = output / "HustMCL-Setup-Windows-x64.exe"
    sha = digest(exe)
    (output / (exe.name + ".sha256")).write_text(f"{sha}  {exe.name}\n", encoding="ascii")
    (output / "build.json").write_text(json.dumps({"fileCount": len(records), "payloadBytes": sum(r["size"] for r in records),
        "packageId": digest(manifest), "installerSha256": sha, "channel": channel}, indent=2), encoding="utf-8")
    return exe


def main() -> None:
    workspace = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, default=workspace / "out/gui-26.2-staging/HustMCL-GUI-26.2")
    parser.add_argument("--output", type=Path, required=True, help="New directory; existing outputs are never overwritten")
    parser.add_argument("--compiler", type=Path, default=workspace / ".deps/InnoSetup/ISCC.exe")
    parser.add_argument("--launcher", type=Path, help="Replace only the verified package launcher in the private snapshot")
    parser.add_argument("--channel", choices=tuple(CHANNELS), default="trial")
    args = parser.parse_args()
    # absolute() preserves symlink components so regular_path can reject them.
    print(build(args.package.absolute(), args.output.absolute(), args.compiler.absolute(), workspace / "tools/hustmcl_setup.iss",
                args.launcher.absolute() if args.launcher else None, args.channel))


if __name__ == "__main__":
    main()
