#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/qct-ared-queue-disc.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QctAredValidation");

int main(int argc, char* argv[])
{
  std::string queueType = "QCT";
  uint32_t nSources = 50;
  double simTime = 20.0;
  uint32_t seed = 1;

  CommandLine cmd(__FILE__);
  cmd.AddValue("queueType", "Queue discipline to test (RED, ARED, QCT)", queueType);
  cmd.AddValue("nSources", "Number of sender/receiver pairs", nSources);
  cmd.AddValue("simTime", "Simulation duration in seconds", simTime);
  cmd.AddValue("seed", "Random seed", seed);
  cmd.Parse(argc, argv);

  RngSeedManager::SetSeed(seed);
  RngSeedManager::SetRun(1);

  // 1. Configure Links per Paper Section 4.1
  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
  accessLink.SetChannelAttribute("Delay", StringValue("10ms"));

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  bottleneckLink.SetChannelAttribute("Delay", StringValue("40ms"));

  PointToPointDumbbellHelper dumbbell(nSources, accessLink, nSources, accessLink, bottleneckLink);

  // 2. Install Network Stack
  InternetStackHelper stack;
  dumbbell.InstallStack(stack);

  dumbbell.AssignIpv4Addresses(
      Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
      Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
      Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

  // 3. Configure Queue Discipline on Bottleneck Router
  TrafficControlHelper tch;
  if (queueType == "QCT")
    {
      tch.SetRootQueueDisc("ns3::QctAredQueueDisc",
                           "MinTh", DoubleValue(24.0),
                           "MaxTh", DoubleValue(72.0),
                           "Wq0", DoubleValue(0.002),
                           "MaxP", DoubleValue(0.1));
    }
  else if (queueType == "ARED")
    {
      tch.SetRootQueueDisc("ns3::RedQueueDisc",
                           "MinTh", DoubleValue(24.0),
                           "MaxTh", DoubleValue(72.0),
                           "QW", DoubleValue(0.002),
                           "LInterm", DoubleValue(0.1),
                           "ARED", BooleanValue(true));
    }
  else
    {
      tch.SetRootQueueDisc("ns3::RedQueueDisc",
                           "MinTh", DoubleValue(24.0),
                           "MaxTh", DoubleValue(72.0),
                           "QW", DoubleValue(0.002),
                           "LInterm", DoubleValue(0.1),
                           "ARED", BooleanValue(false));
    }

  // Uninstall the default FqCoDel queue disc before installing RED/QCT
  tch.Uninstall(dumbbell.GetLeft()->GetDevice(0));
  tch.Install(dumbbell.GetLeft()->GetDevice(0));

  // 4. Install BulkSend Applications
  uint16_t port = 50000;
  for (uint32_t i = 0; i < nSources; ++i)
    {
      Address sinkAddress(InetSocketAddress(dumbbell.GetRightIpv4Address(i), port));
      PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
      ApplicationContainer sinkApp = sinkHelper.Install(dumbbell.GetRight(i));
      sinkApp.Start(Seconds(0.0));
      sinkApp.Stop(Seconds(simTime));

      BulkSendHelper source("ns3::TcpSocketFactory", sinkAddress);
      source.SetAttribute("MaxBytes", UintegerValue(0));
      ApplicationContainer sourceApp = source.Install(dumbbell.GetLeft(i));
      sourceApp.Start(Seconds(0.1 + (i * 0.02)));
      sourceApp.Stop(Seconds(simTime));
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 5. FlowMonitor for Output Metrics
  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  uint64_t totalRxBytes = 0;
  uint64_t totalTxPackets = 0;
  uint64_t totalRxPackets = 0;
  uint64_t totalLostPackets = 0;
  double sumDelaySec = 0.0;
  double sumJitterSec = 0.0;

  for (auto const& [flowId, flowStats] : stats)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);
      if (t.destinationPort == port)
        {
          totalRxBytes += flowStats.rxBytes;
          totalTxPackets += flowStats.txPackets;
          totalRxPackets += flowStats.rxPackets;
          totalLostPackets += flowStats.lostPackets;

          if (flowStats.rxPackets > 0)
            {
              sumDelaySec += flowStats.delaySum.GetSeconds();
              sumJitterSec += flowStats.jitterSum.GetSeconds();
            }
        }
    }

  double throughputMbps = (totalRxBytes * 8.0) / (simTime * 1e6);
  double lossRatePct = totalTxPackets > 0 ? (100.0 * totalLostPackets / totalTxPackets) : 0.0;
  double avgDelayMs = totalRxPackets > 0 ? (sumDelaySec / totalRxPackets) * 1000.0 : 0.0;
  double avgJitterMs = totalRxPackets > 0 ? (sumJitterSec / totalRxPackets) * 1000.0 : 0.0;

  std::cout << "\n=======================================================\n";
  std::cout << " [VALIDATION RUN] Alg: " << queueType << " | N = " << nSources << " sources\n";
  std::cout << "=======================================================\n";
  std::cout << " Throughput : " << std::fixed << std::setprecision(3) << throughputMbps << " Mbps\n";
  std::cout << " Loss Rate  : " << std::fixed << std::setprecision(2) << lossRatePct << " %\n";
  std::cout << " Avg Delay  : " << std::fixed << std::setprecision(2) << avgDelayMs << " ms\n";
  std::cout << " Jitter     : " << std::fixed << std::setprecision(3) << avgJitterMs << " ms\n";
  std::cout << "=======================================================\n\n";

  Simulator::Destroy();
  return 0;
}
