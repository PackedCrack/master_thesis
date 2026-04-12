from pathlib import Path
from beartype import beartype
import sys
import subprocess


g_idat = r"C:\Program Files\IDA Professional 9.0\idat.exe"
g_BlacklistScript = Path(__file__).resolve().parent / "ida_make_blacklist.py"
g_NgramsScript = Path(__file__).resolve().parent / "ida_ngrams.py"

@beartype
def make_log_file(location: Path):
    global g_LogPath
    g_LogPath = location

    assert g_LogPath.suffix.lower() == ".log"
    g_LogPath.unlink(missing_ok = True)
    g_LogPath.touch(exist_ok = False)

@beartype
def log(text: str) -> None:
    assert isinstance(g_LogPath, Path)
    with g_LogPath.open("a", encoding="utf-8") as f:
        f.write("\n=======================MESSAGE BEGIN=================================\n")
        f.write(text)
        f.write("\n========================MESSAGE END==================================\n")

@beartype
def advance_step(curStep: int, finalStep: int) -> int:
    progress = float(curStep) / float(finalStep)
    percent = min(100.0, progress * 100)

    sys.stdout.write(f"\rProgress: \x1b[42;30m{percent:6.1f}%\x1b[0m")
    sys.stdout.flush()

    return curStep + 1

@beartype
def get_flag_conf_dir() -> Path:
    this = Path(__file__).resolve().parent
    return this / "output"

@beartype
def get_tigress_dirs() -> list[Path]:
    this = Path(__file__).resolve().parent
    seeds = [1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000, 16000]
    dirs = []
    for seed in seeds:
        dirs.append(Path(this / f"output-tigress_auto_{seed}"))
    return dirs

@beartype
def get_baseline_dir() -> Path:
    this = Path(__file__).resolve().parent
    return this / "output-baseline"

@beartype
def clear_all_idb():
    baselineDir = get_baseline_dir() / "_O2"
    for entry in baselineDir.iterdir():
        if entry.suffix.lower() == ".i64":
            entry.unlink(missing_ok = True)

    tigressDirs = get_tigress_dirs()
    for tigressDir in tigressDirs:
        for programDir in tigressDir.iterdir():
            for entry in programDir.iterdir():
                if entry.suffix.lower() == ".i64":
                    entry.unlink(missing_ok = True)

    flagDir = get_flag_conf_dir()
    for programDir in flagDir.iterdir():
        for entry in programDir.iterdir():
            if entry.suffix.lower() == ".i64":
                entry.unlink(missing_ok = True)

@beartype
def analysis_succeeded(logfile: Path) -> bool:
    with open(logfile, "r", encoding = "utf-8") as l:
        loadedSuccess = False
        for line in l:
            if line.find("ERROR::") != -1:
                log(f"Log file {str(logfile)} contain error(s).")
                return False
            elif line.find("has been successfully loaded into the database") != -1:
                loadedSuccess = True
            elif line.find(".i64 already exists.") != -1:
                loadedSuccess = True

        if not loadedSuccess:
            log(f"Executeable was not loaded successfully into the database. See {str(logfile)}") 
            return False

    return True

@beartype 
def make_out_dir(binary: Path, suffix: str) -> Path:
    outDir = binary.parent / str(binary.name).replace(".", "_") / suffix
    outDir.mkdir(parents = True, exist_ok = True)
    return outDir

@beartype
def make_ida_log_file(outDir: Path) -> Path:
    logfile = outDir / "log.txt"
    logfile.unlink(missing_ok = True)
    
    return logfile

@beartype
def make_blacklist(binary: Path) -> Path | None:
    outDir = make_out_dir(binary, "blacklist")
    logfile = make_ida_log_file(outDir)

    args = [g_idat, "-A", f"-S{str(g_BlacklistScript)} {str(outDir)}", f"-L{str(logfile)}", str(binary)]
    subprocess.run(args, cwd = Path(__file__).resolve().parent, check = True)

    if not analysis_succeeded(logfile):
        log(f"Analysis error when attempting to create blacklist for: {str(binary)}")
        return None

    return outDir / "blacklist.txt"

@beartype
def extract_ngrams(binary: Path, blacklist: Path):
    outDir = make_out_dir(binary, "ngrams")
    logfile = make_ida_log_file(outDir)

    args = [g_idat, "-A", f"-S{str(g_NgramsScript)} {str(outDir)} {str(blacklist)}", f"-L{str(logfile)}", str(binary)]
    subprocess.run(args, cwd = Path(__file__).resolve().parent, check = True)

    if not analysis_succeeded(logfile):
        log(f"Analysis error when attempting to extract ngrams for: {str(binary)}")
        return None

@beartype
def process_binary(curStep: int, finalStep: int, binary: Path) -> int:
    blacklist = make_blacklist(binary)
    curStep = advance_step(curStep, finalStep)

    if blacklist != None:
        extract_ngrams(binary, blacklist)
                
    return advance_step(curStep, finalStep)
    
def process_baselines(curStep: int, finalStep: int) -> int:
    binaryDir = get_baseline_dir() / "_O2"
    for entry in binaryDir.iterdir():
        if entry.is_file():
            if entry.suffix.lower().find(".exe") != -1:
                binary = entry.resolve()
                curStep = process_binary(curStep, finalStep, binary)

    return curStep

def process_tigress(curStep: int, finalStep: int) -> int:
    tigressDirs = get_tigress_dirs()
    for tigressDir in tigressDirs:
        for programDir in tigressDir.iterdir():
            if not programDir.is_file():
                for entry in programDir.iterdir():
                    if entry.name == "obf.exe":
                        binary = entry.resolve()
                        curStep = process_binary(curStep, finalStep, binary)
            else:
                log(f"Found unexpected file {str(programDir)} in {str(tigressDir)}")

    return curStep

def process_flag_confs(curStep: int, finalStep: int) -> int:
    flagDir = get_flag_conf_dir()
    for programDir in flagDir.iterdir():
        if not programDir.is_file():
            for entry in programDir.iterdir():
                if entry.is_file():
                    if entry.suffix.lower().find(".exe") != -1:
                        binary = entry.resolve()
                        curStep = process_binary(curStep, finalStep, binary)
        else:
            log(f"Found unexpected file {str(programDir)} in {str(flagDir)}")

    return curStep

def main():
    make_log_file(Path(__file__).resolve().parent / "ngram_extraction.log")
    clear_all_idb()


    curStep = 1
    finalStep = (1344 + 21 + (21 * 16)) * 2 # (num flag programs + num baselines + (num tigress programs * num tigress seeds)) * num ida calls
    curStep = process_baselines(curStep, finalStep)
    curStep = process_tigress(curStep, finalStep)
    curStep = process_flag_confs(curStep, finalStep)

main()