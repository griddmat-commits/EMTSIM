"""Plot the node voltages written by the C++ simulator (results.csv, events.csv).

Usage:
    python plot.py                 # show the plot in a window
    python plot.py --save out.png  # save it to a file instead

plot_nodes() is also used by the Python version (emtcls.Network.plotv).
"""
import argparse
import csv
import math

import numpy as np
import matplotlib.pyplot as plt


def plot_nodes(t_ms, V, names, faults=(), switches=()):
    """Plot node voltages.

    t_ms     : time in ms, shape (steps,)
    V        : node voltages, shape (steps, nodes)
    names    : label of each node
    faults   : (t_apply, t_clear or None) in seconds -> shaded
    switches : switching times in seconds -> dashed lines

    Up to 12 nodes are stacked in one column; larger networks are drawn as a
    grid of small plots.
    """
    n = V.shape[1]
    t_end = t_ms[-1]/1000.0
    big = n > 12
    if big:
        cols = math.ceil(math.sqrt(n*1.4))
        rows = math.ceil(n/cols)
        fig, axes = plt.subplots(rows, cols, sharex=True, figsize=(min(1.9*cols, 26), min(1.25*rows + 1, 20)),
                                 squeeze=False)
    else:
        fig, axes = plt.subplots(n, 1, sharex=True, figsize=(10, min(2.2*n, 1.4*n + 2)), squeeze=False)
    flat = axes.ravel()
    for k, ax in enumerate(flat):
        if k >= n:
            ax.axis("off")
            continue
        ax.plot(t_ms, V[:, k], linewidth=0.7 if big else 1)
        for t0, t1 in faults:
            ax.axvspan(t0*1000.0, (t1 if t1 is not None else t_end)*1000.0, color="red", alpha=0.1, lw=0)
        for ts in switches:
            ax.axvline(ts*1000.0, color="gray", linestyle="--", linewidth=0.6 if big else 1)
        ax.grid(True, alpha=0.3)
        if big:
            ax.set_title(names[k], fontsize=7, pad=2)
            ax.tick_params(labelsize=6, length=2, pad=1)
        else:
            ax.set_ylabel("%s (V)" % names[k])
    title = "Node voltages (shaded: fault applied, dashed: switch operation)"
    if big:
        fig.suptitle(title + " - %d nodes, voltages in V, time in ms" % n, fontsize=11)
        fig.tight_layout(rect=(0, 0, 1, 0.98), h_pad=0.4, w_pad=0.3)
    else:
        axes[0, 0].set_title(title)
        axes[-1, 0].set_xlabel("Time (ms)")
        fig.tight_layout()
    return fig


def read_events(path):
    faults, switches = [], []
    try:
        with open(path, newline="") as f:
            for row in csv.DictReader(f):
                t0 = float(row["t_start"])
                if row["type"] == "fault":
                    faults.append((t0, float(row["t_end"]) if row["t_end"] else None))
                else:
                    switches.append(t0)
    except FileNotFoundError:
        pass
    return faults, switches


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default="results.csv")
    ap.add_argument("--events", default="events.csv")
    ap.add_argument("--save", help="save the figure to this file instead of showing it")
    args = ap.parse_args()

    with open(args.results) as f:
        names = f.readline().strip().split(",")[1:]
    data = np.loadtxt(args.results, delimiter=",", skiprows=1, ndmin=2)
    faults, switches = read_events(args.events)
    fig = plot_nodes(data[:, 0]*1000.0, data[:, 1:], names, faults, switches)

    if args.save:
        fig.savefig(args.save, dpi=100)
        print("Saved", args.save)
    else:
        plt.show()


if __name__ == "__main__":
    main()
