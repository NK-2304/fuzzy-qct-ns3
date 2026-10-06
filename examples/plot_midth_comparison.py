import pandas as pd
import matplotlib.pyplot as plt

# Load traces
qct = pd.read_csv("QCT_midth_trace.csv", header=None, names=["t", "midth"])
fuzzy = pd.read_csv("FUZZY_midth_trace.csv", header=None, names=["t", "midth"])

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 7), sharey=True)

# Full time series
ax1.plot(qct["t"], qct["midth"], label="QCT-ARED (Discrete Steps: Eq. 5)", color="#d95f02", lw=1.2, alpha=0.85)
ax1.plot(fuzzy["t"], fuzzy["midth"], label="Fuzzy-QCT (Continuous Mamdani)", color="#1b9e77", lw=1.2, alpha=0.85)
ax1.set_title("Full Trajectory: MidThreshold Dynamic Adaptation ($N=100$ sources)", fontsize=12, fontweight="bold")
ax1.set_xlabel("Time (s)")
ax1.set_ylabel("Target Threshold $mid_{th}$ (pkts)")
ax1.grid(True, linestyle="--", alpha=0.6)
ax1.legend(loc="upper right")

# Zoomed window (8.0s - 10.0s) showing discrete jumps vs smooth surface
qct_zoom = qct[(qct["t"] >= 8.0) & (qct["t"] <= 10.0)]
fuzzy_zoom = fuzzy[(fuzzy["t"] >= 8.0) & (fuzzy["t"] <= 10.0)]

ax2.plot(qct_zoom["t"], qct_zoom["midth"], label="QCT-ARED (Discrete Steps)", color="#d95f02", lw=1.5)
ax2.plot(fuzzy_zoom["t"], fuzzy_zoom["midth"], label="Fuzzy-QCT (Continuous)", color="#1b9e77", lw=1.5)
ax2.set_title("Microscopic Zoom ($t \\in [8.0, 10.0]$ s): Stepping vs. Continuous Adaptation", fontsize=11, fontweight="bold")
ax2.set_xlabel("Time (s)")
ax2.set_ylabel("Target Threshold $mid_{th}$ (pkts)")
ax2.grid(True, linestyle="--", alpha=0.6)
ax2.legend(loc="upper right")

plt.tight_layout()
plt.savefig("midth_comparison.png", dpi=300)
print("[SUCCESS] Saved midth_comparison.png")
