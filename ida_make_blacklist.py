import idc
import idautils
import ida_pro
import ida_auto
from pathlib import Path


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

def resolve_output_directory(output: str) -> Path:
    out = resolve_filepath(output)
    if not out.exists():
        log_err(f"Blacklist does not exist. Filename: {output} and resolved absolute path: {str(out)}.")

    return out


if not wait_for_analysis():
    ida_pro.qexit(1)

out = resolve_output_directory(idc.ARGV[1])
blacklist = out / "blacklist.txt"

with open(blacklist, "w", encoding = "utf-8") as file:
    for ea in idautils.Functions():
        name = idc.get_func_name(ea)
        if name != "main" and (name.find("sub_") == -1):
            file.write(f"{name}\n")

ida_pro.qexit(0)