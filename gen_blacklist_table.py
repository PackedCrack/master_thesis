from pathlib import Path


def main():
    scriptDir = Path(__file__).resolve().parent
    roots = ["output", "output-baseline", "output-tigress"]

    counts = {}

    for rootName in roots:
        rootPath = scriptDir / rootName
        if not rootPath.is_dir():
            continue

        for blacklistFile in rootPath.rglob("blacklist.txt"):
            with blacklistFile.open("r", encoding = "utf-8", errors = "replace") as file:
                for line in file:
                    key = line.rstrip("\r\n")
                    counts[key] = counts.get(key, 0) + 1

    outputFile = scriptDir / "blacklist_table.txt"
    with outputFile.open("w", encoding = "utf-8") as file:
        file.write(r"\begin{longtable}{p{0.82\textwidth}r}" + "\n")
        file.write("\t" + r"\caption{All IDA recognized functions that were blacklisted.}\label{tab:blacklist_full}\\" + "\n")
        file.write("\t" + r"\hline" + "\n")
        file.write("\t" + r"\textbf{Symbol} & \textbf{Count} \\" + "\n")
        file.write("\t" + r"\hline" + "\n")
        file.write("" + r"\endfirsthead" + "\n")

        file.write("\t" + r"\hline" + "\n")
        file.write("\t" + r"\textbf{Symbol} & \textbf{Count} \\" + "\n")
        file.write("\t" + r"\hline" + "\n")
        file.write("" + r"\endhead" + "\n")

        file.write("\t" + r"\hline" + "\n")
        file.write("" + r"\endfoot" + "\n")

        file.write("\t" + r"\hline" + "\n")
        file.write("" + r"\endlastfoot" + "\n")

        for key, value in sorted(counts.items(), key=lambda item: (-item[1], item[0])):
            symbol = str(key)
            symbol = symbol.replace("\\", r"\textbackslash{}")
            symbol = symbol.replace("{", r"\{")
            symbol = symbol.replace("}", r"\}")
            symbol = symbol.replace("&", r"\&")
            symbol = symbol.replace("%", r"\%")
            symbol = symbol.replace("#", r"\#")
            symbol = symbol.replace("$", r"\$")
            symbol = symbol.replace("^", r"\^{}")
            symbol = symbol.replace("~", r"\~{}")

            # add legal breakpoints
            symbol = symbol.replace("_", r"\_\allowbreak{}")
            symbol = symbol.replace(".", r".\allowbreak{}")
            symbol = symbol.replace("/", r"/\allowbreak{}")
            symbol = symbol.replace(":", r":\allowbreak{}")
            symbol = symbol.replace("-", r"-\allowbreak{}")

            file.write("\t" + rf"\texttt{{{symbol}}} & {value} \\" + "\n")

        file.write(r"\end{longtable}" + "\n")

if __name__ == "__main__":
    main()