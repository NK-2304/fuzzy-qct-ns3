import subprocess
import re

algos = ["RED", "ARED", "QCT"]
sources = [25, 50, 75, 100]

h_algo = "Algorithm"
h_src = "Sources"
h_thr = "Throughput (Mbps)"
h_loss = "Loss (%)"
h_delay = "Delay (ms)"

print("\n" + "=" * 76)
print(f"{h_algo:<10} | {h_src:<8} | {h_thr:<18} | {h_loss:<10} | {h_delay:<10}")
print("=" * 76)

for algo in algos:
    for n in sources:
        cmd = f'./ns3 run "scratch/qct-ared-validation --queueType={algo} --nSources={n} --simTime=15"'
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        t = re.search(r"Throughput\s*:\s*([\d\.]+)", res.stdout)
        l = re.search(r"Loss Rate\s*:\s*([\d\.]+)", res.stdout)
        d = re.search(r"Avg Delay\s*:\s*([\d\.]+)", res.stdout)
        
        thr = t.group(1) if t else "N/A"
        loss = l.group(1) if l else "N/A"
        delay = d.group(1) if d else "N/A"
        print(f"{algo:<10} | {n:<8} | {thr:<18} | {loss:<10} | {delay:<10}")

print("=" * 76 + "\n")
