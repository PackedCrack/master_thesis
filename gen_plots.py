from openpyxl import Workbook
from openpyxl import load_workbook
from beartype import beartype
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
import numpy as np
from statistics import pstdev


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
        plot_heatmap(values, title, colorbarLabel, colTitles, rowTitles)

@beartype
def create_histogram(data: dict, numBins: int, title: str, xlabel: str, ylabel: str):
    values = get_matrix_values_as_1d(data)
    fig, ax = plt.subplots(figsize = (20, 14))
    ax.hist(values, bins = numBins, range = (0.0, 1.0), edgecolor = "black", linewidth = 0.8)

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
    create_histogram(tigressData, 40, "Overall JSD Distribution: Tigress", "Jensen-Shannon Divergence", "Count")
    create_histogram(flagData, 40, "Overall JSD Distribution: Compiler Flags", "Jensen-Shannon Divergence", "Count")
    create_heatmap(tigressData, "Tigress Drift", "Jensen-Shannon Divergence")
    create_heatmap(flagData, "Flag Drift", "Jensen-Shannon Divergence")

@beartype
def calc_means_and_std(matrix: list[list]):
    means = []
    stds = []
    for row in matrix:
        means.append(sum(row) / len(row))
        stds.append(pstdev(row))

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

def main():
    filePath = Path(__file__).resolve().parent / Path("jsd_results_auto.xlsx")

    wb = load_workbook(filePath, data_only = True, read_only = True)
    
    tigressData = extract_sheet_matrix_data(wb, "Tigress")
    flagData = extract_sheet_matrix_data(wb, "Compiler Flags")
    plot_dataset(tigressData, flagData)

    rq1(flagData)

main()