# Benchmarking Harness Specification (Phase 4 / Phase 6 Fix)

- **Bottleneck NetDevice Queue**: `ns3::DropTailQueue` with `MaxSize = 1p` (eliminates driver bufferbloat).
- **Bottleneck Link**: 10 Mbps DataRate, 40 ms propagation delay.
- **Access Links**: 50 Mbps DataRate, 10 ms propagation delay.
- **AQM Buffer Capacity**: `MaxSize = 120p` enforced identically across RED, ARED, QCT-ARED, and Fuzzy-QCT.
- **TCP Stack**: `TcpNewReno`, segment size 1000 bytes, staggered starts via uniform jitter.
- **Simulation Parameters**:
  - `simTime`: 15 s
  - Tuning Seeds: 1–5
  - Evaluation / Reporting Seeds: 6–15
- **Baseline Jitter Reduction (N=25 to 100)**:
  - QCT vs RED: 10.0% (N=25), 30.8% (N=50), 44.9% (N=75), 62.2% (N=100).
