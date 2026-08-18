#ifndef QCT_ARED_QUEUE_DISC_H
#define QCT_ARED_QUEUE_DISC_H

#include "ns3/nstime.h"
#include "ns3/queue-disc.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"

namespace ns3
{

/**
 * \ingroup traffic-control
 * \brief Implements the QCT-ARED Active Queue Management discipline.
 */
class QctAredQueueDisc : public QueueDisc
{
  public:
    static TypeId GetTypeId(void);

    static constexpr const char* UNFORCED_DROP = "Unforced drop";
    static constexpr const char* FORCED_DROP = "Forced drop";

    QctAredQueueDisc();
    virtual ~QctAredQueueDisc() override;

    double GetAvgQueueSize() const;
    double GetMidThreshold() const;
    double GetDropProbability() const;

  protected:
    virtual bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    virtual Ptr<QueueDiscItem> DoDequeue(void) override;
    virtual Ptr<const QueueDiscItem> DoPeek(void) override;
    virtual bool CheckConfig(void) override;
    virtual void InitializeParams(void) override;

  private:
    double CalculateWq(uint32_t currentQ) const;
    void UpdateMidTh(double dAvg, double sdAvg);
    double CalculateDropProb(double avg);

    // Configuration Attributes
    double m_minTh; //!< Minimum threshold (min_th)
    double m_maxTh; //!< Maximum threshold (max_th)
    double m_midTh; //!< Target dynamic threshold (mid_th)
    double m_wq0;   //!< Base queue weight parameter (w_q0)
    double m_maxP;  //!< Maximum drop probability (max_p)
    double m_alpha; //!< mid_th adjustment increment step
    double m_beta;  //!< mid_th adjustment decrement step

    // State Variables
    TracedValue<double> m_qAvg;     //!< Instantaneous smoothed average queue
    double m_prevQAvg;              //!< q_avg(t-1)
    double m_dAvg;                  //!< First difference d_avg
    double m_prevDAvg;              //!< d_avg(t-1)
    double m_sdAvg;                 //!< Second difference sd_avg
    TracedValue<double> m_curMidTh; //!< Current mid_th tracked
    TracedValue<double> m_curDropProb;
    uint32_t m_count; //!< Packets enqueued since last drop

    Ptr<UniformRandomVariable> m_uv; //!< RNG for drop decisions
};

} // namespace ns3

#endif /* QCT_ARED_QUEUE_DISC_H */