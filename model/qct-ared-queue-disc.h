#ifndef QCT_ARED_QUEUE_DISC_H
#define QCT_ARED_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"

namespace ns3 {

class QctAredQueueDisc : public QueueDisc {
public:
  static TypeId GetTypeId(void);

  static constexpr const char* UNFORCED_DROP = "Unforced drop";
  static constexpr const char* FORCED_DROP = "Forced drop";

  QctAredQueueDisc();
  virtual ~QctAredQueueDisc() override;

  double GetAvgQueueSize() const;
  double GetMidThreshold() const;
  double GetDropProbability() const;

  double GetDavg() const;
  double GetSdavg() const;

protected:
  virtual bool DoEnqueue(Ptr<QueueDiscItem> item) override;
  virtual Ptr<QueueDiscItem> DoDequeue(void) override;
  virtual Ptr<const QueueDiscItem> DoPeek(void) override;
  virtual bool CheckConfig(void) override;
  virtual void InitializeParams(void) override;

private:
  double CalculateWq(double avg) const;
  void UpdateMidTh(double dAvg, double sdAvg);
  double CalculateDropProb(double avg, double dAvg, double sdAvg);

  double m_minTh;
  double m_maxTh;
  double m_midTh;
  double m_wq0; // [2]
  double m_maxP;
  double m_alpha;
  double m_beta;

  TracedValue<double> m_qAvg;
  double m_instPrev1;  // inst(t)
  double m_dAvg; // [3]
  double m_instPrev2;  // inst(t-1)
  double m_sdAvg; // [4]
  TracedValue<double> m_curMidTh; // [5]
  TracedValue<double> m_curDropProb; // [6 - 14]
  TracedValue<double> m_curDavg;     // mirrors m_dAvg, for tracing/plots
  TracedValue<double> m_curSdavg;    // mirrors m_sdAvg, for tracing/plots
  uint32_t m_count; // fig.4 

  Ptr<UniformRandomVariable> m_uv;
};

} // namespace ns3

#endif /* QCT_ARED_QUEUE_DISC_H */