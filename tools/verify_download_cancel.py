"""Windows integration probe: hidden CLI, real Ctrl+Break, cooperative cleanup.

Requires network access to the pinned official sources. No game is started.
"""
import ctypes
import json
import pathlib
import subprocess
import sys
import tempfile
import time


def main():
    repo = pathlib.Path(__file__).resolve().parents[1]
    executable = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else repo / "build/backend-release/src/app/hustmcl_batch_probe.exe"
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    with tempfile.TemporaryDirectory(prefix="hustmcl-cancel-") as directory:
        root = pathlib.Path(directory)
        process = subprocess.Popen([
            str(executable), str(root), str(repo / "tests/fixtures/metadata/1.21.1.json"),
            "1757c8b1fbfc6b638504e1f6cceb05893f7389d5", str(repo / "tests/fixtures/metadata/17.json"),
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, startupinfo=startup,
            creationflags=subprocess.CREATE_NEW_CONSOLE | subprocess.CREATE_NEW_PROCESS_GROUP)
        try:
            deadline = time.monotonic() + 10
            while not (root / ".write.lock").exists():
                if process.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("CLI did not acquire its batch lock")
                time.sleep(0.02)
            time.sleep(0.2)
            kernel.FreeConsole()
            if not kernel.AttachConsole(process.pid):
                raise ctypes.WinError(ctypes.get_last_error())
            try:
                if not kernel.GenerateConsoleCtrlEvent(1, process.pid):
                    raise ctypes.WinError(ctypes.get_last_error())
            finally:
                kernel.FreeConsole()
            stdout, stderr = process.communicate(timeout=15)
            result = json.loads(stdout)
            assert process.returncode == 4 and result["status"] == "cancelled", (process.returncode, stdout, stderr)
            assert not (root / ".write.lock").exists()
            assert not list((root / "staging").iterdir())
            print(json.dumps({"ctrlBreak": "passed", "exitCode": 4, "stagingEmpty": True, "lockReleased": True}))
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()


if __name__ == "__main__":
    main()
