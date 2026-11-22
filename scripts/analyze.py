import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("data/features.csv")
df = df.iloc[:-1]  # ultimul snapshot poate fi incomplet

assert (df["spread_ticks"] >= 1).all(), "book încrucișat: bug"
assert df[["imb1", "imb5", "imb20"]].abs().max().max() <= 1.0
print("snapshots:", len(df), "| spread mediu (ticks):", round(df["spread_ticks"].mean(), 3))

HORIZONS = [1, 5, 10, 20, 50]  # în snapshot-uri, ~0.1 s fiecare
for k in HORIZONS:
    dm = df["mid_ticks"].shift(-k) - df["mid_ticks"]
    valid = dm.notna()
    moved = valid & (dm != 0)
    print(f"\nk={k}  (mid-ul se mișcă în {moved.sum() / valid.sum():.0%} din cazuri)")
    for col in ["imb1", "imb5", "imb20"]:
        corr = df.loc[valid, col].corr(dm[valid])
        hit = (np.sign(df.loc[moved, col]) == np.sign(dm[moved])).mean()
        edge = (np.sign(df.loc[valid, col]) * dm[valid]).mean()
        print(f"  {col}: corr={corr:+.3f}  hit={hit:.1%}  avantaj={edge:+.3f} ticks")

# decile de imbalance vs mișcarea medie a mid-ului
k = 10
d = df.copy()
d["dm"] = d["mid_ticks"].shift(-k) - d["mid_ticks"]
d = d.dropna(subset=["dm"])
d["bin"] = pd.qcut(d["imb5"], 10, duplicates="drop")
print("std(dm) =", round(d["dm"].std(), 1), "ticks")

g = d.groupby("bin", observed=True)["dm"]
se = g.std() / np.sqrt(g.count() / k)   # aproximare; ferestrele se suprapun
g.mean().plot(kind="bar", yerr=se, capsize=3)
plt.ylabel(f"mișcare medie mid după {k} snapshot-uri (ticks)")
plt.tight_layout()
plt.savefig("data/imb5_deciles.png")

print("\nMEDIANA pe decile:")
print(g.median().round(1))

# aceeași decilă, pe 3 bucăți de timp consecutive
d["chunk"] = np.arange(len(d)) * 3 // len(d)
print("\nmedia pe decilă × bucată de timp:")
print(d.groupby(["bin", "chunk"], observed=True)["dm"].mean().unstack().round(1))