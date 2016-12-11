/*
 * simi240.h
 *
 *  Created on: Dec 10, 2016
 *      Author: chl
 *
 *
 *  Class Diagram:
 *    main()
 *    	+--use--CS240NotFabrication
 */

#ifndef SCRATCH_SIMI240_H_
#define SCRATCH_SIMI240_H_

#include <string>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/vanetmobility-helper.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

using namespace ns3;
using namespace std;

class CS240DataBasket
{
public:
  CS240DataBasket ();
  uint64_t m_RxBytes, m_Ob;
  uint64_t m_RxPackets, m_Op;
};

class CS240NotFabrication
{
public:
  CS240NotFabrication ();
  void
  Simulate (int argc, char **argv);

private:
  void
  ParseCommandLineArguments (int argc, char **argv);
  void
  LoadTraffic ();
  void
  ConfigureNodes ();
  void
  ConfigureChannels ();
  void
  ConfigureDevices ();
  void
  ConfigureMobility ();
  void
  ConfigureApplications ();
  void
  RunSimulation ();
  void
  ProcessOutputs ();

  void
  ReceivePacket (Ptr<Socket> socket);
  void
  Tick ();

  int m_port, m_nodes, m_streamIndex;
  double m_txp, m_duration;
  enum RType
  {
    AODV = 0, OLSR, DSR, DSDV
  } m_routing;
  string m_input;
  NodeContainer m_TxNodes;
  NetDeviceContainer m_TxDevices;
  Ipv4InterfaceContainer m_TxInterface;
  ofstream m_os;
  Ptr<ns3::vanetmobility::VANETmobility> m_vanetMo;
  CS240DataBasket m_data;

};

#endif /* SCRATCH_SIMI240_H_ */
