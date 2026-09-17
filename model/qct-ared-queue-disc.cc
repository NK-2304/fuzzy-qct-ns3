#include "qct-ared-queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/queue-size.h"
#include "ns3/drop-tail-queue.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("QctAredQueueDisc");
NS_OBJECT_ENSURE_REGISTERED(QctAredQueueDisc);

TypeId
QctAredQueueDisc::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::QctAredQueueDisc")
    .SetParent<QueueDisc>()
    .SetGroupName("TrafficControl")
    .AddConstructor<QctAredQueueDisc>()
    .AddAttribute("MinTh", "Minimum queue threshold.",
                   DoubleValue(24.0),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_minTh),
                   MakeDoubleChecker<double>())
    .AddAttribute("MaxTh", "Maximum queue threshold.",
                   DoubleValue(72.0),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_maxTh),
                   MakeDoubleChecker<double>())
    .AddAttribute("Wq0", "Base queue weight parameter.",
                   DoubleValue(0.002),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_wq0),
                   MakeDoubleChecker<double>())
    .AddAttribute("MaxP", "Maximum marking/dropping probability.",
                   DoubleValue(0.1),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_maxP),
                   MakeDoubleChecker<double>())
    .AddAttribute("Alpha",
                  "Step size for decreasing midTh when queue is growing",
                  DoubleValue(1.0),
                  MakeDoubleAccessor(&QctAredQueueDisc::m_alpha),
                  MakeDoubleChecker<double>(0.0))
    .AddAttribute("Beta",
                  "Step size for increasing midTh when queue is shrinking",
                  DoubleValue(1.0),
                  MakeDoubleAccessor(&QctAredQueueDisc::m_beta),
                  MakeDoubleChecker<double>(0.0))
    .AddTraceSource("AvgQueueSize", "Average queue size calculated by QCT-ARED",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_qAvg),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("MidThreshold", "Dynamically updated mid_th threshold",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curMidTh),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("DropProbability", "Calculated packet drop probability",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curDropProb),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("Davg",
                    "First-order change rate of average queue length",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curDavg),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("Sdavg",
                    "Second-order change rate of average queue length",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curSdavg),
                    "ns3::TracedValueCallback::Double");
  return tid;
}

QctAredQueueDisc::QctAredQueueDisc()
  : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE),
    m_minTh(24.0),
    m_maxTh(72.0),
    m_midTh(48.0),
    m_wq0(0.002),
    m_maxP(0.1),
    m_alpha(1.0),
    m_beta(1.0),
    m_qAvg(0.0),
    m_instPrev1(0.0),
    m_dAvg(0.0),
    m_instPrev2(0.0),
    m_sdAvg(0.0),
    m_curMidTh(48.0),
    m_curDropProb(0.0),
    m_curDavg(0.0),
    m_curSdavg(0.0),
    m_count(0)
{
  m_uv = CreateObject<UniformRandomVariable>();
}

QctAredQueueDisc::~QctAredQueueDisc()
{
}

double
QctAredQueueDisc::GetAvgQueueSize() const
{
  return m_qAvg.Get();
}

double
QctAredQueueDisc::GetMidThreshold() const
{
  return m_curMidTh.Get();
}

double
QctAredQueueDisc::GetDropProbability() const
{
  return m_curDropProb.Get();
}

double
QctAredQueueDisc::GetDavg() const
{
  return m_curDavg.Get();
}

double
QctAredQueueDisc::GetSdavg() const
{
  return m_curSdavg.Get();
}

// 2.1. Novel average queue length evaluation model (queue weight) [2]
double
QctAredQueueDisc::CalculateWq(double avg) const
{
  if (avg < m_minTh || avg >= m_maxTh)
    {
      return 4.0 * m_wq0; // 0.008: Rapid responsiveness outside the safe band
    }
  else
    {
      return 2.0 * m_wq0; // 0.004: Stable smoothing within the operating band
    }
}

// 3.1. Threshold update model [5]
void
QctAredQueueDisc::UpdateMidTh(double dAvg, double sdAvg)
{
  double delta = 0.0;

  if (dAvg > 0.0 && sdAvg > 0.0)
    {
      delta = -m_alpha;           // Accelerated queue growth: drop threshold faster
    }
  else if (dAvg > 0.0 && sdAvg <= 0.0)
    {
      delta = -m_alpha / 2.0;     // Decelerated queue growth: moderate threshold drop
    }
  else if (dAvg == 0.0)
    {
      delta = 0.0;                // Steady state: mid_th unchanged (Eq. 5 exact condition)
    }
  else if (dAvg < 0.0 && sdAvg > 0.0)
    {
      delta = m_beta / 2.0;       // Queue draining, but deceleration slowing down
    }
  else // dAvg < 0.0 && sdAvg <= 0.0
    {
      delta = m_beta;             // Rapid queue drain: expand capacity aggressively
    }

  m_midTh += delta;

  // Boundary clamp: keep midTh strictly within (minTh, maxTh)
  m_midTh = std::max(m_minTh + 1.0, std::min(m_midTh, m_maxTh - 1.0));
  m_curMidTh = m_midTh;
}

double
QctAredQueueDisc::CalculateDropProb(double avg, double dAvg, double sdAvg)
{
  // Universal safety check: zero drop below minimum threshold
  if (avg < m_minTh)
    {
      m_count = 0;
      return 0.0;
    }

  // Quadrant 1: dAvg > 0 && sdAvg > 0 -> p1: Linear over [minTh, midTh] (Eq. 7, 11)
  if (dAvg > 0 && sdAvg > 0)
    {
      if (avg >= m_midTh) return 1.0;
      return m_maxP * (avg - m_minTh) / (m_midTh - m_minTh);
    }
  // Quadrant 2: dAvg > 0 && sdAvg <= 0 -> p2: Cubic over [minTh, midTh] (Eq. 8, 12)
  else if (dAvg > 0 && sdAvg <= 0)
    {
      if (avg >= m_midTh) return 1.0;
      double ratio = (avg - m_minTh) / (m_midTh - m_minTh);
      return m_maxP * std::pow(ratio, 3.0);
    }
  // Quadrant 3: dAvg <= 0 && sdAvg > 0 -> p3: Linear over [minTh, maxTh] (Eq. 9, 13)
  else if (dAvg <= 0 && sdAvg > 0)
    {
      if (avg >= m_maxTh) return 1.0;
      return m_maxP * (avg - m_minTh) / (m_maxTh - m_minTh);
    }
  // Quadrant 4: dAvg <= 0 && sdAvg <= 0 -> p4: Cubic over [minTh, maxTh] (Eq. 10, 14)
  else
    {
      if (avg >= m_maxTh) return 1.0;
      double ratio = (avg - m_minTh) / (m_maxTh - m_minTh);
      return m_maxP * std::pow(ratio, 3.0);
    }
}

bool
QctAredQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
  uint32_t currentQ = GetInternalQueue(0)->GetCurrentSize().GetValue();
  double inst = static_cast<double>(currentQ);

  // 1. Equation (2): Compute adaptive weight based on current smoothed average avg(t)
  double prevAvg = m_qAvg.Get();
  double wq = CalculateWq(prevAvg);

  // 2. Equation (1): Update average queue length
  double newQAvg = (1.0 - wq) * prevAvg + wq * inst;
  m_qAvg = newQAvg;

  // 3. Equation (3): Velocity / 1st-order rate of change
  m_dAvg = (1.0 - wq) * m_dAvg + wq * (inst - m_instPrev1);

  // 4. Equation (4): Acceleration / 2nd-order rate of change
  m_sdAvg = (1.0 - wq) * m_sdAvg + wq * (inst - 2.0 * m_instPrev1 + m_instPrev2);

  // Wire up traced mirrors for external monitoring and plotting
  m_curDavg = m_dAvg;
  m_curSdavg = m_sdAvg;

  // 5. Shift instantaneous samples for the next packet arrival
  m_instPrev2 = m_instPrev1;
  m_instPrev1 = inst;

  // 6. Update dynamic intermediate threshold
  UpdateMidTh(m_dAvg, m_sdAvg);

  // 7. Calculate drop probability using the 4-quadrant curves
  double pb = CalculateDropProb(newQAvg, m_dAvg, m_sdAvg);
  m_curDropProb = pb;

  // 8. Dropping evaluation
  if (pb >= 1.0)
    {
      m_count = 0;
      DropBeforeEnqueue(item, FORCED_DROP);
      return false;
    }
  else if (pb > 0.0)
    {
      m_count++;
      double denom = 1.0 - static_cast<double>(m_count) * pb;
      double pa = (denom > 0.0) ? std::min(pb / denom, 1.0) : 1.0;

      if (m_uv->GetValue() < pa)
        {
          m_count = 0;
          DropBeforeEnqueue(item, UNFORCED_DROP);
          return false;
        }
    }
  else
    {
      m_count = 0;
    }

  return GetInternalQueue(0)->Enqueue(item);
}

Ptr<QueueDiscItem>
QctAredQueueDisc::DoDequeue(void)
{
  if (GetInternalQueue(0)->IsEmpty())
    {
      return nullptr;
    }
  return GetInternalQueue(0)->Dequeue();
}

Ptr<const QueueDiscItem>
QctAredQueueDisc::DoPeek(void)
{
  if (GetInternalQueue(0)->IsEmpty())
    {
      return nullptr;
    }
  return GetInternalQueue(0)->Peek();
}

bool
QctAredQueueDisc::CheckConfig(void)
{
  if (GetNInternalQueues() == 0)
    {
      AddInternalQueue(CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize", QueueSizeValue(QueueSize("120p"))));
    }
  if (GetNInternalQueues() != 1)
    {
      NS_LOG_ERROR("QctAredQueueDisc requires exactly 1 internal queue");
      return false;
    }
  if (m_minTh >= m_maxTh)
    {
      NS_LOG_ERROR("MinTh must be strictly less than MaxTh");
      return false;
    }
  return true;
}

void
QctAredQueueDisc::InitializeParams(void)
{
  m_midTh = (m_minTh + m_maxTh) / 2.0;
  m_curMidTh = m_midTh;
  m_qAvg = 0.0;
  m_dAvg = 0.0;
  m_sdAvg = 0.0;
  m_curDavg = 0.0;     
  m_curSdavg = 0.0;    
  m_instPrev1 = 0.0;
  m_instPrev2 = 0.0;
  m_count = 0;
}

} // namespace ns3
