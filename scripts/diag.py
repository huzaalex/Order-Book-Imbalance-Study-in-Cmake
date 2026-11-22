import numpy as np
import pandas as pd

df = pd.read_csv("data/features.csv").iloc[:-1]
mid = df["mid_ticks"]

print("mid început/sfârșit:", mid.iloc[0], mid.iloc[-1], "| deriva (ticks):", mid.iloc[-1] - mid.iloc[0])
print("min/max mid:", mid.min(), mid.max())
print("imb5 > 0 în", f"{(df.imb5 > 0).mean():.0%}", "din cazuri | mediană imb5:", round(df.imb5.median(), 3))

step = mid.diff().dropna()
moves = step[step != 0].abs()
print("mișcări la k=1:", len(moves), "| |mișcare| percentile 50/90/99/max:", moves.quantile([.5, .9, .99, 1]).round(1).tolist())

# semnal centrat (altfel e aproape mereu +1) vs baseline "mereu long", cu întârziere de execuție L
K = 10
sig = np.sign(df["imb5"] - df["imb5"].median())
chunk = pd.Series(np.arange(len(df)) * 3 // len(df), index=df.index)
print("\nK =", K, "(avantaj mediu, în ticks)")
for L in [0, 1, 3, 5]:   # întârziere în snapshot-uri (~0.1 s fiecare)
    dm = mid.shift(-(L + K)) - mid.shift(-L)
    ok = dm.notna()
    row = f"L={L}: semnal={ (sig[ok] * dm[ok]).mean():+.2f}  mereu long={dm[ok].mean():+.2f} |"
    for c in range(3):
        m = ok & (chunk == c)
        row += f" chunk{c}: {(sig[m] * dm[m]).mean():+.2f}/{dm[m].mean():+.2f}"
    print(row)