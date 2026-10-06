import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv("fuzzy_grid.csv")

n = int(np.sqrt(len(df)))
DAVG = df["davg"].values.reshape(n, n)
SDAVG = df["sdavg"].values.reshape(n, n)
DELTA = df["delta_midth"].values.reshape(n, n)
E = df["e"].values.reshape(n, n)

# Expanded canvas with ample vertical height for 3D projections
fig = plt.figure(figsize=(16, 7.5))

# --- Plot 1: delta_mid_th ---
ax1 = fig.add_subplot(1, 2, 1, projection="3d")
surf1 = ax1.plot_surface(DAVG, SDAVG, DELTA, cmap="coolwarm", edgecolor="none", alpha=0.95)
ax1.set_xlabel("davg", labelpad=8)
ax1.set_ylabel("sdavg", labelpad=8)
ax1.set_zlabel("delta mid_th", labelpad=8)
ax1.set_title("Fuzzy-QCT: Continuous Threshold Offset\n(cf. QCT-ARED's 5-step Eq. 5)", 
              fontsize=12, fontweight="bold", pad=12)
fig.colorbar(surf1, ax=ax1, shrink=0.55, aspect=12, pad=0.08)

# --- Plot 2: Drop Exponent e ---
ax2 = fig.add_subplot(1, 2, 2, projection="3d")
surf2 = ax2.plot_surface(DAVG, SDAVG, E, cmap="viridis", edgecolor="none", alpha=0.95)
ax2.set_xlabel("davg", labelpad=8)
ax2.set_ylabel("sdavg", labelpad=8)
ax2.set_zlabel("drop exponent e", labelpad=8)
ax2.set_title("Fuzzy-QCT: Continuous Drop Exponent\n(cf. QCT-ARED's 4-branch Eq. 6-14)", 
              fontsize=12, fontweight="bold", pad=12)
fig.colorbar(surf2, ax=ax2, shrink=0.55, aspect=12, pad=0.08)

# Fixed margin boundaries (avoids buggy 3D tight_layout / bbox cropping)
fig.subplots_adjust(left=0.04, right=0.94, bottom=0.12, top=0.86, wspace=0.18)

# Save without bbox_inches='tight'
plt.savefig("fuzzy_surface.png", dpi=200)
print("[OK] Saved clean fuzzy_surface.png without cropping.")
