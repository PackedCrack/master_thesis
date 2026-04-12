from openpyxl import Workbook
from openpyxl import load_workbook
from beartype import beartype
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
import numpy as np
from statistics import stdev
import math
from dataclasses import dataclass


@beartype
def extract_sheet_matrix_data(wb: Workbook, sheetName: str):
    sheet = wb[sheetName]

    # First row (excluding A1 since its empty)
    colTitles = list(next(sheet.iter_rows(min_row = 1, max_row = 1, min_col = 2, max_col = sheet.max_column, values_only = True)))

    # data[rowTitle][columnTitle] = cell_value
    data = {}
    for row in sheet.iter_rows(min_row = 2,  max_row = sheet.max_row, min_col = 1, max_col = sheet.max_column, values_only = True):
        # Title in the first column
        rowTitle = row[0]
        # The remaining columns hold the data
        values = row[1:]

        # Row title is None for A1
        if rowTitle is not None:
            data[rowTitle] = dict(zip(colTitles, values))

    return data

@beartype
def get_matrix_titles(data: dict) -> list[list[str]]:
    rowTitles = list(data.keys())
    colTitles = list(next(iter(data.values())).keys())

    return [colTitles, rowTitles]

@beartype
def get_matrix_values(data: dict) -> list[list]:
    colTitles, rowTitles = get_matrix_titles(data)

    values = [
        [data[row][col] for col in colTitles]
        for row in rowTitles
    ]

    return values

@beartype
def get_matrix_values_by_column(data: dict) -> list[list]:
    colTitles, rowTitles = get_matrix_titles(data)

    values = [
        [data[row][col] for row in rowTitles]
        for col in colTitles
    ]

    return values

@beartype
def get_matrix_values_as_1d(data: dict) -> list:
    values = [
        data[row][col]
        for row in data
        for col in data[row]
    ]

    return values

@beartype
def plot_heatmap(values: list[list], title: str, colorbarLabel: str, colTitles: list[str], rowTitles: list[str]):
    fig, ax = plt.subplots(figsize = (20, 14))

    im = ax.imshow(values, aspect = "auto", cmap = "Reds", vmin = 0.0, vmax = 1.0)

    ax.set_xticks(range(len(colTitles)))
    ax.set_xticklabels(colTitles, rotation = 45, ha = "right")
    ax.set_yticks(range(len(rowTitles)))
    ax.set_yticklabels(rowTitles)

    ax.set_xticks([x - 0.5 for x in range(1, len(colTitles))], minor = True)
    ax.set_yticks([y - 0.5 for y in range(1, len(rowTitles))], minor = True)
    ax.grid(which = "minor", color = "black", linestyle = "-", linewidth = 1)
    ax.tick_params(which = "minor", bottom = False, left = False)

    for i in range(len(rowTitles)):
        for j in range(len(colTitles)):
            ax.text(j, i, f"{values[i][j]:.3f}", ha = "center", va = "center", color = "black")

    fig.colorbar(im, ax = ax, label = colorbarLabel)
    ax.set_title(title)

    plt.tight_layout()
    plt.show(block = True)

@beartype
def create_heatmap(data: dict, title: str, colorbarLabel: str):
    colTitles, rowTitles = get_matrix_titles(data)
    values = get_matrix_values(data)

    # Hack because 64 rows per plot is too much
    if title == "Flag Drift":
        mid = len(values) // 2
        left = values[:mid]
        plot_heatmap(left, title, colorbarLabel, colTitles, rowTitles[:mid])
        right = values[mid:]
        plot_heatmap(right, title, colorbarLabel, colTitles, rowTitles[mid:])
    else:
        for i in range(0, len(rowTitles)):
            rowTitles[i] = f"Run: {i + 1}"
        plot_heatmap(values, title, colorbarLabel, colTitles, rowTitles)

@beartype
def create_histogram(data: dict, numBins: int, title: str, xlabel: str, ylabel: str, colour: str):
    values = get_matrix_values_as_1d(data)
    fig, ax = plt.subplots(figsize = (20, 14))
    ax.hist(values, bins = numBins, range = (0.0, 1.0), edgecolor = "black", linewidth = 0.8, color = colour)

    ax.set_xlim(0.0, 1.0)
    ax.xaxis.set_major_locator(MultipleLocator(0.025))
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.2f'))

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    plt.show(block = True)

@beartype
def create_box_plot(data: dict, title: str, ylabel: str, boxLabels: list[str]):
    values = get_matrix_values_by_column(data)


    fig, ax = plt.subplots(figsize=(20, 14))
    medProps = {
        "color": "green",
        "linewidth": 2
    }
    props = {
        "marker": "x",
        "markerfacecolor": "red",
        "markeredgecolor": "black",
        "markersize": 10
    }
    ax.boxplot(values, tick_labels = boxLabels, showmeans = True, meanprops = props, medianprops = medProps)

    ax.set_ylim(0, 1)
    plt.xticks(rotation = 45, ha = "right")

    ax.set_ylabel(ylabel)
    ax.set_title(title)
    plt.tight_layout()
    plt.show(block = True)

@beartype
def plot_dataset(tigressData: dict, flagData: dict):
    create_histogram(tigressData, 40, "Overall JSD Distribution: Tigress", "Jensen-Shannon Divergence", "Count", "red")
    create_histogram(flagData, 40, "Overall JSD Distribution: Compiler Flags", "Jensen-Shannon Divergence", "Count", "blue")
    create_heatmap(tigressData, "Tigress Drift", "Jensen-Shannon Divergence")
    create_heatmap(flagData, "Flag Drift", "Jensen-Shannon Divergence")

@beartype
def calc_means_and_std(matrix: list[list]):
    means = []
    stds = []
    for row in matrix:
        means.append(sum(row) / len(row))
        stds.append(stdev(row))

    return means, stds

@beartype
def calc_90th_percentiles(matrix: list[list]):
    percentiles = []
    for row in matrix:
        percentiles.append(np.percentile(row, 90))

    return percentiles

@beartype
def rq1(flagData: dict):
    data = get_matrix_values_by_column(flagData)
    means, stds = calc_means_and_std(data)
    percentiles = calc_90th_percentiles(data)

    assert len(means) == 21
    assert len(stds) == len(means)
    assert len(percentiles) == len(means)

    # Make latex table
    out = Path(__file__).parent.resolve() / Path("rq1_table.txt")
    with out.open(mode = "+w", encoding = "utf-8") as file:
        file.write(r"\begin{table}[htbp]" + "\n")
        file.write("\t" + r"\centering" + "\n")
        file.write("\t" + r"\begin{tabular}{lrrr}" + "\n")
        file.write("\t\t" + r"\textbf{Program ID} & \textbf{90th percentile} & \textbf{mean} & \textbf{Standard Deviation} \\" + "\n")
        file.write("\t\t" + r"\hline" + "\n")

        for i in range(0, len(means)):
            file.write("\t\t" + rf"\textbf{{{i + 1}}} & {percentiles[i]:.8f} & {means[i]:.8f} & {stds[i]:.8f} \\" + "\n")
            file.write("\t\t" + r"\hline" + "\n")
            
        file.write("\t" + r"\end{tabular}" + "\n")
        file.write("\t" + r"\caption{RQ1. Presented numbers are rounded to $8$ decimals.}" + "\n")
        file.write("\t" + r"\label{tab:rq1}" + "\n")
        file.write(r"\end{table}" + "\n")

    labels = []
    for i in range(1, 22):
        labels.append(f"Program {i}")
    create_box_plot(flagData, "Compiler Flag Drift Per Program", "Jensen Shannon Divergence", labels)

@beartype
def rq2(tigressData: dict):
    data = get_matrix_values_by_column(tigressData)
    means, stds = calc_means_and_std(data)
    percentiles = calc_90th_percentiles(data)

    # Make latex table
    out = Path(__file__).parent.resolve() / Path("rq2_table.txt")
    with out.open(mode = "+w", encoding = "utf-8") as file:
        file.write(r"\begin{table}[htbp]" + "\n")
        file.write("\t" + r"\centering" + "\n")
        file.write("\t" + r"\begin{tabular}{lrrr}" + "\n")
        file.write("\t\t" + r"\textbf{Program ID} & \textbf{90th percentile} & \textbf{mean} & \textbf{Standard Deviation} \\" + "\n")
        file.write("\t\t" + r"\hline" + "\n")

        for i in range(0, len(means)):
            file.write("\t\t" + rf"\textbf{{{i + 1}}} & {percentiles[i]:.8f} & {means[i]:.8f} & {stds[i]:.8f} \\" + "\n")
            file.write("\t\t" + r"\hline" + "\n")
            
        file.write("\t" + r"\end{tabular}" + "\n")
        file.write("\t" + r"\caption{RQ2. Presented numbers are rounded to $8$ decimals.}" + "\n")
        file.write("\t" + r"\label{tab:rq2}" + "\n")
        file.write(r"\end{table}" + "\n")

@beartype
def calc_signed_mean_diff(flagData: list[list], tigressData: list[list]) -> list[float]:
    tmeans, tstds = calc_means_and_std(tigressData)
    fmeans, fstds = calc_means_and_std(flagData)

    assert len(tmeans) == len(fmeans)
    diffs = []
    for i in range(0, len(tmeans)):
        diffs.append(tmeans[i] - fmeans[i])

    return diffs

@dataclass
class Drift:
    value: float
    rowID: int
    colID: int
    source: str

@beartype
def get_highest_value(matrix: list[list[float]], top: int, source: str) -> list[Drift]:
    values = []
    for c, col in enumerate(matrix):
        for r, value in enumerate(col):
            # We don't want actual indices because it should map to the heatmap plots
            values.append(Drift(matrix[c][r], c + 1, r + 1, source))

    values.sort(key = lambda drift: drift.value)
    return values[-top:]

@beartype
def calc_prob_of_superiority(fdata: list[list[float]], tdata: list[list[float]]) -> list[float]:
    assert len(fdata) == len(tdata)

    probs: list[float] = []
    for i, (frow, trow) in enumerate(zip(fdata, tdata)):
        assert len(frow) != 0 and len(trow) != 0

        
        score = 0.0
        for x in trow:
            for y in frow:
                if x > y:
                    score += 1.0
                elif math.isclose(x, y):
                    score += 0.5

        probs.append(score / (len(trow) * len(frow)))

    return probs

@beartype
def calc_top_5_drift(fdata: list[list[float]], tdata: list[list[float]]) -> list[Drift]:
    fbest = get_highest_value(fdata, 5, "F")
    tbest = get_highest_value(tdata, 5, "T")

    best = fbest.copy()
    best.extend(tbest)

    best.sort(key = lambda drift: drift.value)
    return best[-5:]

@beartype
def create_rq3_dumbbell_plot(fdata: list[list], tdata: list[list], smd: list[float]):
    fmeans, _ = calc_means_and_std(fdata)
    tmeans, _ = calc_means_and_std(tdata)

    assert len(fmeans) == len(tmeans) == len(smd)

    rows = []
    for i in range(len(fmeans)):
        rows.append({
            "program": f"Program {i + 1}",
            "compiler_mean": fmeans[i],
            "tigress_mean": tmeans[i],
            "difference": smd[i]
        })

    y = np.arange(len(rows))
    compiler_means = [r["compiler_mean"] for r in rows]
    tigress_means = [r["tigress_mean"] for r in rows]
    labels = [
        f'{r["program"]}  (Δ={r["difference"]:.3f})'
        for r in rows
    ]

    fig, ax = plt.subplots(figsize=(12, 10))

    for i in range(len(rows)):
        ax.plot(
            [compiler_means[i], tigress_means[i]],
            [y[i], y[i]],
            linewidth = 1.5,
            color = "black",
            zorder = 1
        )

    ax.scatter(compiler_means, y, s= 120, marker = "o", label = "Compiler mean JSD", zorder = 3, color = "blue")
    ax.scatter(tigress_means, y, s= 120, marker = "o", label = "Tigress mean JSD", zorder = 3, color = "red")

    ax.set_yticks(y)
    ax.set_yticklabels(labels)
    ax.set_xlabel("Mean Jensen-Shannon Divergence")
    ax.set_ylabel("Program")
    ax.set_title("RQ3: Mean compiler vs Tigress drift by program")
    ax.set_xlim(0.0, 1.0)
    ax.xaxis.set_major_locator(MultipleLocator(0.1))
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.1f'))
    ax.invert_yaxis()
    ax.legend()
    plt.tight_layout()
    plt.show(block = True)

@beartype
def create_rq3_pos_bar_plot(probs: list[float]):
    rows = []
    for i, p in enumerate(probs):
        rows.append({
            "program": f"Program {i + 1}",
            "pos": p,
            "rest": 1.0 - p
        })

    rows.sort(key=lambda x: x["pos"], reverse = False)

    labels = [r["program"] for r in rows]
    posValues = np.array([r["pos"] for r in rows], dtype = float)
    restValues = np.array([r["rest"] for r in rows], dtype = float)
    y = np.arange(len(rows))

    fig, ax = plt.subplots(figsize = (12, 10))

    ax.barh(y, posValues, edgecolor = "black", label = "P(Tigress > Compiler)", color = "red")
    ax.barh(y, restValues, left = posValues, edgecolor = "black", label = "P(Tigress < Compiler)", color = "blue")

    ax.axvline(0.5, color = "black", linewidth = 1, linestyle = "--")

    ax.set_yticks(y)
    ax.set_yticklabels(labels)
    ax.set_xlim(0.0, 1.0)

    ax.xaxis.set_major_locator(MultipleLocator(0.1))
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.1f'))

    ax.set_xlabel("Probability scale")
    ax.set_ylabel("Program ID")
    ax.set_title("RQ3: Probability of superiority by program")
    ax.legend()

    plt.tight_layout()
    plt.show(block = True)

@beartype
def rq3(flagData: dict, tigressData: dict):
    fdata = get_matrix_values_by_column(flagData)
    tdata = get_matrix_values_by_column(tigressData)

    smd = calc_signed_mean_diff(fdata, tdata)
    probs = calc_prob_of_superiority(fdata, tdata)
    assert len(smd) == len(probs)
    overall = sum(probs) / len(probs)

    top5 = calc_top_5_drift(fdata, tdata)

    create_rq3_dumbbell_plot(fdata, tdata, smd)
    create_rq3_pos_bar_plot(probs)

   
    # Make latex table
    out = Path(__file__).parent.resolve() / Path("rq3_table.txt")
    with out.open(mode = "+w", encoding = "utf-8") as file:
        file.write(r"\begin{table}[htbp]" + "\n")
        file.write("\t" + r"\centering" + "\n")
        file.write("\t" + r"\begin{tabular}{lrr}" + "\n")
        file.write("\t\t" + r"\textbf{Program ID} & \textbf{Signed Mean Difference} & \textbf{POS} \\" + "\n")
        file.write("\t\t" + r"\hline" + "\n")

        for i in range(0, len(smd)):
            file.write("\t\t" + rf"\textbf{{{i + 1}}} & {smd[i]:.8f} & {probs[i]:.8f} \\" + "\n")
            file.write("\t\t" + r"\hline" + "\n")
            
        file.write("\t" + r"\end{tabular}" + "\n")
        file.write("\t" + r"\caption{R3. Presented numbers are rounded to $8$ decimals.}" + "\n")
        file.write("\t" + r"\label{tab:rq3}" + "\n")
        file.write(r"\end{table}" + "\n")

    out = Path(__file__).parent.resolve() / Path("rq3_table2.txt")
    with out.open(mode = "+w", encoding = "utf-8") as file:
        file.write(r"\begin{table}[htbp]" + "\n")
        file.write("\t" + r"\centering" + "\n")
        file.write("\t" + r"\begin{tabular}{lrrr}" + "\n")
        file.write("\t\t" + r"\textbf{Source} & \textbf{JSD} & \textbf{Configuration/Run} & \textbf{Program ID} \\" + "\n")
        file.write("\t\t" + r"\hline" + "\n")

        for i in range(0, len(top5)):
            source = ""
            if top5[i].source == "T":
                source = "Tigress"
            else:
                source = "Compiler"
            file.write("\t\t" + rf"\textbf{{{source}}} & {top5[i].value:.8f} & {top5[i].rowID} & {top5[i].colID} \\" + "\n")
            file.write("\t\t" + r"\hline" + "\n")
            
        file.write("\t" + r"\end{tabular}" + "\n")
        file.write("\t" + r"\caption{RQ3 top 5. Presented numbers are rounded to $8$ decimals.}" + "\n")
        file.write("\t" + r"\label{tab:rq3}" + "\n")
        file.write(r"\end{table}" + "\n")


    fpercentiles = calc_90th_percentiles(fdata)
    tpercentiles = calc_90th_percentiles(tdata)
    assert len(fpercentiles) == len(tpercentiles)

    diff = []
    for i in range(0, len(fpercentiles)):
        diff.append(tpercentiles[i] - fpercentiles[i])

    out = Path(__file__).parent.resolve() / Path("rq3_table3.txt")
    with out.open(mode = "+w", encoding = "utf-8") as file:
        file.write(r"\begin{table}[htbp]" + "\n")
        file.write("\t" + r"\centering" + "\n")
        file.write("\t" + r"\begin{tabular}{lrrr}" + "\n")
        file.write("\t\t" + r"\textbf{Program ID} & \textbf{$90$th percentile (Tigress)} & \textbf{$90$th percentile (Compiler)} & \textbf{Δ} \\" + "\n")
        file.write("\t\t" + r"\hline" + "\n")

        for i in range(0, len(tpercentiles)):
            file.write("\t\t" + rf"\textbf{{{i + 1}}} & {tpercentiles[i]:.8f} & {fpercentiles[i]:.8f} & {diff[i]:.8f} \\" + "\n")
            file.write("\t\t" + r"\hline" + "\n")
            
        file.write("\t" + r"\end{tabular}" + "\n")
        file.write("\t" + r"\caption{R3. Presented numbers are rounded to $8$ decimals.}" + "\n")
        file.write("\t" + r"\label{tab:rq3}" + "\n")
        file.write(r"\end{table}" + "\n")
    
def main():
    filePath = Path(__file__).resolve().parent / Path("jsd_results.xlsx")

    wb = load_workbook(filePath, data_only = True, read_only = True)
    
    tigressData = extract_sheet_matrix_data(wb, "Tigress")
    flagData = extract_sheet_matrix_data(wb, "Compiler Flags")
    plot_dataset(tigressData, flagData)

    rq1(flagData)
    rq2(tigressData)
    rq3(flagData, tigressData)

main()