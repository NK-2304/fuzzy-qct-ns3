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
    .AddAttribute("Alpha", "mid_th adjustment increment step.",
                   DoubleValue(0.5),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_alpha),
                   MakeDoubleChecker<double>())
    .AddAttribute("Beta", "mid_th adjustment decrement step.",
                   DoubleValue(0.5),
                   MakeDoubleAccessor(&QctAredQueueDisc::m_beta),
                   MakeDoubleChecker<double>())
    .AddTraceSource("AvgQueueSize", "Average queue size calculated by QCT-ARED",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_qAvg),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("MidThreshold", "Dynamically updated mid_th threshold",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curMidTh),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("DropProbability", "Calculated packet drop probability",
                    MakeTraceSourceAccessor(&QctAredQueueDisc::m_curDropProb),
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
    m_alpha(0.5),
    m_beta(0.5),
    m_qAvg(0.0),
    m_prevQAvg(0.0),
    m_dAvg(0.0),
    m_prevDAvg(0.0),
    m_sdAvg(0.0),
    m_curMidTh(48.0),
    m_curDropProb(0.0),
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
QctAredQueueDisc::CalculateWq(uint32_t currentQ) const
{
  if (currentQ < m_minTh)
    {
      return m_wq0 / 2.0;
    }
  else if (currentQ <= m_maxTh)
    {
      return m_wq0;
    }
  else
    {
      return 2.0 * m_wq0;
    }
}

void
QctAredQueueDisc::UpdateMidTh(double dAvg, double sdAvg)
{
  double delta = 0.0;
  if (dAvg > 0 && sdAvg > 0)
    {
      delta = -m_alpha;
    }
  else if (dAvg > 0 && sdAvg <= 0)
    {
      delta = -m_alpha / 2.0;
    }
  else if (dAvg <= 0 && sdAvg < 0)
    {
      delta = m_beta;
    }
  else if (dAvg <= 0 && sdAvg >= 0)
    {
      delta = m_beta / 2.0;
    }

  m_midTh = std::clamp(m_midTh + delta, m_minTh + 1.0, m_maxTh - 1.0);
  m_curMidTh = m_midTh;
}

double
QctAredQueueDisc::CalculateDropProb(double avg)
{
  if (avg < m_minTh)
    {
      m_count = 0;
      return 0.0;
    }
  else if (avg < m_midTh)
    {
      double ratio = (avg - m_minTh) / (m_midTh - m_minTh);
      return m_maxP * 0.5 * std::pow(ratio, 3.0);
    }
  else if (avg < m_maxTh)
    {
      double ratio = (avg - m_midTh) / (m_maxTh - m_midTh);
      return (m_maxP * 0.5) + (m_maxP * 0.5 * ratio);
    }
  else
    {
      return 1.0;
    }
}

bool
QctAredQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
  uint32_t currentQ = GetInternalQueue(0)->GetCurrentSize().GetValue();

  double wq = CalculateWq(currentQ);
  m_prevQAvg = m_qAvg.Get();
  double newQAvg = (1.0 - wq) * m_prevQAvg + wq * static_cast<double>(currentQ);
  m_qAvg = newQAvg;

  m_prevDAvg = m_dAvg;
  m_dAvg = newQAvg - m_prevQAvg;
  m_sdAvg = m_dAvg - m_prevDAvg;

  UpdateMidTh(m_dAvg, m_sdAvg);

  double pb = CalculateDropProb(newQAvg);
  m_curDropProb = pb;

  if (pb >= 1.0)
    {
      m_count = 0;
      DropBeforeEnqueue(item, FORCED_DROP);
      return false;
    }
  else if (pb > 0.0)
    {
      m_count++;
      double pa = pb / (1.0 - static_cast<double>(m_count) * pb);
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
}

} // namespace ns3
