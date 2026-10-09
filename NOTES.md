# Phase 6 Validated Baseline Specification

- **Bottleneck NetDevice Queue**: `ns3::DropTailQueue` with `MaxSize = 1p` (driver bufferbloat eliminated).
- **Bottleneck Link**: 10 Mbps DataRate, 40 ms propagation delay.
- **Access Links**: 50 Mbps DataRate, 10 ms propagation delay.
- **AQM Buffer Capacity**: `MaxSize = 120p` enforced identically across RED, ARED, QCT-ARED, and Fuzzy-QCT.
- **RED Calibration**: `LinkBandwidth = 10Mbps`, `MeanPktSize = 1000` (idle decay calibrated).
- **Fuzzy-QCT Configuration**:
  - `DeltaGain = 0.25` (anti-windup scaling)
  - `GentleTail = true` (linear drop escalation above mid_th)
- **Evaluation Windows**: `simTime = 15 s`, `warmupTime = 5 s`.
- **RNG Seeds**:
  - Tuning & Exploration: Seeds 1–5
  - Evaluation & Final Reporting: Seeds 6–15
