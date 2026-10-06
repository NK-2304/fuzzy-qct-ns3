# Rule-Base Audit and Monotonicity Formulation

### Initial Proposal Deficiencies Identified
A programmatic audit of the preliminary 5x5 heuristic table revealed:
1. **Antisymmetry Violations:** 14 out of 25 rule cells violated point-reflection antisymmetry:
   $f(-d_{avg}, -sd_{avg}) \neq -f(d_{avg}, sd_{avg})$
2. **Monotonicity Breakdown:** The PH and PL rows produced localized control inversions (e.g., $PL, PL \rightarrow Keep$ while $PL, ZE \rightarrow LowerSlow$). This manifested as an anomalous positive slope ("hump") where a growing queue received less throttling than a steady queue.

### Reformulated Analytical Surface
The discrete table was redesigned via a severity index parameterized by dominant state $d_{avg}$ and secondary derivative nudge $sd_{avg}$:
$$S(i, j) = 1.6 \cdot i + j \cdot \left(1 - \frac{|i|}{4}\right), \quad i, j \in [-2, 2]$$
$$\text{Output Index} = \text{round}\left(-\text{clip}(S, -4, 4)\right)$$

### Validation
- Monotonicity: 100% monotonic across all rows and columns.
- Antisymmetry: 0 symmetry violations.
- Verified via 41x41 grid probe (1,681 points) and 2D line profile slicing.
