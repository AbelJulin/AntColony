"""Basic analysis of simulation_stats.csv.
Usage: python3 analysis/analyze.py simulation_stats.csv
"""
import sys
import pandas as pd
import matplotlib.pyplot as plt

path = sys.argv[1] if len(sys.argv) > 1 else "simulation_stats.csv"
df = pd.read_csv(path)
if df.empty:
    raise SystemExit("CSV is empty")

figures = [
    ("Population", "population"),
    ("Food stored", "food"),
    ("Territory", "territory"),
]
for title, column in figures:
    plt.figure()
    for colony, group in df.groupby("colony"):
        plt.plot(group["step"], group[column], label=f"Colony {colony}")
    plt.title(title)
    plt.xlabel("Simulation step")
    plt.ylabel(column)
    plt.legend()
    plt.tight_layout()
    plt.show()

print("Final state:")
print(df.sort_values("step").groupby("colony").tail(1).to_string(index=False))
