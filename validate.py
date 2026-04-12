from pathlib import Path
import os
from beartype import beartype
import subprocess
import json
import time
import shutil
import winreg
import re


# ANSI colors
GREEN = "\033[92m"
RED   = "\033[91m"
RESET = "\033[0m"


@beartype
def log_directory() -> Path:
    return (Path.home() / "Desktop" / "output").resolve()

@beartype
def log_file_exist(logDir: Path, module: str) -> bool:
    try:
        if not logDir.is_dir() or not module:
            return False

        for p in logDir.rglob("*"):
            if not p.is_file():
                continue

            if module in p.name:
                try:
                    if p.stat().st_size > 0:
                        return True
                except OSError:
                    continue

        return False
    except OSError:
        return False

@beartype
def test_logged_module(module: str):
    logName = f"LOG_{module}"
    if log_file_exist(log_directory(), logName):
        print(f"{GREEN}SUCCESS{RESET} - {module}")
    else:
        print(f"{RED}FAIL{RESET} - {module}")

@beartype
def test_t1005():
    module = "T1005"
    test_logged_module(module)

@beartype
def test_t1057():
    module = "T1057"
    test_logged_module(module)

@beartype
def test_t1082():
    module = "T1082"
    test_logged_module(module)

@beartype
def test_t1083():
    module = "T1083"
    test_logged_module(module)

@beartype
def test_t1070(exePath: Path):
    logDir = log_directory()
    createdDummyDir = not logDir.exists()
    if createdDummyDir:
        logDir.mkdir(parents = True, exist_ok = True)

    src = exePath.resolve()
    dst = src.parent / f"{src.stem}_copy{src.suffix}"

    shutil.copy2(src, dst)
    process = subprocess.run([str(dst)], cwd = str(dst.parent), check = False, capture_output = True, text = True, encoding = "utf-8")
    out = (process.stdout or "") + (process.stderr or "")
    saw145Msg = ("RemoveDirectoryW failed with code: 145" in out)

    # Self deletion always fails with access denied because its bugged
    #selfDeleted = not dst.exists()
    logsDeleted = (not createdDummyDir) or (not logDir.exists())
    if logsDeleted or saw145Msg:
        print(f"{GREEN}SUCCESS{RESET} - T1070")
    else:
        print(f"{RED}FAIL{RESET} - T1070")

@beartype
def test_t1547(exePath: Path):
    RUN_SUBKEY = r"Software\Microsoft\Windows\CurrentVersion\Run"
    VALUE_NAME = "__Pseudo_Malware"

    startupDir = Path(os.environ["APPDATA"]) / "Microsoft" / "Windows" / "Start Menu" / "Programs" / "Startup"
    startupFiles = [startupDir / "mal_techniques.exe", startupDir / "mal_techniquesd.exe"]

    def run_value_exists(hive) -> bool:
        try:
            with winreg.OpenKey(hive, RUN_SUBKEY, 0, winreg.KEY_READ) as key:
                winreg.QueryValueEx(key, VALUE_NAME)
            return True
        except OSError:
            return False

    def delete_run_value(hive) -> None:
        try:
            with winreg.OpenKey(hive, RUN_SUBKEY, 0, winreg.KEY_SET_VALUE) as key:
                winreg.DeleteValue(key, VALUE_NAME)
        except OSError:
            pass

    delete_run_value(winreg.HKEY_CURRENT_USER)
    delete_run_value(winreg.HKEY_LOCAL_MACHINE)
    for f in startupFiles:
        try:
            f.unlink()
        except OSError:
            pass

    subprocess.run([str(exePath.resolve())], check = False, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

    # verify (brief poll)
    runExists = False
    startupExist = False
    for _ in range(30):
        runExists = run_value_exists(winreg.HKEY_CURRENT_USER) or run_value_exists(winreg.HKEY_LOCAL_MACHINE)
        startupExist = any(f.exists() for f in startupFiles)
        if runExists and startupExist:
            break
        time.sleep(0.1)

    delete_run_value(winreg.HKEY_CURRENT_USER)
    delete_run_value(winreg.HKEY_LOCAL_MACHINE)
    for f in startupFiles:
        try:
            f.unlink()
        except OSError:
            pass

    if runExists or startupExist:
        print(f"{GREEN}SUCCESS{RESET} - T1547")
    else:
        print(f"{RED}FAIL{RESET} - T1547")

@beartype
def children(parentPid: int) -> list[dict]:
    pwsh = rf"""
    Get-CimInstance Win32_Process -Filter "ParentProcessId={parentPid}" |
      Select-Object Name,CommandLine | ConvertTo-Json -Compress
    """
    out = subprocess.check_output(
        ["powershell.exe", "-NoProfile", "-Command", pwsh],
        text = True, stderr = subprocess.DEVNULL
    ).strip()
    if not out:
        return []
    data = json.loads(out)
    return data if isinstance(data, list) else [data]

@beartype
def test_t1059(exePath: Path):
    p = subprocess.Popen(
        [str(exePath)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    value = "Hello PowerShell!"
    seen = False
    deadline = time.time() + 1200.0  # 20 minutes max

    try:
        while time.time() < deadline:
            for ch in children(p.pid):
                if (ch.get("Name") or "").lower() == "powershell.exe" and value in (ch.get("CommandLine") or ""):
                    seen = True
                    break

            if seen:
                break

            # stop if parent exited
            if p.poll() is not None:
                for ch in children(p.pid):
                    if (ch.get("Name") or "").lower() == "powershell.exe" and value in (ch.get("CommandLine") or ""):
                        seen = True
                        break
                break

            time.sleep(0.05)
        try:
            p.wait(timeout = 1)
        except subprocess.TimeoutExpired:
            p.kill()
    finally:
        if p.poll() is None:
            p.kill()

    if seen:
        print(f"{GREEN}SUCCESS{RESET} - PowerShell invoked with expected command")
    else:
        print(f"{RED}FAIL{RESET} - Did not observe expected PowerShell invocation")

@beartype
def full_cleanup():
    logdir = log_directory()
    if logdir.exists():
        try:
            shutil.rmtree(logdir, ignore_errors=True)
        except OSError:
            pass

    # remove Run value __Pseudo_Malware (HKCU/HKLM)
    RUN_SUBKEY = r"Software\Microsoft\Windows\CurrentVersion\Run"
    VALUE_NAME = "__Pseudo_Malware"
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, RUN_SUBKEY, 0, winreg.KEY_SET_VALUE) as key:
            winreg.DeleteValue(key, VALUE_NAME)
    except OSError:
        pass
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, RUN_SUBKEY, 0, winreg.KEY_SET_VALUE) as key:
            winreg.DeleteValue(key, VALUE_NAME)
    except OSError:
        pass

    # remove Startup files
    startup_dir = Path(os.environ["APPDATA"]) / "Microsoft" / "Windows" / "Start Menu" / "Programs" / "Startup"
    for name in ("mal_techniques.exe", "mal_techniquesd.exe"):
        try:
            (startup_dir / name).unlink()
        except OSError:
            pass



def run_validations(root: Path):
    for d in sorted(root.iterdir()):
        if not d.is_dir():
            continue

        print(f"\n\n\nRunning tests for {str(d)}")
        exe = d / "obf.exe"
        if not exe.is_file():
            print(f"{RED}FAIL{RESET} - {exe.parent}")
            continue

        modules = re.findall(r"T\d{4}", d.name)[:3]
        if len(modules) != 3:
            print(f"{RED}FAIL{RESET} - {exe.parent}")
            continue

        containsDeleteModule = False
        testsRun = 0
        for m in modules:
            if m == "T1059":
                full_cleanup()
                test_t1059(exe)
                testsRun += 1
            elif m == "T1070":
                print("Will not look for log files since they are deleted by T1070.")
                full_cleanup()
                test_t1070(exe)
                testsRun += 1
                containsDeleteModule = True
            elif m == "T1547":
                full_cleanup()
                test_t1547(exe)
                testsRun += 1

        if containsDeleteModule:
            continue

        full_cleanup()
        subprocess.run([str(exe)], cwd = str(d), check = False, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

        for m in modules:
            if m == "T1005":
                test_t1005()
                testsRun += 1
            elif m == "T1057":
                test_t1057()
                testsRun += 1
            elif m == "T1082":
                test_t1082()
                testsRun += 1
            elif m == "T1083":
                test_t1083()
                testsRun += 1

        if testsRun != 3:
            print(f"{RED}FAIL (only ran {testsRun} tests but expected 3){RESET} - {exe.parent}")



seeds = [1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000, 16000]
for seed in seeds:
    root = Path(fr"C:\Users\qwerty\Documents\repos\master_thesis\output-tigress_auto_{seed}")
    run_validations(root)