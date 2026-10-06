#ifndef FUZZY_ENGINE_H
#define FUZZY_ENGINE_H

#include <fl/Headers.h>

namespace fuzzyqct {

struct FuzzyOutput
{
  double deltaMidTh;   ///< continuous mid_th offset (replaces QCT-ARED's Eq. 5 discrete steps)
  double e;            ///< continuous drop exponent in [1.0, 3.0] (replaces Eq. 6-14 quadrant selection)
};

/**
 * Standalone Mamdani fuzzy controller implementing Table 1 (the 5x5
 * davg/sdavg -> delta_mid_th rule base) plus a paired second output (the
 * drop exponent e), inferred from the SAME 25 rules so both outputs
 * respond consistently to the same underlying congestion-severity signal.
 *
 * Deliberately has NO ns-3 dependency — build/test it fully standalone
 * (see fuzzy_grid_probe.cc) before wiring it into QctAredQueueDisc.
 */
class FuzzyEngine
{
public:
  /**
   * \param davgRange  universe half-width for davg, i.e. input domain [-davgRange, +davgRange]
   * \param sdavgRange universe half-width for sdavg
   *
   * Determine these empirically from real QctAredQueueDisc Davg/Sdavg trace
   * data — do not guess.
   */
  FuzzyEngine(double davgRange, double sdavgRange);
  ~FuzzyEngine();

  // non-copyable: owns raw fuzzylite pointers
  FuzzyEngine(const FuzzyEngine&) = delete;
  FuzzyEngine& operator=(const FuzzyEngine&) = delete;

  FuzzyOutput Evaluate(double davg, double sdavg);

private:
  void BuildEngine(double davgRange, double sdavgRange);

  fl::Engine* m_engine;
  fl::InputVariable* m_davg;
  fl::InputVariable* m_sdavg;
  fl::OutputVariable* m_deltaMidTh;
  fl::OutputVariable* m_e;
};

} // namespace fuzzyqct

#endif // FUZZY_ENGINE_H
