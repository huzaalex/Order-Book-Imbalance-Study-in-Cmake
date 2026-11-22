import pandas as pd

raw = pd.read_csv("data/btcusdt_depth20.csv")
feat = pd.read_csv("data/features.csv")

for i in [0, 100, 2000, 5000, 8000]:
    ts = feat["ts_ms"].iloc[i]
    snap = raw[raw["ts_ms"] == ts]
    bids = snap[snap["side"] == "B"].sort_values("price_ticks", ascending=False).head(5)["qty"].sum()
    asks = snap[snap["side"] == "A"].sort_values("price_ticks").head(5)["qty"].sum()
    print(i, "python:", round((bids - asks) / (bids + asks), 6), "c++:", round(feat["imb5"].iloc[i], 6), "rows:", len(snap))