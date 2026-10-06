#include "fuzzy-qct-queue-disc.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("FuzzyQctQueueDisc");
NS_OBJECT_ENSURE_REGISTERED(FuzzyQctQueueDisc);

TypeId
FuzzyQctQueueDisc::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::FuzzyQctQueueDisc")
    .SetParent<QctAredQueueDisc>()
    .SetGroupName("TrafficControl")
    .AddConstructor<FuzzyQctQueueDisc>()
    .AddAttribute("DavgRange",
                  "Fuzzy input universe half-width for davg (empirical 99th-percentile)",
                  DoubleValue(0.12),
                  MakeDoubleAccessor(&FuzzyQctQueueDisc::m_davgRange),
                  MakeDoubleChecker<double>())
    .AddAttribute("SdavgRange",
                  "Fuzzy input universe half-width for sdavg (empirical 99th-percentile)",
                  DoubleValue(0.02),
                  MakeDoubleAccessor(&FuzzyQctQueueDisc::m_sdavgRange),
                  MakeDoubleChecker<double>())
    .AddTraceSource("DropExponent",
                    "Fuzzy-inferred continuous drop exponent (e in [1,3])",
                    MakeTraceSourceAccessor(&FuzzyQctQueueDisc::m_curE),
                    "ns3::TracedValueCallback::Double");
  return tid;
}

FuzzyQctQueueDisc::FuzzyQctQueueDisc()
  : QctAredQueueDisc(),
    m_davgRange(0.12),
    m_sdavgRange(0.02),
    m_fuzzyEngine(nullptr),
    m_curE(2.0)
{
}

FuzzyQctQueueDisc::~FuzzyQctQueueDisc()
{
}

double
FuzzyQctQueueDisc::GetDropExponent() const
{
  return m_curE.Get();
}

void
FuzzyQctQueueDisc::UpdateMidTh(double dAvg, double sdAvg)
{
  fuzzyqct::FuzzyOutput out = m_fuzzyEngine->Evaluate(dAvg, sdAvg);
  m_curE = out.e;

  m_midTh = std::clamp(m_midTh + out.deltaMidTh, m_minTh + 1.0, m_maxTh - 1.0);
  m_curMidTh = m_midTh;
}

double
FuzzyQctQueueDisc::CalculateDropProb(double avg, double dAvg, double sdAvg)
{
  (void) dAvg;
  (void) sdAvg;

  if (avg < m_minTh)
    {
      return 0.0;
    }
  if (avg >= m_maxTh)
    {
      return 1.0;
    }

  double ratio = (avg - m_minTh) / (m_midTh - m_minTh);
  ratio = std::clamp(ratio, 0.0, 1.0);

  double pb = m_maxP * std::pow(ratio, m_curE.Get());
  return std::clamp(pb, 0.0, 1.0);
}

void
FuzzyQctQueueDisc::InitializeParams(void)
{
  QctAredQueueDisc::InitializeParams();
  m_fuzzyEngine = std::make_unique<fuzzyqct::FuzzyEngine>(m_davgRange, m_sdavgRange);
  m_curE = 2.0;
}

} // namespace ns3
