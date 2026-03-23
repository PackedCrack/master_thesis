import subprocess
from pathlib import Path


def print_success(msg: str):
    print(f"\033[32m{msg}\033[0m")

def print_fail(msg: str):
    print(f"\033[31m{msg}\033[0m")

def database_exist(binary: Path) -> bool:
    parentDir = binary.parent
    # If idb failed to archive correctly it will expose internal file types
    extensions = [".id0", ".id1", ".id2", ".nam", ".til"]
    for entry in parentDir.iterdir():
        if entry.is_file():
            if entry.suffix in extensions:
                print_fail(f"Found intermediate IDB file: {entry.name}")
                return False

    for entry in parentDir.iterdir():
        if entry.is_file():
            if entry.suffix == ".i64":
                print_success("Found IDB file: .i64")
                return True

    print_fail("Failed to found IDB file: .i64")
    return False

def log_is_error_free(outDir: Path) -> bool:
    with open(outDir / "log.txt", "r", encoding = "utf-8") as log:
        for line in log:
            if line.find("ERROR::") != -1:
                print_fail(f"Log file {str(outDir / "log.txt")} contain error(s).")
                return False

    print_success("Found no errors in log file.")  
    return True

def correct_ngrams(outDir: Path) -> bool:
    ngrams = {}
    with open(outDir / "ngrams.csv", "r", encoding = "utf-8") as file:
        for line in file:
            l = line.strip("\r\n")
            mnemonic, count = [part.strip() for part in l.rsplit(",", 1)]
            if mnemonic in ngrams:
                print_fail(f"Duplicate mnemonic found: {mnemonic} in {str(outDir / "ngrams.csv")}.")
                return False
            
            ngrams[mnemonic] = int(count)

    expectedNgrams = {
        "[push push sub]": 3,
        "[push sub lea]": 3,
        "[sub lea lea]": 3,
        "[lea lea call]": 3,
        "[lea call nop]": 3,
        "[call nop mov]": 3,
        "[nop mov mov]": 3,
        "[mov mov mov]": 6,
        "[mov mov call]": 2,
        "[mov call mov]": 2,
        "[call mov mov]": 1,
        "[call mov xor]": 1,
        "[mov xor lea]": 1,
        "[xor lea pop]": 1,
        "[lea pop pop]": 3,
        "[pop pop retn]": 3,
        "[mov mov push]": 2,
        "[mov push push]": 2,
        "[mov mov add]": 2,
        "[mov add mov]": 2,
        "[add mov mov]": 1,
        "[add mov lea]": 1,
        "[mov lea pop]": 2,
        "[mov mov sub]": 2,
        "[mov sub mov]": 2,
        "[sub mov mov]": 1,
        "[sub mov lea]": 1
    }

    if ngrams != expectedNgrams:
        print_fail("Extracted ngrams are incorrect.")
        return False
    
    print_success("Extracted ngrams are correct.")
    return True

idat = r"C:\Program Files\IDA Professional 9.0\idat.exe"
# relative paths
script = "ida_ngrams.py"
blacklist = r"ida_test\blacklist.txt"
outDir = "ida_test"
log = r"ida_test\log.txt"
testBinary = r"ida_test\bin_for_test.exe"


args = [idat, "-A", f"-S{script} {outDir} {blacklist}", f"-L{log}", testBinary]
subprocess.run(args, cwd = Path(__file__).resolve().parent, check = True)

assert database_exist(Path(testBinary))
assert log_is_error_free(Path(outDir))
assert correct_ngrams(Path(outDir))