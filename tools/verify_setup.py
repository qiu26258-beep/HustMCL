"""Real installer lifecycle smoke test using a tiny, isolated payload.

Artifacts stay under a new workspace out directory. Only the test installer
uninstalls its own fixture. No production shortcuts/registry IDs are used.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import uuid

from package_setup import digest, write_inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--market-channel", action="store_true")
    parser.add_argument("--skip-interactive", action="store_true", help="Run lifecycle checks without opening an installer window")
    args = parser.parse_args()
    root = args.output.absolute()
    root.mkdir(parents=True, exist_ok=False)
    workspace = Path(__file__).resolve().parent.parent
    compiler_bin = workspace / ".deps/Qt/Tools/mingw1310_64/bin"
    os.environ["PATH"] = str(compiler_bin) + os.pathsep + os.environ.get("PATH", "")
    source = root / "source"
    source.mkdir()
    # Fixture process records launcher invocation. It never starts Minecraft.
    cpp = root / "launcher.cpp"
    cpp.write_text('#include <fstream>\nint main() { std::ofstream("launcher-opened.txt") << "opened"; }\n')
    subprocess.run([str(compiler_bin / "g++.exe"), "-static", "-mwindows",
                    str(cpp), "-o", str(source / "hustmcl_desktop.exe")], check=True)
    (source / "component.dll").write_bytes(b"test component")
    records = [{"path": p.name, "size": p.stat().st_size, "sha256": digest(p)} for p in source.iterdir()]
    inputs = root / "build"
    manifest = write_inputs(source, inputs, records)
    install_id = "HustMCL.InstallerTest." + uuid.uuid4().hex
    links = root / "shortcuts"
    command = [str(workspace / ".deps/InnoSetup/ISCC.exe"), "/Q", f"/DBuildInputs={inputs}",
                    f"/DPackageId={digest(manifest)}", f"/DInstallId={install_id}",
                    f"/DShortcutRoot={links}", str(workspace / "tools/hustmcl_setup.iss")]
    shortcut_name = "HustMCL Market" if args.market_channel else "HustMCL Trial"
    if args.market_channel:
        command[1:1] = ["/DAppDisplayName=HustMCL Market Trial", "/DInstallDirName=HustMCL-Market",
                        f"/DShortcutName={shortcut_name}"]
    subprocess.run(command, check=True)
    installer = inputs / "HustMCL-Setup-Windows-x64.exe"
    destination = root / "installed"
    common = ["/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART"]

    def run(name, target=destination, expected=0, silent=True):
        switches = common if silent else ["/SUPPRESSMSGBOXES", "/NORESTART"]
        result = subprocess.run([str(installer), *switches, f"/DIR={target}", f"/LOG={root / (name + '.log')}"])
        assert result.returncode == expected, (name, result.returncode, expected)

    unknown = root / "unknown"
    unknown.mkdir()
    (unknown / "keep.txt").write_text("unrelated")
    run("unknown", unknown, expected=7)
    assert (unknown / "keep.txt").read_text() == "unrelated"
    run("first")
    original = (destination / "hustmcl_desktop.exe").stat().st_mtime_ns
    world = destination / "data/game/saves/retained-world/level.dat"
    world.parent.mkdir(parents=True)
    world.write_bytes(b"personal data must survive")
    (destination / "launcher-opened.txt").write_text("not yet")
    shortcut = links / "desktop" / (shortcut_name + ".lnk")
    assert shortcut.is_file()
    backup = root / "saved-shortcut.lnk"
    shortcut.rename(backup)
    run("repeat")
    assert shortcut.is_file(), "Missing shortcut was not restored"
    assert (destination / "hustmcl_desktop.exe").stat().st_mtime_ns == original, "Repeat overwrote payload"
    assert (destination / "launcher-opened.txt").read_text() == "not yet", "Silent install opened launcher"
    marker = destination / ".hustmcl-package"
    marker.rename(root / "legacy-marker-backup")
    run("legacy-adoption")
    assert marker.exists(), "Registered legacy installation was not adopted"
    assert (destination / "hustmcl_desktop.exe").stat().st_mtime_ns == original
    # A registered installation holding another package must be upgraded in
    # place: refusing would strand users whose data directory outlives the
    # uninstaller.
    (destination / "component.dll").write_bytes(b"stale component")
    marker.write_bytes(b"different-package")
    run("different-package")
    assert marker.read_bytes() != b"different-package", "Upgrade kept the foreign marker"
    assert digest(marker) == digest(manifest), "Upgrade did not record the new package"
    assert (destination / "component.dll").read_bytes() == b"test component", \
        "Upgrade left stale payload files"
    assert world.read_bytes() == b"personal data must survive", "Upgrade changed personal data"
    # Interactive default must start launcher, not the game. Wait for file marker below.
    import time
    if not args.skip_interactive:
        run("interactive", silent=False)
        for _ in range(100):
            if (destination / "launcher-opened.txt").read_text() == "opened":
                break
            time.sleep(.05)
        assert (destination / "launcher-opened.txt").read_text() == "opened"
    damaged = destination / "component.dll"
    saved = damaged.read_bytes()
    damaged.write_bytes(b"damaged")
    run("damaged")
    assert damaged.read_bytes() == saved, "Damaged installation was not repaired"
    saved_link = shortcut.read_bytes()
    shortcut.write_bytes(b"foreign shortcut")
    run("shortcut-conflict", expected=7)
    assert shortcut.read_bytes() == b"foreign shortcut"
    shortcut.write_bytes(saved_link)
    before = digest(world)
    uninstaller = destination / "unins000.exe"
    result = subprocess.run([str(uninstaller), *common, f"/LOG={root / 'uninstall.log'}"])
    assert result.returncode == 0, result.returncode
    # Uninstaller may delegate to a temporary process; observe completion.
    for _ in range(200):
        if not (destination / "hustmcl_desktop.exe").exists():
            break
        time.sleep(.05)
    assert not (destination / "hustmcl_desktop.exe").exists()
    assert not shortcut.exists()
    assert world.exists() and digest(world) == before, "Uninstall changed personal data"
    report = {"status": "passed", "scenarios": ["unknown-directory", "first-install", "repeat-no-payload-writes",
        "shortcut-recovery", "registered-legacy-adoption", "different-package-upgrade",
        "silent-no-launch", "damaged-install-repaired",
        "foreign-shortcut-preserved", "uninstall-preserves-personal-data"], "installId": install_id,
        "interactiveTested": not args.skip_interactive, "marketChannel": args.market_channel}
    if not args.skip_interactive:
        report["scenarios"].append("interactive-opens-launcher")
    (root / "result.json").write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == "__main__":
    main()
