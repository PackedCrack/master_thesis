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
        dirs.append(Path(this / f"output-tigress_{seed}"))
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
def as_distribution(ngrams: Path) -> dict | None:
    with ngrams.open(newline = "", encoding = "utf-8") as file:
        reader = csv.reader(file)
        data = {}
        for key, value in reader:
            assert key not in data
            data[key] = int(value)

        totalCount = sum(data.values())
        if totalCount > 0:
            return {key: value / totalCount for key, value in data.items()}
        
        log(f"{str(ngrams)} had no counts!")
        return None

@beartype
def load_baseline_ngrams(programIds: dict) -> list[dict]:
    distributions = [{} for _ in range(21)]
    
    rootDir = get_baseline_dir() / "_O2"
    for entry in rootDir.iterdir():
        if not entry.is_file():
            ngrams = entry.resolve() / Path("ngrams") / Path("ngrams.csv")
            if not ngrams.exists():
                log(f"File {str(ngrams)} does not exist.")
            elif ngrams.stat().st_size < 8 * 1024: # 8kb
                log(f"File {str(ngrams)} is smaller than 8kb. It may have broken ngrams.. Inspect it.")
            else:
                distribution = as_distribution(ngrams)
                if distribution is not None:
                    programName = entry.name[-15:]
                    programName = programName.replace("T", "_T")
                    id = programIds[programName]
                    distributions[id - 1] = distribution
                else:
                    log(f"{str(ngrams)} produced None distribution")

    return distributions    

@beartype
def make_sheet(wb: Workbook, title: str, width: int, columnNames: list[str], rowNames: list[str], data: list[list[float]]):
    sheet = wb.create_sheet(title = title)
    sheet.append([""] + columnNames)    # Top left cell must be empty

    for rowName, rowValues, in zip(rowNames, data):
        sheet.append([rowName] + rowValues)

    for i in range(len(columnNames) + 1):
        col = openpyxl.utils.get_column_letter(i + 1)
        sheet.column_dimensions[col].width = width

    return sheet

# no templates because ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
@beartype
def make_sheet2(wb: Workbook, title: str, width: int, columnNames: list[str], rowNames: list[str], data: list[list[str]]):
    sheet = wb.create_sheet(title = title)
    sheet.append([""] + columnNames)    # Top left cell must be empty

    for rowName, rowValues, in zip(rowNames, data):
        sheet.append([rowName] + rowValues)

    for i in range(len(columnNames) + 1):
        col = openpyxl.utils.get_column_letter(i + 1)
        sheet.column_dimensions[col].width = width

    return sheet

@beartype
def make_tigress_sheet_names(programIds: dict) -> list[list[str]]:
    rowNames = []
    global g_Seeds
    for seed in g_Seeds:
        rowNames.append(f"Seed: {seed}")

    colNames = [str()] * 21
    for id in programIds.values():
        colNames[id - 1] = f"Program ID: {id}"

    return [colNames, rowNames]

@beartype
def make_flag_sheet_names(programIDs: dict, configIDs: dict) -> list[list[str]]:
    rowNames = [str()] * 64
    for id in configIDs.values():
        rowNames[id - 1] = f"Config: {id}"

    colNames = [str()] * 21
    for id in programIDs.values():
        colNames[id - 1] = f"Program ID: {id}"

    return [colNames, rowNames]

@beartype
def load_tigress_ngrams(programIds: dict) -> list[list[dict]]:
    distributions = [[{} for _ in range(21)] for _ in range(16)]        # distributions[seed][programid] = distribution
    assert len(distributions) == 16                                     # 16 seeds
    assert all(len(seed_list) == 21 for seed_list in distributions)     # each seed has 21 programs

    dirs = get_tigress_dirs()
    seedIndex = 0
    for dir in dirs:
        seedDistributions = distributions[seedIndex]

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
                        distribution = as_distribution(ngrams)
                        if distribution is not None:
                            id = programIds[programName]
                            seedDistributions[id - 1] = distribution
                        else:
                            log(f"{str(ngrams)} produced None distribution")
        seedIndex += 1

    return distributions

def as_aligned_vectors(baselineDistribution: dict, programDistribution: dict):
    assert len(baselineDistribution) > 0
    assert len(programDistribution) > 0

    allNgrams = sorted(baselineDistribution.keys() | programDistribution.keys())

    p = numpy.array([baselineDistribution.get(ngram, 0.0) for ngram in allNgrams], dtype = numpy.float64)
    q = numpy.array([programDistribution.get(ngram, 0.0) for ngram in allNgrams], dtype = numpy.float64)

    # For debug
    #p2 = {k: baselineDistribution.get(k, 0.0) for k in allNgrams}
    #q2 = {k: programDistribution.get(k, 0.0) for k in allNgrams}

    return p, q

@beartype
def compute_tigress_jsd(wb: Workbook, programIds: dict, baselineDistributions: list[dict]):
    # distributions[seed][programid] = distribution
    distributions = load_tigress_ngrams(programIds)

    # results[seed][programid]
    results = [[0.0] * 21 for _ in range(16)]
    seedIndex = 0
    for seedDistributions in distributions:
        id = 0
        for programDistribution in seedDistributions:
            baselineDistribution = baselineDistributions[id]
            p, q = as_aligned_vectors(baselineDistribution, programDistribution)

            # https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.distance.jensenshannon.html
            results[seedIndex][id] = scipy.spatial.distance.jensenshannon(p, q, base = 2.0) ** 2
            id += 1
        seedIndex += 1

    colNames, rowNames = make_tigress_sheet_names(programIds)
    tigressSheet = make_sheet(wb, "Tigress", 20, colNames, rowNames, results)

@beartype
def sha256(path: Path) -> str:
    with path.open("rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()
    
@beartype
def compute_tigress_hashes(wb: Workbook, programIds: dict):
    hashes = [["" for _ in range(21)] for _ in range(16)]        # hashes[seed][programid] = hash
    assert len(hashes) == 16                                     # 16 seeds
    assert all(len(seed_list) == 21 for seed_list in hashes)     # each seed has 21 programs

    dirs = get_tigress_dirs()
    seedIndex = 0
    for dir in dirs:
        for entry in dir.iterdir():
            if entry.is_file():
                log(f"Unexepcted file: {str(entry.resolve())}")
            else:
                programName = "_" + entry.name[:-1]
                if programName not in programIds:
                    log(f"Failed to find id for program: {programName}")
                else:
                    executable = entry / Path("obf.exe")
                    if not executable.exists():
                        log(f"Unexpected missing ngrams file: {str(executable.resolve())}")
                    else:
                        id = programIds[programName]
                        hashes[seedIndex][id - 1] = sha256(executable)

        seedIndex += 1
    
    colNames, rowNames = make_tigress_sheet_names(programIds)
    sheet = make_sheet2(wb, "Tigress Hashes", 70, colNames, rowNames, hashes)

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
def load_flag_ngrams(programIDs: dict, configIDs: dict) -> list[list[dict]]:
    distributions = [[{} for _ in range(21)] for _ in range(64)]        # distributions[config][programid] = distribution
    assert len(distributions) == 64                                     # 64 configs
    assert all(len(seed_list) == 21 for seed_list in distributions)     # each config has 21 programs

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
                distribution = as_distribution(ngrams)
                if distribution is not None:
                    configID = to_config_id(configIDs, runDir)
                    programID = to_program_id(programIDs, entry)
                    if (configID is None) or (programID is None):
                        log("Invalid config or program id.")
                    else:
                        distributions[configID - 1][programID - 1] = distribution
                else:
                    log(f"{str(ngrams)} produced None distribution")

    return distributions

@beartype
def compute_flag_jsd(wb: Workbook, programIDs: dict, baselineDistributions: list[dict]):
    configIDs = config_indices()
    distributions = load_flag_ngrams(programIDs, configIDs)

    # results[configID][programID]
    results = [[0.0] * 21 for _ in range(64)]
    configIndex = 0
    for configDistribution in distributions:
        programID = 0
        for programDistribution in configDistribution:
            baselineDistribution = baselineDistributions[programID]
            p, q = as_aligned_vectors(baselineDistribution, programDistribution)

            # https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.distance.jensenshannon.html
            results[configIndex][programID] = scipy.spatial.distance.jensenshannon(p, q, base = 2.0) ** 2
            programID += 1

        configIndex += 1

    colNames, rowNames = make_flag_sheet_names(programIDs, configIDs)
    sheet = make_sheet(wb, "Compiler Flags", 20, colNames, rowNames, results)

def main():
    make_log_file(Path(__file__).resolve().parent / "jsd_compute.log")

    programIds = progam_ids()
    baselineDistributions = load_baseline_ngrams(programIds)

    wb = Workbook()

    compute_flag_jsd(wb, programIds, baselineDistributions)
    compute_tigress_jsd(wb, programIds, baselineDistributions)
    compute_tigress_hashes(wb, programIds)

    wb.save("jsd_results.xlsx")

main()