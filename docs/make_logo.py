"""Generate the EMTSim logo (docs/logo.svg, docs/logo-dark.svg, docs/logo.png).

Run from the repository root:  python docs/make_logo.py
"""
import os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch

plt.rcParams["svg.fonttype"] = "path"   # text as outlines: identical everywhere
plt.rcParams["font.family"] = ["Segoe UI", "DejaVu Sans"]

HERE = os.path.dirname(os.path.abspath(__file__))
NAVY = "#1F2537"
TEAL = "#21A99B"
AMBER = "#F2A93B"


def draw(text_color, sub_color, icon_edge):
    fig = plt.figure(figsize=(6.4, 1.6))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, 640); ax.set_ylim(0, 160)
    ax.set_aspect("equal"); ax.axis("off")

    # Icon tile
    ax.add_patch(FancyBboxPatch((14, 14), 132, 132, boxstyle="round,pad=0,rounding_size=30",
                                fc=NAVY, ec=icon_edge, lw=1.5))

    # Waveform: steady sine -> fault (voltage collapses) -> clearing overshoot that rings down
    x0, x1, yc, amp = 32, 128, 80, 22.0
    n = 481
    t = np.linspace(0, 1, n)
    w = 2*np.pi*2.5
    tf, tc = 0.4, 0.6               # fault applied / cleared (zero crossings of the sine)
    y = np.sin(w*t)
    y[(t >= tf) & (t < tc)] *= 0.06
    post = t >= tc
    dt = t[post] - tc
    y[post] = np.sin(w*t[post]) - 1.7*np.exp(-dt/0.09)*np.sin(2*np.pi*6.0*dt)
    ys = yc + amp*y
    xs = x0 + (x1 - x0)*t

    # Fault window
    ax.add_patch(FancyBboxPatch((x0 + (x1-x0)*tf, yc - 46), (x1-x0)*(tc-tf), 92,
                                boxstyle="round,pad=0,rounding_size=4", fc=AMBER, ec="none", alpha=0.26))
    ax.plot([x0 - 6, x1 + 6], [yc, yc], color="#FFFFFF", alpha=0.16, lw=1.4, solid_capstyle="round")
    ax.plot(xs, ys, color=TEAL, lw=3.4, solid_capstyle="round", solid_joinstyle="round")
    # Sample points: the discrete time steps of the EMT solution
    idx = np.arange(0, n, 24)
    ax.scatter(xs[idx], ys[idx], s=13, color="#FFFFFF", zorder=3, linewidths=0)

    # Wordmark
    ax.text(170, 84, "EMT", fontsize=50, fontweight="bold", color=text_color, va="baseline")
    ax.text(312, 84, "Sim", fontsize=50, fontweight="bold", color=TEAL, va="baseline")
    ax.text(173, 46, "Electromagnetic transients, step by step", fontsize=14.5,
            color=sub_color, va="baseline")
    return fig


def save(fig, name, **kw):
    fig.savefig(os.path.join(HERE, name), transparent=True, **kw)
    plt.close(fig)


if __name__ == "__main__":
    save(draw(NAVY, "#5B6477", NAVY), "logo.svg")
    save(draw("#E6EDF3", "#9AA4B2", "#3A4560"), "logo-dark.svg")
    save(draw(NAVY, "#5B6477", NAVY), "logo.png", dpi=200)
