import pandas as pd
import openpyxl
import bambi as bmb
import arviz as az
from beartype import beartype
from pathlib import Path


@beartype
def extract_xlsx_data(sheet) -> list[list]:
    programIDs = []
    configIDs = []
    jsd = []

    configID = 0
    for row in sheet.iter_rows(min_row = 2, min_col = 2, values_only = True):
        configID += 1
        programID = 0
        for cell in row:
            assert cell is not None
            programID += 1
            programIDs.append(programID)
            configIDs.append(configID)
            jsd.append(cell)

    return [jsd, programIDs, configIDs]

@beartype
def get_fixed_effect_keys(configID: int) -> list[str]:
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
    
    keys = {"/homeparams": "F1_homeparams",
            "/Qspectre-load": "F2_Qspectre_load",
            "/Gs0": "F3_Gs0",
            "/favor:INTEL64": "F4_favor_INTEL64",
            "/Gh": "F5_Gh",
            "/QIntel-jcc-erratum": "F6_QIntel_jcc_erratum",
            "/guard:cf": "F7_guard_cf",
            "/GL": "F8_GL"}
    
    usedFlags = flags[configID - 1]
    k = []
    for flag in usedFlags:
        k.append(keys[flag])

    return k

@beartype
def make_fixed_effects(configIDs: list[int]) -> dict:
    fixedEffects = {"F1_homeparams": [],
                    "F2_Qspectre_load": [],
                    "F3_Gs0": [],
                    "F4_favor_INTEL64": [],
                    "F5_Gh": [],
                    "F6_QIntel_jcc_erratum": [],
                    "F7_guard_cf": [],
                    "F8_GL": []}
    
    for id in configIDs:
        # Make all flags off by default
        for k in fixedEffects:
            fixedEffects[k].append(-1)

        # Then overwrite those that are used
        usedFlags = get_fixed_effect_keys(id)
        for flag in usedFlags:
            fixedEffects[flag][-1] = 1

    return fixedEffects

@beartype
def make_dataframe() -> pd.DataFrame:
    xlsx_path = "jsd_results.xlsx"
    wb = openpyxl.load_workbook(xlsx_path, data_only = True, read_only = True)
    sheet = wb["Compiler Flags"]

    jsd, programIDs, configIDs = extract_xlsx_data(sheet)
    assert len(jsd) == len(programIDs)
    assert len(programIDs) == len(configIDs)

    data = {"jsd": jsd,
            "program_id": programIDs}
    fixedEffects = make_fixed_effects(configIDs)
    data.update(fixedEffects)
    
    wb.close()
    
    return pd.DataFrame(data)


@beartype
def main():
    modelFile = Path(__file__).parent.resolve() / Path("model.nc")

    df = make_dataframe()
    df["program_id"] = df["program_id"].astype("category")

    formula = (
            "jsd ~ "
            "F1_homeparams + F2_Qspectre_load + F3_Gs0 + F4_favor_INTEL64 + "
            "F5_Gh + F6_QIntel_jcc_erratum + F7_guard_cf + F8_GL + "
            "(1|program_id)"
        )
    
    idata = None
    if not modelFile.exists():
        model = bmb.Model(formula, df, family = "beta", link = "logit")
        idata = model.fit(draws = 10000, tune = 10000, chains = 8, cores = 8, target_accept = 0.95, random_seed = 42)
        idata.to_netcdf(str(modelFile))
    else:
        model = bmb.Model(formula, df, family = "beta", link = "logit")
        idata = az.from_netcdf(str(modelFile))


    print(az.summary(
        idata,
        var_names=[
            "Intercept",
            "F1_homeparams",
            "F2_Qspectre_load",
            "F3_Gs0",
            "F4_favor_INTEL64",
            "F5_Gh",
            "F6_QIntel_jcc_erratum",
            "F7_guard_cf",
            "F8_GL",
            "1|program_id_sigma",
        ],
        hdi_prob=0.95,
        round_to = 5
    ))

if __name__ == "__main__":
    main()