import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

df = pd.read_csv("fuzzy_grid.csv")
n = int(np.sqrt(len(df)))

fig, axes = plt.subplots(1, 2, figsize=(13, 5))

# Sample 5 slices across the sdavg universe
slices = [-0.8, -0.4, 0.0, 0.4, 0.8]
colors = ["#2b5c8f", "#4682b4", "#2e8b57", "#d95f02", "#b22222"]

for sdavg_frac, col in zip(slices, colors):
    target_sdavg = sdavg_frac * df["sdavg"].abs().max()
    # Extract the nearest row slice corresponding to target_sdavg
    nearest = df.iloc[(df["sdavg"] - target_sdavg).abs().argsort()[:n]].sort_values("davg")
    
    label_str = f"sdavg = {target_sdavg:+.4f}"
    axes[0].plot(nearest["davg"], nearest["delta_midth"], label=label_str, color=col, linewidth=1.8)
    axes[1].plot(nearest["davg"], nearest["e"], label=label_str, color=col, linewidth=1.8)

# Format Left Axis: delta_mid_th
axes[0].set_xlabel("davg", fontsize=11, fontweight="bold")
axes[0].set_ylabel("delta_mid_th", fontsize=11, fontweight="bold")
axes[0].set_title("Cross-Section: delta_mid_th vs davg", fontsize=11, fontweight="bold")
axes[0].grid(True, linestyle="--", alpha=0.6)
axes[0].legend(fontsize=8, loc="upper right")

# Format Right Axis: e
axes[1].set_xlabel("davg", fontsize=11, fontweight="bold")
axes[1].set_ylabel("e (drop exponent)", fontsize=11, fontweight="bold")
axes[1].set_title("Cross-Section: e vs davg", fontsize=11, fontweight="bold")
axes[1].grid(True, linestyle="--", alpha=0.6)
axes[1].legend(fontsize=8, loc="upper right")

plt.tight_layout()
plt.savefig("fuzzy_line_profiles.png", dpi=200)
print("[OK] Saved fuzzy_line_profiles.png")
