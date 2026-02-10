import subprocess
from pathlib import Path
from beartype import beartype
import shutil
import os


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
def delete_directory(directory: Path) -> None:
    if directory.is_dir():
        shutil.rmtree(directory, ignore_errors = True)

@beartype
def copy_file(src: Path, dst: Path) -> None:
    dstDirectory = os.path.dirname(os.path.abspath(str(dst)))
    if dstDirectory:
        os.makedirs(dstDirectory, exist_ok = True)

    pdb = src.with_suffix(".pdb")
    if pdb.is_file():
        shutil.copy2(pdb, Path(str(dst) + ".pdb"))

    shutil.copy2(str(src), str(dst))

@beartype
def make_absolute_path(parent: Path, filename: str) -> Path:
    return parent / Path(filename)

@beartype
def make_folder_name(program: list[str]) -> Path:
    folder = ""
    for p in program:
        folder += p
        folder += "_"
    return Path(folder)

@beartype
def make_sources_list(sourceLocation: Path, program: list[str]) -> list[str]:
    sources = [
        rf"{str(sourceLocation)}\main.c",
        rf"{str(sourceLocation)}\runtime_linking.c",
        rf"{str(sourceLocation)}\misc\common.c",
        rf"{str(sourceLocation)}\misc\str.c",
        rf"{str(sourceLocation)}\misc\vector.c",
        rf"{str(sourceLocation)}\misc\wstr.c"
    ]

    for p in program:
        if p == "T1059" or p == "T1547":
            sources.append(rf"{str(sourceLocation)}\{p}.001\{p}.001.c")
        elif p == "T1070":
            sources.append(rf"{str(sourceLocation)}\{p}.004\{p}.004.c")
        else:
            sources.append(rf"{str(sourceLocation)}\{p}\{p}.c")

    return sources

@beartype
def tigress_merge(vcvars64: Path, tigressLocation: Path, sourceLocation: Path, outLocation: Path, program: list[str], ):
    sources = make_sources_list(sourceLocation, program)

    outFile = outLocation / Path("merged.c")
    outFile.parent.mkdir(parents = True, exist_ok = True)
    out = outFile.as_posix()                                    # Prefer forward slashes for Tigress/Cygwin tooling

    tigress = subprocess.list2cmdline([tigressLocation, "--Merge", *sources, f"--out={out}"])
    cmdline = f'call "{vcvars64}" && call {tigress}'

    try:
        subprocess.run(cmdline, 
                       cwd = str(sourceLocation), 
                       shell = True, 
                       check = True,
                       capture_output = True,
                       text = True)
    except subprocess.CalledProcessError as err:
        log(f"Trigress failed to merge {program}. \nSTDOUT: {err.stdout}\nSTDERR: {err.stderr}")

def main():
    rootDirectory = Path(__file__).resolve().parent
    outputDirectory = rootDirectory / "output-tigress"
    srcDirectory = rootDirectory / "src"
    make_log_file(rootDirectory / "compile_tigress.log")         # Create a new log file for this run
    delete_directory(outputDirectory)                           # Erase all output from previous executions of this script

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
    

    vcvars64 = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")
    tigress = Path(r"C:\Program Files\University of Arizona\Tigress C Source Code Obfuscator\Tigress\tigress.bat")
    for program in programs:
        folder = make_folder_name(program)
        out = outputDirectory / folder
        tigress_merge(vcvars64, tigress, srcDirectory, out, program)
    


main()