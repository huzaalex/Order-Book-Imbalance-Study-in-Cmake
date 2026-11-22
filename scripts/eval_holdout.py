import numpy as np
import pandas as pd

df = pd.read_csv("data/features2.csv").iloc[:-1]
THRESH, K, L = 0.673, 10, 3   # fixate din sesiunea 1, nu le mai schimbi
mid = df["mid_ticks"]
dm = mid.shift(-(L + K)) - mid.shift(-L)
ok = dm.notna()
sig = np.sign(df["imb5"] - THRESH)
edge = sig[ok] * dm[ok]

print("avantaj mediu (ticks):", round(edge.mean(), 2))
print("avantaj mediu (bps):", round(edge.mean() / mid[ok].mean() * 1e4, 3))
print("fracție long:", round((sig[ok] > 0).mean(), 2))
chunk = np.arange(len(df)) * 3 // len(df)
for c in range(3):
    m = ok & (chunk == c)
    print("chunk", c, round((sig[m] * dm[m]).mean(), 2))
    
edge_no = edge.iloc[::K]  # ferestre fără suprapunere
se = edge_no.std() / np.sqrt(len(edge_no))
print("n independent:", len(edge_no), "| medie:", round(edge_no.mean(), 2),
      "| eroare standard:", round(se, 2), "| t:", round(edge_no.mean() / se, 1))