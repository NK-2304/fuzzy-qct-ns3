import subprocess
import re
import statistics

algos = ["RED", "ARED", "QCT"]
sources = [25, 50, 75, 100]
SEEDS = list(range(1, 6))       # bump to range(1, 11) once this runs cleanly
SIM_TIME = 15.0
WARMUP_TIME = 5.0

h_algo = "Algorithm"
h_src = "Sources"
h_thr = "Throughput (Mbps)"
h_loss = "Loss (%)"
h_delay = "Delay (ms)"
h_jit = "Jitter (ms)"

print("\n" + "=" * 100)
print(f"{h_algo:<10} | {h_src:<8} | {h_thr:<22} | {h_loss:<16} | {h_delay:<16} | {h_jit:<16}")
print("=" * 100)

# Replication of Tables 1-3, averaged over multiple RNG seeds


def mean_std(values, decimals):
    if not values:
        return "N/A"
    m = statistics.mean(values)
    s = statistics.stdev(values) if len(values) > 1 else 0.0
    return f"{m:.{decimals}f} +/- {s:.{decimals}f}"


for algo in algos:
    for n in sources:
        thrs, losses, delays, jitters = [], [], [], []

        for seed in SEEDS:
            cmd = (
                f'./ns3 run "scratch/qct-ared-validation '
                f'--queueType={algo} --nSources={n} '
                f'--simTime={SIM_TIME} --warmupTime={WARMUP_TIME} --seed={seed}"'
            )
            res = subprocess.run(cmd, shell=True, capture_output=True, text=True)

            t = re.search(r"Throughput\s*:\s*([\d\.]+)", res.stdout)
            l = re.search(r"Loss Rate\s*:\s*([\d\.]+)", res.stdout)
            d = re.search(r"Avg Delay\s*:\s*([\d\.]+)", res.stdout)
            j = re.search(r"Jitter\s*:\s*([\d\.]+)", res.stdout)

            if t and l and d and j:
                thrs.append(float(t.group(1)))
                losses.append(float(l.group(1)))
                delays.append(float(d.group(1)))
                jitters.append(float(j.group(1)))
            else:
                print(f"  [warn] {algo} N={n} seed={seed}: failed to parse output, skipping this run")

        # Print exactly ONE row per (algorithm, N) — after all seeds are collected, not per-seed
        m_thr = mean_std(thrs, 3)
        m_loss = mean_std(losses, 2)
        m_delay = mean_std(delays, 2)
        m_jit = mean_std(jitters, 3)
        print(f"{algo:<10} | {n:<8} | {m_thr:<22} | {m_loss:<16} | {m_delay:<16} | {m_jit:<16}")

print("=" * 100 + "\n")