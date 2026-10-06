#ifndef FUZZY_QCT_QUEUE_DISC_H
#define FUZZY_QCT_QUEUE_DISC_H

#include "qct-ared-queue-disc.h"
#include "fuzzy-engine.h"
#include <memory>

namespace ns3 {

/**
 * \brief Fuzzy-QCT queue disc.
 *
 * Deliberately a MINIMAL, controlled modification of QctAredQueueDisc: it
 * inherits DoEnqueue/DoDequeue/DoPeek/CheckConfig and the entire Eq. 1-4
 * (avg/davg/sdavg) computation unchanged, and overrides only:
 *   - UpdateMidTh()       — was QCT-ARED's discrete Eq. 5 step table,
 *                           now a continuous fuzzy-inferred offset.
 *   - CalculateDropProb() — was QCT-ARED's 4-branch linear/cubic Eqs. 6-14,
 *                           now a single continuous-exponent curve.
 */
class FuzzyQctQueueDisc : public QctAredQueueDisc
{
public:
  static TypeId GetTypeId(void);

  FuzzyQctQueueDisc();
  virtual ~FuzzyQctQueueDisc() override;

  double GetDropExponent() const;

protected:
  virtual void UpdateMidTh(double dAvg, double sdAvg) override;
  virtual double CalculateDropProb(double avg, double dAvg, double sdAvg) override;
  virtual void InitializeParams(void) override;

private:
  double m_davgRange;    ///< fuzzy input universe half-width for davg (Attribute)
  double m_sdavgRange;   ///< fuzzy input universe half-width for sdavg (Attribute)

  std::unique_ptr<fuzzyqct::FuzzyEngine> m_fuzzyEngine;

  /// Cached between UpdateMidTh() and CalculateDropProb() in the same DoEnqueue()
  TracedValue<double> m_curE;
};

} // namespace ns3

#endif // FUZZY_QCT_QUEUE_DISC_H
