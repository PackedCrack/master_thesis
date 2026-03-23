import ida_auto
import ida_pro
import idautils
import ida_ua
import idc
import random
from pathlib import Path


# Execute as
# & "C:\Program Files\IDA Professional 9.0\idat.exe" -A -S"ida_ngrams.py ida_test ida_test\blacklist.txt" -L"ida_test\log.txt" ida_test\bin_for_test.exe

def log_info(msg: str):
    print(f"INFO::{msg}")

def log_err(msg: str):
    print(f"ERROR::{msg}")

def wait_for_analysis() -> bool:
    ok = ida_auto.auto_wait()
    if not ok:
        log_err("Wait for auto analysis was cancelled or did not complete correctly")
        
    return ok

def resolve_filepath(filename: str) -> Path:
    parentDir = Path(__file__).resolve().parent
    absPath = Path(parentDir / filename).resolve()
    return absPath

def resolve_blacklist_path(blacklist: str) -> Path:
    bl = resolve_filepath(blacklist)
    if not bl.exists():
        log_err(f"Blacklist does not exist. Filename: {blacklist} and resolved absolute path: {str(bl)}.")

    return bl

def resolve_output_directory(output: str) -> Path:
    out = resolve_filepath(output)
    if not out.exists():
        log_err(f"Blacklist does not exist. Filename: {output} and resolved absolute path: {str(out)}.")

    return out

def parse_blacklist(blacklist: Path) -> set:
    bl = set()
    with open(blacklist, "r", encoding = "utf-8") as file:
        for line in file:
            bl.add(line.strip("\r\n"))

    return bl

def get_blacklist() -> set:
    bl = resolve_blacklist_path(idc.ARGV[2])
    log_info(str(bl))

    return parse_blacklist(bl)

def get_mnemonics(functionAddress):
    mnemonics = []
    for ea in idautils.FuncItems(functionAddress):
        mnemonic = ida_ua.print_insn_mnem(ea)
        mnemonics.append(mnemonic)

    return mnemonics

def add_ngrams(ngrams: dict, mnemonics) -> dict:
    for i in range(0, len(mnemonics) - 2):
        ngram = (mnemonics[i],
                 mnemonics[i + 1],
                 mnemonics[i + 2])
        
        if ngram in ngrams:
            ngrams[ngram] += 1
        else:
            ngrams[ngram] = 1

    return ngrams

def save_ngrams(out: Path, ngrams: dict):
    with open(out / "ngrams.csv", "w", encoding = "utf-8") as file:
        for ngram, count in ngrams.items():
            file.write(f"[{ngram[0]} {ngram[1]} {ngram[2]}], {count}\n")


if not wait_for_analysis():
    ida_pro.qexit(1)

#scriptName = idc.ARGV[0]
out = resolve_output_directory(idc.ARGV[1])
log_info(str(out))
blacklist = get_blacklist()

ngrams = {}
for funcAddress in idautils.Functions():
    funcName = idc.get_func_name(funcAddress)
    if funcName not in blacklist:
        mnemonics = get_mnemonics(funcAddress)
        ngrams = add_ngrams(ngrams, mnemonics)

save_ngrams(out, ngrams)


ida_pro.qexit(0)