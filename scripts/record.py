import time
from decimal import Decimal

import json
import websocket   # pip install websocket-client

SYMBOL = "btcusdt"
URL = f"wss://stream.binance.com:9443/ws/{SYMBOL}@depth20@100ms"
TICK = Decimal("0.01")      # pasul de preț; verifică în exchangeInfo (tickSize)
QTY_SCALE = 10**8           # 8 zecimale la cantitate
DURATION_S = 30 * 60
OUT = "data/session2.csv"


def main():
    ws = websocket.create_connection(URL, timeout=10)
    start = time.time()
    snapshots = 0
    try:
        with open(OUT, "w", newline="") as f:
            f.write("ts_ms,side,price_ticks,qty\n")
            while time.time() - start < DURATION_S:
                msg = json.loads(ws.recv())
                ts = int(time.time() * 1000)   # timp local de primire
                for side, levels in (("B", msg["bids"]), ("A", msg["asks"])):
                    for p, q in levels:
                        price = Decimal(p)
                        assert price % TICK == 0, f"preț care nu e multiplu de tick: {p}"
                        f.write(f"{ts},{side},{int(price / TICK)},{int(Decimal(q) * QTY_SCALE)}\n")
                snapshots += 1
                if snapshots % 100 == 0:
                    f.flush()
    except KeyboardInterrupt:
        pass
    finally:
        ws.close()
    print(f"{snapshots} snapshot-uri scrise în {OUT}")


if __name__ == "__main__":
    main()