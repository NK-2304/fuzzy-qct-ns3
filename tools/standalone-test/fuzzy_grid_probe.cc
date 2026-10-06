#include "fuzzy-engine.h"
#include <fstream>
#include <iostream>

int main()
{
  // Calibrated empirical 99th-percentile half-widths from Step 2
  double davgRange = 0.12;
  double sdavgRange = 0.02;

  fuzzyqct::FuzzyEngine engine(davgRange, sdavgRange);

  // --- Sanity spot-checks against Table 1's extreme cells before the full grid ---
  auto check = [&](double d, double s, const std::string& label)
  {
    fuzzyqct::FuzzyOutput r = engine.Evaluate(d, s);
    std::cout << label << "  davg=" << d << " sdavg=" << s
              << "  -> deltaMidTh=" << r.deltaMidTh << "  e=" << r.e << "\n";
  };

  std::cout << "=== Spot checks (compare sign/magnitude against Table 1) ===\n";
  check(davgRange, sdavgRange, "PH,PH (expect deltaMidTh near -1.0, e near 1.0)");
  check(-davgRange, -sdavgRange, "NH,NH (expect deltaMidTh near +1.0, e near 3.0)");
  check(0.0, 0.0, "ZE,ZE (expect deltaMidTh near 0.0, e near 2.0)");
  std::cout << "\n";

  // --- Full grid dump for the 3D surface plot ---
  std::ofstream out("fuzzy_grid.csv");
  out << "davg,sdavg,delta_midth,e\n";

  const int STEPS = 41; // 41x41 = 1681 points
  for (int i = 0; i < STEPS; ++i)
    {
      double davg = -davgRange + (2.0 * davgRange) * i / (STEPS - 1);
      for (int j = 0; j < STEPS; ++j)
        {
          double sdavg = -sdavgRange + (2.0 * sdavgRange) * j / (STEPS - 1);
          fuzzyqct::FuzzyOutput result = engine.Evaluate(davg, sdavg);
          out << davg << "," << sdavg << "," << result.deltaMidTh << "," << result.e << "\n";
        }
    }

  std::cout << "Wrote fuzzy_grid.csv (" << STEPS * STEPS << " points)\n";
  return 0;
}
