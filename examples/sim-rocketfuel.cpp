/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

//#include <experimental/random>

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Gbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("200000p"));

  int num_nodes = 5;
  int failures = 0;
  float percent_failures = 0;
  int duration = 5;
  int min_time = 2;
  int max_time = 4;
  int enable_caller_relation = 1;
  int param_k = 10;
  int seed = 1;
  int first_node_id = 120;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue ("NumNodes", "Number of nodes", num_nodes);
  cmd.AddValue ("Failures", "Faiures/yes or no", failures);
  cmd.AddValue ("PercentFailures", "Percent of failures", percent_failures);
  cmd.AddValue ("Duration", "Simulation Duration", duration);
  cmd.AddValue ("MinTime", "Min Time for Failures", min_time);
  cmd.AddValue ("MaxTime", "Max Time for Failures", max_time);
  cmd.AddValue ("CallerRelation", "Enable/disable caller relation", enable_caller_relation);
  cmd.AddValue ("ParamK", "Value of parameter k", param_k);
  cmd.AddValue ("Seed", "Random number generator seed", seed);
  cmd.AddValue ("FirstNodeId", "ID of first node", first_node_id);
  cmd.Parse(argc, argv);


  RngSeedManager::SetSeed(seed);

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min_time));
  x->SetAttribute ("Max", DoubleValue (max_time));

  AnnotatedTopologyReader topologyReader ("", 1.0);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/rocketfuel.txt");
  topologyReader.Read ();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/graph/program", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/exec", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/util", "/localhost/nfd/strategy/multicast");

  std::string hint = "/node";

  // install CFN nodes
  for (int i = 0; i < num_nodes; i++) {
    int rand_num = rand() % topologyReader.GetNodes().GetN();
    if (i == 0) {
      rand_num = first_node_id;
    }
    bool trueFalse = false;
    if (failures == 1 && i > 0) {
      trueFalse = (rand() % 100) < percent_failures;
    }
    ndn::AppHelper consumerHelper("ns3::ndn::CFN");
    // Consumer will request /prefix/0, /prefix/1, ...
    consumerHelper.SetPrefix("/graph/program");
    consumerHelper.SetAttribute("Function", StringValue("/exec"));
    consumerHelper.SetAttribute("Hint", StringValue(hint+std::to_string(rand_num)));
    consumerHelper.SetAttribute("NumNodes", StringValue(std::to_string(num_nodes)));
    consumerHelper.SetAttribute("NumNode", StringValue(std::to_string(rand_num)));
    auto apps = consumerHelper.Install(topologyReader.GetNodes().Get(rand_num));
    if (trueFalse) {
      std::cout << "Node: " << i << " will fail in: " << x->GetValue() << "sec" << "\n";
      apps.Stop(Seconds(x->GetValue()));
    }
    else {
      apps.Stop(Seconds(duration - 1));
    }
    ndnGlobalRoutingHelper.AddOrigins("/graph/program", topologyReader.GetNodes().Get(rand_num));
    ndnGlobalRoutingHelper.AddOrigins("/exec", topologyReader.GetNodes().Get(rand_num));
    ndnGlobalRoutingHelper.AddOrigins("/util", topologyReader.GetNodes().Get(rand_num));
    ndnGlobalRoutingHelper.AddOrigins(hint+std::to_string(rand_num), topologyReader.GetNodes().Get(rand_num));
  }
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  //p2p.Install(nodes.Get(1), nodes.Get(2));
  //p2p.Install(nodes.Get(2), nodes.Get(3));
  //p2p.Install(nodes.Get(3), nodes.Get(4));

  //
  //
  //
  // for (int i = 0; i < num_nodes; i++) {
  //   ndn::AppHelper consumerHelper("ns3::ndn::RealGraphLocality");
  //   // Consumer will request /prefix/0, /prefix/1, ...
  //   consumerHelper.SetPrefix("/graph/program");
  //   consumerHelper.SetAttribute("Function", StringValue("/exec"));
  //   consumerHelper.SetAttribute("Hint", StringValue(hint+std::to_string(i)));
  //   consumerHelper.SetAttribute("NumNodes", StringValue(std::to_string(num_nodes)));
  //   consumerHelper.SetAttribute("NumNode", StringValue(std::to_string(i)));
  //   auto apps = consumerHelper.Install(nodes.Get(i));
  //   ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(i));
  //   ndnGlobalRoutingHelper.AddOrigins("/exec", nodes.Get(i));
  //   ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(i));
  //   ndnGlobalRoutingHelper.AddOrigins(hint+std::to_string(i), nodes.Get(i));
  // }
  //
  // // ndnGlobalRoutingHelper.AddOrigins("/test", nodes.Get(3));
  // // ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(3));
  // // ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(3));
  // // ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(3));
  // //
  // // ndnGlobalRoutingHelper.AddOrigins("/test2", nodes.Get(4));
  // // ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(4));
  // // ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(4));
  // // ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(4));
  //
  // // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(duration));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
