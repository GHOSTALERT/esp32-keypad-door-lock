"""Draw docs/images/state_machine.png (needs matplotlib).  python3 docs/make_state_diagram.py"""
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

OUT = Path(__file__).resolve().parent / "images" / "state_machine.png"
INK, GREEN, RED, AMBER, BLUE = "#1f2937", "#15803d", "#b91c1c", "#b45309", "#1d4ed8"

fig, ax = plt.subplots(figsize=(9.4, 5.2))
ax.set_xlim(0, 110)
ax.set_ylim(0, 62)
ax.axis("off")


def state(x, y, title, lines, color):
    ax.add_patch(FancyBboxPatch((x - 14, y - 7.5), 28, 15, boxstyle="round,pad=0.3,rounding_size=2.5",
                                fc="white", ec=color, lw=2.4))
    ax.text(x, y + 3.2, title, ha="center", va="center", fontsize=12, weight="bold", color=color)
    ax.text(x, y - 2.6, lines, ha="center", va="center", fontsize=8.2, color=INK, linespacing=1.4)


def arrow(a, b, label, color=INK, rad=0.0, dy=1.8):
    ax.add_patch(FancyArrowPatch(a, b, arrowstyle="-|>", mutation_scale=15, lw=1.7, color=color,
                                 connectionstyle=f"arc3,rad={rad}"))
    ax.text((a[0] + b[0]) / 2, (a[1] + b[1]) / 2 + dy, label, ha="center", fontsize=8.5, color=color,
            weight="bold")


state(55, 50, "ENTRY", "digits shown as *\n* clear · D delete · # submit", BLUE)
state(16, 18, "UNLOCKED", "relay on · green LED\nsuccess tone · 5 s", GREEN)
state(55, 18, "WRONG", "red LED · warning tone\n\"Attempts left: n\" · 1.5 s", AMBER)
state(94, 18, "LOCKOUT", "keypad ignored · alarm\n30 s countdown", RED)

arrow((43, 43), (20, 26), "# correct", GREEN, rad=0.15, dy=2.5)
arrow((24, 26), (45, 42.5), "after 5 s", GREEN, rad=0.15, dy=-4)
arrow((52, 42.5), (52, 25.5), "# wrong", AMBER, dy=0)
arrow((58, 25.5), (58, 42.5), "after 1.5 s", AMBER, dy=0)
ax.texts[-2].set_position((45.5, 34))
ax.texts[-1].set_position((65, 34))
arrow((69.5, 18), (79.5, 18), "", RED)
ax.text(74.5, 22.5, "4th wrong\nin a row", ha="center", fontsize=8.5, color=RED, weight="bold")
arrow((90, 25.5), (69, 45), "after 30 s", RED, rad=0.15, dy=2.5)
ax.text(55, 3, "Plain C++ state machine (lock_logic.h): the same code runs on the ESP32 and in 52 host unit tests",
        ha="center", fontsize=8.5, color="#475569", style="italic")
OUT.parent.mkdir(exist_ok=True)
fig.savefig(OUT, dpi=170, bbox_inches="tight", facecolor="white")
print("wrote", OUT)
