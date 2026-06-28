import numpy
import scipy
from beartype import beartype
from pathlib import Path
import sys
import csv
import openpyxl.utils
from openpyxl import Workbook
import hashlib
import re
import pefile
import heapq


# The jensen shannon distance is the sqr of jensen shannon divergence
# https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.distance.jensenshannon.html


g_Seeds = [1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000, 16000]

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
    dirs = []
    global g_Seeds
    for seed in g_Seeds:
        dirs.append(Path(this / f"output-tigress_auto_{seed}"))
    return dirs

@beartype
def get_baseline_dir() -> Path:
    this = Path(__file__).resolve().parent
    return this / "output-baseline"

@beartype
def progam_ids() -> dict:
    T1082 = "T1082" # A
    T1083 = "T1083" # B
    T1057 = "T1057" # C
    T1070 = "T1070" # D
    T1547 = "T1547" # E
    T1059 = "T1059" # F
    T1005 = "T1005" # G
    programs = [[T1082, T1083, T1057], [T1082, T1083, T1070], [T1082, T1083, T1547], [T1082, T1057, T1005],
                [T1082, T1057, T1059 ], [T1082, T1070, T1547], [T1082, T1070, T1059 ], [T1082, T1547, T1005],
                [T1082, T1059, T1005], [T1083, T1057, T1547], [T1083, T1057, T1005], [T1083, T1070, T1059 ],
                [T1083, T1070, T1005], [T1083, T1547, T1059 ], [T1083, T1059, T1005], [T1057, T1070, T1547],
                [T1057, T1070, T1059 ], [T1057, T1070, T1005], [T1057, T1547, T1059 ], [T1070, T1547, T1005],
                [T1547, T1059, T1005]]
    
    def to_str(program: list[str]) -> str:
        name = ""
        for module in program:
            name += "_"
            name += module
        return name

    ids = {}
    for i in range(0, len(programs)):
        name = to_str(programs[i])
        ids[name] = i + 1

    return ids

@beartype
def as_count_vector(ngrams: Path) -> dict | None:
    with ngrams.open(newline = "", encoding = "utf-8") as file:
        reader = csv.reader(file)
        data = {}
        for key, value in reader:
            assert key not in data
            data[key] = int(value)

        totalCount = sum(data.values())
        if totalCount > 0:
            return data
        
        log(f"{str(ngrams)} had no counts!")
        return None

@beartype
def calc_top5(programTop5: list[dict]) -> dict:
    merged = {}
    for top5 in programTop5:
        for k, v in top5.items():
            if k not in merged or v > merged[k]:
                merged[k] = v

    top5 = dict(heapq.nlargest(5, merged.items(), key = lambda item: item[1]))
    return top5

@beartype
def count_baseline_ngrams(programIds: dict):
    programTop5 = [{} for _ in range(21)]
    counts = []
    rootDir = get_baseline_dir() / "_O2"
    for entry in rootDir.iterdir():
        if not entry.is_file():
            ngrams = entry.resolve() / Path("ngrams") / Path("ngrams.csv")
            if not ngrams.exists():
                log(f"File {str(ngrams)} does not exist.")
            elif ngrams.stat().st_size < 8 * 1024: # 8kb
                log(f"File {str(ngrams)} is smaller than 8kb. It may have broken ngrams.. Inspect it.")
            else:
                ngramCounts = as_count_vector(ngrams)
                if ngramCounts is not None:
                    counts.append(len(ngramCounts))
                    programName = entry.name[-15:]
                    programName = programName.replace("T", "_T")
                    id = programIds[programName]
                    programTop5[id - 1] = dict(sorted(ngramCounts.items(), key = lambda item: item[1], reverse = True)[:5])
                else:
                    log(f"{str(ngrams)} produced None distribution")

    print("\n\nBaseline Top5:")
    top5 = calc_top5(programTop5)
    print(top5)
    print(f"Mean ngrams per program: {(sum(counts) / len(counts))}\nn-gram min/max range: [{min(counts)}-{max(counts)}]")

@beartype
def count_tigress_ngrams(programIds: dict):
    programTop5 = [[{} for _ in range(21)] for _ in range(16)]        # distributions[seed][programid] = distribution
    assert len(programTop5) == 16                                     # 16 seeds
    assert all(len(seedTop5s) == 21 for seedTop5s in programTop5)       # each seed has 21 programs

    counts = []
    dirs = get_tigress_dirs()
    seedIndex = 0
    for dir in dirs:
        seedTop5s = programTop5[seedIndex]

        for entry in dir.iterdir():
            if entry.is_file():
                log(f"Unexepcted file: {str(entry.resolve())}")
            else:
                programName = "_" + entry.name[:-1]
                if programName not in programIds:
                    log(f"Failed to find id for program: {programName}")
                else:
                    ngrams = entry / Path("obf_exe") / Path("ngrams") / Path("ngrams.csv")
                    if not ngrams.exists():
                        log(f"Unexpected missing ngrams file: {str(ngrams.resolve())}")
                    elif ngrams.stat().st_size < 8 * 1024: # 8kb
                        log(f"File {str(ngrams)} is smaller than 8kb. It may have broken ngrams.. Inspect it.")
                    else:
                        ngramCounts = as_count_vector(ngrams)
                        if ngramCounts is not None:
                            counts.append(len(ngramCounts))
                            id = programIds[programName]
                            seedTop5s[id - 1] = dict(sorted(ngramCounts.items(), key = lambda item: item[1], reverse = True)[:5])
                        else:
                            log(f"{str(ngrams)} produced None distribution")
        seedIndex += 1

    print("\n\nTigress Top5:")
    allTop5s = []
    for seedTop5s in programTop5:
        allTop5s.extend(seedTop5s)
    top5 = calc_top5(allTop5s)
    print(top5)
    print(f"Mean ngrams per program: {(sum(counts) / len(counts))}\nn-gram min/max range: [{min(counts)}-{max(counts)}]")
  
@beartype
def config_indices() -> dict:
    flags = [['/guard:cf'],
            ['/homeparams', '/GL'],
            ['/Qspectre-load', '/GL'],
            ['/homeparams', '/Qspectre-load', '/guard:cf'],
            ['/Gs0', '/GL'],
            ['/homeparams', '/Gs0', '/guard:cf'],
            ['/Qspectre-load', '/Gs0', '/guard:cf'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/GL'],
            ['/favor:INTEL64', '/GL'],
            ['/homeparams', '/favor:INTEL64', '/guard:cf'],
            ['/Qspectre-load', '/favor:INTEL64', '/guard:cf'],
            ['/homeparams', '/Qspectre-load', '/favor:INTEL64', '/GL'],
            ['/Gs0', '/favor:INTEL64', '/guard:cf'],
            ['/homeparams', '/Gs0', '/favor:INTEL64', '/GL'],
            ['/Qspectre-load', '/Gs0', '/favor:INTEL64', '/GL'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/favor:INTEL64', '/guard:cf'],
            ['/Gh', '/GL'],
            ['/homeparams', '/Gh', '/guard:cf'],
            ['/Qspectre-load', '/Gh', '/guard:cf'],
            ['/homeparams', '/Qspectre-load', '/Gh', '/GL'],
            ['/Gs0', '/Gh', '/guard:cf'],
            ['/homeparams', '/Gs0', '/Gh', '/GL'],
            ['/Qspectre-load', '/Gs0', '/Gh', '/GL'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/Gh', '/guard:cf'],
            ['/favor:INTEL64', '/Gh', '/guard:cf'],
            ['/homeparams', '/favor:INTEL64', '/Gh', '/GL'],
            ['/Qspectre-load', '/favor:INTEL64', '/Gh', '/GL'],
            ['/homeparams', '/Qspectre-load', '/favor:INTEL64', '/Gh', '/guard:cf'],
            ['/Gs0', '/favor:INTEL64', '/Gh', '/GL'],
            ['/homeparams', '/Gs0', '/favor:INTEL64', '/Gh', '/guard:cf'],
            ['/Qspectre-load', '/Gs0', '/favor:INTEL64', '/Gh', '/guard:cf'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/favor:INTEL64', '/Gh', '/GL'],
            ['/QIntel-jcc-erratum'],
            ['/homeparams', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Qspectre-load', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Qspectre-load', '/QIntel-jcc-erratum'],
            ['/Gs0', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Gs0', '/QIntel-jcc-erratum'],
            ['/Qspectre-load', '/Gs0', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/favor:INTEL64', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/favor:INTEL64', '/QIntel-jcc-erratum'],
            ['/Qspectre-load', '/favor:INTEL64', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Qspectre-load', '/favor:INTEL64', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Gs0', '/favor:INTEL64', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Gs0', '/favor:INTEL64', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Qspectre-load', '/Gs0', '/favor:INTEL64', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/favor:INTEL64', '/QIntel-jcc-erratum'],
            ['/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Gh', '/QIntel-jcc-erratum'],
            ['/Qspectre-load', '/Gh', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Qspectre-load', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Gs0', '/Gh', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Gs0', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Qspectre-load', '/Gs0', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/Gh', '/QIntel-jcc-erratum'],
            ['/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum'],
            ['/homeparams', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/Qspectre-load', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Qspectre-load', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum'],
            ['/Gs0', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL'],
            ['/homeparams', '/Gs0', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum'],
            ['/Qspectre-load', '/Gs0', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum'],
            ['/homeparams', '/Qspectre-load', '/Gs0', '/favor:INTEL64', '/Gh', '/QIntel-jcc-erratum', '/guard:cf', '/GL']]
    

    ids = {}
    index = 0
    for fs in flags:
        index += 1
        configName = ""
        for f in fs:
            cleaned = f.replace("/", "_")
            cleaned = cleaned.replace(":", "-")
            configName += cleaned
        if configName not in ids:
            ids[configName] = index
        else:
            log(f"Unexpected double occourances of {configName}")

    return ids

@beartype
def to_config_id(configIDs: dict, runDir: Path) -> int | None:
    configName = re.sub(r"^Run_\d+_", "", runDir.name)
    if configName not in configIDs:
        log(f"Unexpected config name: {configName}")
        return None

    return configIDs[configName]

@beartype
def to_program_id(programIDs: dict, dir: Path) -> int | None:
    programName = dir.name[-15:]
    programName = programName.replace("T", "_T")
    if programName not in programIDs:
        log(f"Unexpected program name: {programName}")
        return None
    
    return programIDs[programName]

@beartype
def count_flag_ngrams(programIDs: dict, configIDs: dict):
    programTop5 = [[{} for _ in range(21)] for _ in range(64)]        # distributions[config][programid] = distribution
    assert len(programTop5) == 64                                     # 64 configs
    assert all(len(seedList) == 21 for seedList in programTop5)       # each config has 21 programs

    counts = []
    outputDir = get_flag_conf_dir()
    for runDir in outputDir.iterdir():
        for entry in runDir.iterdir():
            if entry.is_file():
                continue

            ngrams = entry / Path("ngrams") / Path("ngrams.csv")
            if not ngrams.exists():
                log(f"Unexpected missing ngrams file: {str(ngrams.resolve())}")
            elif ngrams.stat().st_size < 8 * 1024: # 8kb
                log(f"File {str(ngrams)} is smaller than 8kb. It may have broken ngrams.. Inspect it.")
            else:
                ngramCounts = as_count_vector(ngrams)
                if ngramCounts is not None:
                    configID = to_config_id(configIDs, runDir)
                    programID = to_program_id(programIDs, entry)
                    if (configID is None) or (programID is None):
                        log("Invalid config or program id.")
                    else:
                        counts.append(len(ngramCounts))
                        programTop5[configID - 1][programID - 1] = dict(sorted(ngramCounts.items(), key = lambda item: item[1], reverse = True)[:5])
                else:
                    log(f"{str(ngrams)} produced None distribution")

    print("\n\nFlag Top5:")
    allTop5s = []
    for seedTop5s in programTop5:
        allTop5s.extend(seedTop5s)
    top5 = calc_top5(allTop5s)
    print(top5)
    print(f"Mean ngrams per program: {(sum(counts) / len(counts))}\nn-gram min/max range: [{min(counts)}-{max(counts)}]")

def main():
    make_log_file(Path(__file__).resolve().parent / "count_ngrams.log")

    programIds = progam_ids()
    count_baseline_ngrams(programIds)
    count_tigress_ngrams(programIds)
    configIDs = config_indices()
    count_flag_ngrams(programIds, configIDs)

main()