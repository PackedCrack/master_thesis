from cProfile import label
import subprocess
from pathlib import Path
from beartype import beartype
import shutil
import os
import sys
import re
import secrets


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
def make_tigress_define_args(program: list[str]) -> list[str]:
    defines = ["-DNDEBUG"]
    for p in program:
        defines.append(f"-D{p}")

    return defines

@beartype
def tigress_merge(vcvars64: Path, tigressLocation: Path, sourceLocation: Path, outFile: Path, program: list[str]):
    sources = make_sources_list(sourceLocation, program)

    defines = make_tigress_define_args(program)
    
    outFile.parent.mkdir(parents = True, exist_ok = True)
    out = outFile.as_posix()                                    # pefer forward slashes

    tigress = subprocess.list2cmdline([tigressLocation, "--FilePrefix=AUTO", *defines, "--Merge", *sources, f"--out={out}"])
    cmdline = f'call "{vcvars64}" && call {tigress}'

    #try:
    #    subprocess.run(cmdline, 
    #                   cwd = str(sourceLocation), 
    #                   shell = True, 
    #                   check = True,
    #                   capture_output = True,
    #                   text = True)
    #except subprocess.CalledProcessError as err:
    #    log(f"Trigress failed to merge {program}. \nSTDOUT: {err.stdout}\nSTDERR: {err.stderr}")

    cmd = [str(tigressLocation), "--FilePrefix=AUTO", *defines, "--Merge", *sources, f"--out={out}"]
    env = vcvars_env(vcvars64)
    process = subprocess.run(cmd,
                             cwd = str(sourceLocation),
                             capture_output = True,
                             env = env,
                             text = True,
                             check = False
                             )
    
    o = Path(out)
    if process.returncode != 0:
        log(f"Tigress merging failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")
    else:
        try:    # Check if tigress produced file exist and is not empty
            if ((not o.is_file()) or (o.stat().st_size < 1)):
                log(f"Tigress merging failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")
        except FileNotFoundError:
            log(f"Tigress merging failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")

@beartype
def fix_msvc_extensions(file: Path):
    extensionsAndFixes = [[b'union  __declspec(align(16))', b'__declspec(align(16)) union']]

    data = file.read_bytes()
    new = data
    for extFix in extensionsAndFixes:
        extension, fix = extFix
        new = data.replace(extension, fix)
    
    if new != data:
        file.write_bytes(new)
    
@beartype
def make_misc_functions() -> list[str]:
    runtime_linking = [
        "dll_module_handle",
        "procedure_list_create",
        "procedure_list_destroy",
    ]
    main = [
        "set_seed",
        "main",
    ]
    wstr = [
        "details_wstring_create",
        "details_wstring_create2",
        "details_wstring_destroy",
        "details_wstring_c_str",
        "details_wstring_size",
        "details_wstring_back",
        "details_wstring_push_back",
        "details_wstring_concatenate",
    ]
    vector = [
        "round_up_to_pow_2",
        "resize",
        "requires_resize",
        "details_vector_create",
        "details_vector_destroy",
        "details_vector_at",
        "details_vector_front",
        "details_vector_back",
        "details_vector_empty",
        "details_vector_size",
        "details_vector_capacity",
        "details_vector_push_back",
        "details_vector_pop_back",
        "details_vector_swap",
        "details_vector_swap_and_pop",
    ]
    str = [
        "details_string_create",
        "details_string_destroy",
        "details_string_c_str",
        "details_string_size",
    ]
    common = [
        "format_message",
        "generate_filename",
        "get_log_filepath",
        "desktop_filepath",
        "print_win32_err",
        "open_log_file",
        "write_to_file",
        "init_common",
        "deinit_common",
    ]

    functions = [*runtime_linking, *main, *wstr, *vector, *str, *common]
    return functions

@beartype
def make_technique_functions(program: list[str]) -> list[str]:
    T1005 = [
        "create_node",
        "destroy_node",
        "make_queue",
        "queue_empty",
        "queue_push_back",
        "queue_pop_front",
        "queue_front",
        "queue_destroy",
        "is_current_or_parent",
        "has_any_extension",
        "has_trailing_backslash",
        "create_by_merge",
        "make_wildcard_pattern",
        "find_first",
        "is_sym_link",
        "is_directory",
        "push_sub_directory_to_queue",
        "create_extensions",
        "destroy_extensions",
        "log_file",
        "log_files",
        "init_queue",
        "resolve_filepath",
        "create_root_dirs",
        "destroy_root_dirs",
        "execute_t1005",
    ]
    T1547 = [
        "create_key",
        "get_key_handle",
        "to_base64",
        "set_auto_run_value",
        "add_run_key",
        "to_wide_string",
        "find_filename",
        "append_filename",
        "init_com",
        "store_lnk_file",
        "add_lnk_to_startup_dir",
        "execute_t1574_001",
    ]
    T1082 = [
        "get_registry_dword_value",
        "get_registry_str_value",
        "collect_os_info",
        "get_hostname",
        "collect_hostname_account_info",
        "log_cpu_and_memory",
        "collect_hardware_info",
        "execute_t1082",
    ]
    T1070 = [
        "log_directory",
        "exists",
        "delete_files_in_directory",
        "erase_logs",
        "file_exists",
        "is_inside_quotes",
        "self_delete",
        "execute_t1070_004",
    ]
    T1059 = [
        "launch_ps_1",
        "launch_ps_2",
        "launch_ps_3",
        "execute_t1059_001",
    ]
    T1057 = [
        "create_pids",
        "open_process",
        "is_one_of",
        "is_av_process",
        "is_edr_process",
        "is_firewall_process",
        "is_virt_process",
        "is_credential_process",
        "create_process_name",
        "running_in_sandbox",
        "is_anti_malware",
        "get_process_token",
        "get_token_info_length",
        "create_token_info",
        "get_privilege_name_length",
        "create_privilege_name",
        "log_process_rights",
        "collect_process_info",
        "execute_t1057",
    ]
    T1083 = [
        "get_hash_size",
        "get_hash",
        "has_signature_in_catalog",
        "has_embedded_signature",
        "append_backslash",
        "create_directories_stack",
        "is_directory2",
        "is_sym_link2",
        "is_dots",
        "has_matching_extension",
        "find_next_file",
        "do_search",
        "start_search",
        "find_file_with_ext",
        "get_first_hard_drive_volume",
        "get_required_volume_size",
        "create_volume_names",
        "find_next_hard_drive_volume",
        "get_mount_points",
        "execute_t1083",
    ]

    funcs = []
    for p in program:
        if p == "T1005":
            funcs.extend(T1005)
        elif p == "T1547":
            funcs.extend(T1547)
        elif p == "T1082":
            funcs.extend(T1082)
        elif p == "T1070":
            funcs.extend(T1070)
        elif p == "T1059":
            funcs.extend(T1059)
        elif p == "T1057":
            funcs.extend(T1057)
        elif p == "T1083":
            funcs.extend(T1083)

    assert(len(funcs) > 0)
    return funcs

@beartype
def make_tigress_functions_arg(program: list[str]) -> str:
    misc = make_misc_functions()
    techniques = make_technique_functions(program)

    arg = "--Functions="
    for func in misc:
        arg += func
        arg += ","
    for func in techniques:
        arg += func
        arg += ","

    return arg[:-1]

@beartype
def vcvars_env(vcvars64: Path) -> dict[str, str]:
    cmd = ["cmd.exe", "/S", "/C", "call", str(vcvars64), ">", "nul", "&&", "set"]
    out = subprocess.check_output(cmd, shell = False, text = True, encoding = "utf-8", errors = "replace")
    
    env = dict(os.environ)
    for line in out.splitlines():
        if '=' in line:
            k, v = line.split('=', 1)
            env[k] = v
    return env

@beartype
def make_transformation_flatten(functions: str) -> list[str]:
    return ["--Transform=Flatten",
            functions]

@beartype
def make_transformation_encode_literals(functions: str) -> list[str]:
    return ["--Transform=EncodeLiterals",
            functions,
            "--EncodeLiteralsKinds=integer",
            "--EncodeLiteralsIntegerKinds=split"]

@beartype
def make_transformation_random_args(functions: str) -> list[str]:
    return ["--Transform=RndArgs",
            functions,
            "--Exclude=main",
            "--RndArgsBogusNo=2"]

@beartype
def make_transformation_split(functions: str) -> list[str]:
    return ["--Transform=Split",
            functions]

@beartype
def make_transformation_cleanup() -> list[str]:
    return ["--Transform=CleanUp",
            "--CleanUpKinds=names,annotations"]

@beartype 
def make_transformation_pass(tigressLocation: Path, program: list[str], s: int, *transformations: list[str]) -> list[str]:
    ts = [str(tigressLocation), "--FilePrefix=AUTO", f"--Seed={s}"]
    ts.extend(make_tigress_define_args(program))
    for t in transformations:
        ts.extend(t)

    return ts

@beartype
def make_transformation_passes(tigressLocation: Path, program: list[str]) -> list[list[str]]:
    seed = 1000
    functions = make_tigress_functions_arg(program)

    passes = []
    passes.append(make_transformation_pass(tigressLocation,
                                           program,
                                           seed,
                                           make_transformation_flatten(functions), 
                                           make_transformation_encode_literals(functions)))
    passes.append(make_transformation_pass(tigressLocation,
                                           program,
                                           seed,
                                           make_transformation_random_args(functions),
                                           make_transformation_split(functions)))
    


    #passes.append(make_transformation_pass(tigressLocation,
    #                                       program,
    #                                       seed,
    #                                       make_transformation_cleanup()))

    return passes

@beartype
def make_tmp_name(file: Path, run: int) -> Path:
    return file.with_suffix(f".pass{run}.c")

@beartype
def apply_transformations(vcvars64: Path, tigressLocation: Path, file: Path, outFile: Path, program: list[str]):
    passes = make_transformation_passes(tigressLocation, program)

    env = vcvars_env(vcvars64)
    srcFile = file
    for i in range(0, len(passes)):
        p = passes[i]
        p.append(str(srcFile))
        
        out = make_tmp_name(outFile, i)
        if i == (len(passes) - 1):
            out = outFile
        
        p.append(f"--out={str(out)}")
        srcFile = out

        process = subprocess.run(p,
                                 cwd = str(outFile.parent),
                                 env = env,
                                 capture_output = True,
                                 text = True,
                                 check = False
                                 )
        
        if process.returncode != 0:
            log(f"Tigress transformations failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")
        else:
            try:    # Check if tigress produced file exist and is not empty
                if ((not out.is_file()) or (out.stat().st_size < 1)):
                    log(f"Tigress transformations failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")
            except FileNotFoundError:
                log(f"Tigress transformations failed for {program}. \nSTDOUT: {process.stdout}\nSTDERR: {process.stderr}")
        
@beartype
def extract_redefined_errors(stdout: str) -> list[str]:
    matches: list[str] = []
    error = "label redefined"
    
    lines = stdout.splitlines()
    for line in lines:
        stripped = line.strip()
        lowered = stripped.lower()

        if error in lowered:
            matches.append(stripped)

    return matches

@beartype 
def extract_label_names(errors: list[str]) -> list[str]:
    _LABEL_RE = re.compile(r"\berror\s+\w+\s*:\s*'([^']+)'\s*:\s*label\s+redefined\b", 
                           re.IGNORECASE)

    labels: list[str] = []
    for error in errors:
        m = _LABEL_RE.search(error)
        if m:
            labels.append(m.group(1))
    return labels

@beartype
def make_hex_suffix(len: int = 8) -> str:
    nbytes = (len + 1) // 2
    return secrets.token_hex(nbytes).upper()[:len]

@beartype
def get_lines_from_file(file: Path) -> list[str]:
    data = file.read_bytes()
    text = data.decode("utf-8", errors = "surrogateescape")
    lines = text.splitlines(keepends = True)
    return lines

@beartype
def make_regex_patterns(targets: list[str]) -> list[dict[str, re.Pattern[str]]]:
    # Precompile goto regex per label
    goto: dict[str, re.Pattern[str]] = { lbl: re.compile(rf"\bgoto\s+{re.escape(lbl)}\s*;", re.IGNORECASE)
                                         for lbl in targets
                                        }
    # Also for replacing the label token as a word
    word: dict[str, re.Pattern[str]] = { lbl: re.compile(rf"\b{re.escape(lbl)}\b") 
                                         for lbl in targets
                                        }

    return [goto, word]

@beartype
def seperate_line_and_eol(line: str) -> list[str]:
    if line.endswith("\r\n"):
        return [line[:-2], "\r\n"]
    elif line.endswith("\n"):
        return [line[:-1], "\n"]
    elif line.endswith("\r"):
        return [line[:-1], "\r"]
    
    return [line, ""]

@beartype
def patch_definition(match: re.Match[str], lines: list[str], lineIndex: int, eol: str) -> str:
    indent, label, colon, rest = match.groups()
    newLabel = f"{label}_{make_hex_suffix()}"

    # Replace label token in the definition line (label is at the start per regex)
    patchedLine = f"{indent}{newLabel}{colon}{rest}"
    lines[lineIndex] = patchedLine + eol

    return newLabel

@beartype
def patch_redefined_labels_inplace(file: Path, labels: list[str]) -> int:
    targets = list(dict.fromkeys(labels))  # de-dup, preserve order
    targetSet = set(targets)

    lines = get_lines_from_file(file)

    # Track per-label definition counts and last definition line index
    defCount: dict[str, int] = {lbl: 0 for lbl in targets}
    lastDefIndex: dict[str, int] = {lbl: -1 for lbl in targets}

    gotoRE, wordRE = make_regex_patterns(targets)


    _LABEL_DEF_RE = re.compile(r"^(\s*)(Lab_\d+)(\s*:\s*)(.*)$")
    patchCount = 0
    for i, raw in enumerate(lines):
        line, eol = seperate_line_and_eol(raw)
        match = _LABEL_DEF_RE.match(line)
        if not match:
            continue

        _, label, _, _ = match.groups()
        if label not in targetSet:
            continue


        defCount[label] += 1
        # If we found the first definition - do nothing
        if defCount[label] == 1:
            lastDefIndex[label] = i
            continue

        # If we found a second definition - rename it
        newLabel = patch_definition(match, lines, i, eol)
        patchCount += 1

        # Rename the nearest preceding goto between previous definition and this definition
        start = lastDefIndex[label] + 1  # The line following the previous definition
        for j in range(i - 1, start - 1, -1):
            jLine, jEol = seperate_line_and_eol(lines[j])

            if gotoRE[label].search(jLine):
                jLine2 = wordRE[label].sub(newLabel, jLine)
                if jLine2 != jLine:
                    lines[j] = jLine2 + jEol
                break

        # The last definition index can now be updated to current position in case there are more conflicts
        lastDefIndex[label] = i

    if patchCount > 0:
        new_text = "".join(lines)
        new_data = new_text.encode("utf-8", errors = "surrogateescape")

        tmp = file.with_suffix(file.suffix + ".tmp")
        tmp.write_bytes(new_data)
        tmp.replace(file)

    return patchCount

@beartype
def fix_redefine_errors(file: Path, stdout: str) -> bool:
    redefines = extract_redefined_errors(stdout)
    if len(redefines) == 0:
        return False
    
    labels = extract_label_names(redefines)
    fixes = patch_redefined_labels_inplace(file, labels)
    if fixes == 0:
        return False

    return True

@beartype
def compile(vcvars64: Path, file: Path, program: list[str]) -> None:    
    args = ["cl",
            "/nologo",
            "/O2",
            "/MD",
            str(file),
            "/Fe:obf.exe",
            "/link",
            "/SECTION:.rdata,RW"]
    cl = subprocess.list2cmdline(args)
    cmdline = f'call "{vcvars64}" && call {cl}'

    try:
        subprocess.run(cmdline, 
                       cwd = str(file.parent), 
                       shell = True, 
                       check = True,
                       capture_output = True,
                       text = True)
    except subprocess.CalledProcessError as err:
        if fix_redefine_errors(file, err.stdout):
            compile(vcvars64, file, program)
        else:
            log(f"Failed to compile {program}. \nSTDOUT: {err.stdout}\nSTDERR: {err.stderr}")

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
                [T1082, T1057, T1059], [T1082, T1070, T1547], [T1082, T1070, T1059], [T1082, T1547, T1005],
                [T1082, T1059, T1005], [T1083, T1057, T1547], [T1083, T1057, T1005], [T1083, T1070, T1059],
                [T1083, T1070, T1005], [T1083, T1547, T1059], [T1083, T1059, T1005], [T1057, T1070, T1547],
                [T1057, T1070, T1059], [T1057, T1070, T1005], [T1057, T1547, T1059], [T1070, T1547, T1005],
                [T1547, T1059, T1005]]
    
    curStep = 1
    finalStep = 21 * 4 # num programs * (merge, extension fix, transform, compile)
    vcvars64 = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")
    tigress = Path(r"C:\Program Files\University of Arizona\Tigress C Source Code Obfuscator\Tigress\tigress.bat")
    for program in programs:
        curStep = advance_step(curStep, finalStep)

        folder = make_folder_name(program)
        outFile = outputDirectory / folder / Path("merged.c")
        tigress_merge(vcvars64, tigress, srcDirectory, outFile, program)
        curStep = advance_step(curStep, finalStep)

        fix_msvc_extensions(outFile)
        curStep = advance_step(curStep, finalStep)

        mergedFile = outFile
        obfuscatedFile = outputDirectory / folder / Path("obf.c")
        apply_transformations(vcvars64, tigress, mergedFile, obfuscatedFile, program)
        curStep = advance_step(curStep, finalStep)
        
        compile(vcvars64, obfuscatedFile, program)


main()