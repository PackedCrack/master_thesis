import subprocess
from pathlib import Path
from beartype import beartype
import os
import shutil

g_Symbols = False
g_LogPath: Path

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
def make_define_args(program: list[str]) -> list[str]:
    defines = []
    for define in program:
        defines.append("-D" + define + "=ON")

    return defines

@beartype
def add_deubg_flags(env: dict[str, str]) -> dict[str, str]:
    global g_Symbols
    if g_Symbols:
        env["_CL_"] += " /Zi"
        env["_LINK_"] += " /DEBUG"

    return env

@beartype
def add_linker_flags(env: dict[str, str], flags: list[str]) -> dict[str, str]:
    env["_LINK_"] = ""  # make sure the str exists

    linkerFlags = []
    if any(f == "/guard:cf" or f.startswith("/guard:cf") for f in flags):
        linkerFlags.append("/guard:cf")
    if any(f.startswith("/GL") for f in flags):
        linkerFlags.append("/LTCG")

    if len(linkerFlags) > 0:
        env["_LINK_"] = " ".join([*linkerFlags])
    
    return env

@beartype
def make_flag_env_dict(flags: list[str]) -> dict[str, str]:
    env = os.environ.copy()
    env["_CL_"] = " ".join(["/O2", "/MD", *flags])

    env = add_linker_flags(env, flags)    
    env = add_deubg_flags(env)

    return env

@beartype
def append_args(args: list[str], newArgs: list[str]) -> list[str]:
    for newArg in newArgs:
        args.append(newArg)

    return args

@beartype
def make_cmake_configuration_args(srcDirectory: Path, buildDirectory: Path, programs: list[str]) -> list[str]:
    args = ["cmake",
            "-S", str(srcDirectory),
            "-B", str(buildDirectory),
            
            "-G", "Visual Studio 17 2022",
            "-A", "x64"
            #"-T", "v143"
            ]
    

    defines = make_define_args(programs)
    args = append_args(args, defines)
    
    return args

@beartype
def make_flag_directory(flags: list[str]) -> Path:
    directory = ""
    for f in flags:
        f = f.replace("/", "_")
        f = f.replace(":", "-")
        directory += f

    return Path(directory)

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
def delete_directory(directory: Path) -> None:
    if directory.is_dir():
        shutil.rmtree(directory, ignore_errors = True)

@beartype
def compile_all_programs(srcDirectory: Path, buildDirectory: Path, binaryPath: Path, outputDirectory: Path, flags: list[str]) -> None:
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
    
    for program in programs:
        try:
            delete_directory(buildDirectory)
            args = make_cmake_configuration_args(srcDirectory, buildDirectory, program)
            subprocess.run(args, check = True, capture_output = True, text = True)
        except subprocess.CalledProcessError as err:
            log(f"CMake configuration failed for program {program} using flags {flags}. \nSTDOUT: {err.stdout}\nSTDERR: {err.stderr}")
            continue

        try:
            env = make_flag_env_dict(flags)
            subprocess.run(
                ["cmake", "--build", str(buildDirectory), "--config", "Release", "--parallel"],
                check = True,
                env = env,
                capture_output = True,
                text = True
            )
        except subprocess.CalledProcessError as err:
            log(f"Build failed for program {program} using flags {flags}. \nSTDOUT: {err.stdout}\nSTDERR: {err.stderr}")
            continue

        dst = outputDirectory / make_flag_directory(flags) / Path("mal_techniques.exe" + "".join(program))
        copy_file(binaryPath, dst)

def main():
    srcDirectory = Path(__file__).resolve().parent
    buildDirectory = srcDirectory / "build-python-baseline"
    outputDirectory = srcDirectory / "output-baseline"
    binaryPath = buildDirectory / "bin" / "mal_techniques.exe"
    
    make_log_file(srcDirectory / "compile-baseline.log")        # Create a new log file for this run
    delete_directory(buildDirectory)                            # Erase any previous build directory if it exsists
    delete_directory(outputDirectory)                           # Erase all output from previous executions of this script


    flags = ["/O2"]
    for f in flags:
        compile_all_programs(srcDirectory, buildDirectory, binaryPath, outputDirectory, [ f ])


main()