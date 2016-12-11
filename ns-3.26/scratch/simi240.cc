/*
 * simi240.cc
 *
 *  Created on: Dec 10, 2016
 *      Author: chl
 */

#include <dirent.h>//DIR*

#include "simi240.h"

CS240DataBasket::CS240DataBasket () :
    m_RxBytes (0), m_Ob (0), m_RxPackets (0), m_Op (0)
{

}

CS240NotFabrication::CS240NotFabrication () :
    m_port (419), m_nodes (0), m_streamIndex (0), m_txp (7.5), m_duration (0), m_routing (
	AODV)
{

}

void
CS240NotFabrication::Simulate (int argc, char **argv)
{
  ParseCommandLineArguments (argc, argv);
  LoadTraffic ();
  ConfigureNodes ();
  ConfigureChannels ();
  ConfigureDevices ();
  ConfigureMobility ();
  ConfigureApplications ();
  RunSimulation ();
  ProcessOutputs ();
}

void
CS240NotFabrication::ParseCommandLineArguments (int argc, char **argv)
{
  int mod = 0;
  CommandLine cmd;
  cmd.AddValue ("routing", "Routing Protocol", mod);
  cmd.AddValue ("inputFolder", "InputFolder", m_input);
  cmd.AddValue ("nodes", "nodes (invalid if inputFolder set)", m_nodes);
  cmd.AddValue ("duration", "duration (invalid if inputFolder set)",
		m_duration);
  cmd.Parse (argc, argv);
  switch (mod)
    {
    case 0:
      m_routing = AODV;
      break;
    case 1:
      m_routing = OLSR;
      break;
    case 2:
      m_routing = DSR;
      break;
    case 3:
      m_routing = DSDV;
      break;
    default:
      m_routing = AODV;
      break;
    }
}

void
CS240NotFabrication::LoadTraffic ()
{
  if (m_input.empty ())
    {
      return;
    }
  DIR* dir = NULL;
  string temp ("./" + m_input);
  if ((dir = opendir (temp.data ())) == NULL)
    NS_FATAL_ERROR("Cannot open input path "<<temp.data()<<", Aborted.");

  string sumo_net = temp + "/input.net.xml";
  string sumo_route = temp + "/input.rou.xml";
  string sumo_fcd = temp + "/input.fcd.xml";

  string output = temp + "/" + "_result_new.txt";

  m_os.open (output.data (), std::ios::out);

  ns3::vanetmobility::VANETmobilityHelper mobilityHelper;
  m_vanetMo = mobilityHelper.GetSumoMObility (sumo_net, sumo_route, sumo_fcd);

  m_nodes = m_vanetMo->GetNodeSize ();
}

void
CS240NotFabrication::ConfigureNodes ()
{
  m_TxNodes.Create (m_nodes);
}

void
CS240NotFabrication::ConfigureChannels ()
{
  //===channel
  YansWifiChannelHelper Channel;
  Channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  Channel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency",
			      DoubleValue (5.9e9));

  // the channelg
  Ptr<YansWifiChannel> CH = Channel.Create ();

  //===wifiphy
  YansWifiPhyHelper ChPhy = YansWifiPhyHelper::Default ();
  ChPhy.SetChannel (CH);
  ChPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

  // 802.11p mac
  NqosWaveMacHelper CH80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper CH80211p = Wifi80211pHelper::Default ();

  CH80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
				    StringValue ("OfdmRate3MbpsBW10MHz"),
				    "ControlMode",
				    StringValue ("OfdmRate3MbpsBW10MHz"));

  // Set Tx Power For The SCH
  ChPhy.Set ("TxPowerStart", DoubleValue (m_txp));
  ChPhy.Set ("TxPowerEnd", DoubleValue (m_txp));
  m_TxDevices = CH80211p.Install (ChPhy, CH80211pMac, m_TxNodes);
}

void
CS240NotFabrication::ConfigureDevices ()
{
  //Done in ConfigChannels()
}

void
CS240NotFabrication::ConfigureMobility ()
{
  if (!m_input.empty ())
    {
      m_vanetMo->Install ();
      m_duration = m_vanetMo->GetReadTotalTime ();
    }
  else
    {
      MobilityHelper mobilityAdhoc;

      ObjectFactory pos;
      pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
      pos.Set ("X",
	       StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
      pos.Set ("Y",
	       StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"));
      // we need antenna height uniform [1.0 .. 2.0] for loss model
      pos.Set ("Z",
	       StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"));

      Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<
	  PositionAllocator> ();
      m_streamIndex += taPositionAlloc->AssignStreams (m_streamIndex);

      mobilityAdhoc.SetMobilityModel (
	  "ns3::RandomWaypointMobilityModel", "Speed",
	  StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=60.0]"),
	  "PositionAllocator", PointerValue (taPositionAlloc));
      mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
      mobilityAdhoc.Install (m_TxNodes);
      m_streamIndex += mobilityAdhoc.AssignStreams (m_TxNodes, m_streamIndex);
    }
}

void
CS240NotFabrication::ConfigureApplications ()
{
  InternetStackHelper internet;
  OlsrHelper olsr;
  AodvHelper aodv;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;

  switch (m_routing)
    {
    case AODV:
      internet.SetRoutingHelper (aodv);
      internet.Install (m_TxNodes);
      break;
    case OLSR:
      internet.SetRoutingHelper (olsr);
      internet.Install (m_TxNodes);
      break;
    case DSDV:
      internet.SetRoutingHelper (dsdv);
      internet.Install (m_TxNodes);
      break;
    case DSR:
      internet.Install (m_TxNodes);
      dsrMain.Install (dsr, m_TxNodes);
      break;
    default:
      break;
    }

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0"); //SCH
  m_TxInterface = ipv4.Assign (m_TxDevices);

  // Setup routing transmissions
  OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address ());
  onoff1.SetAttribute (
      "OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute (
      "OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  int64_t stream = 2;
  var->SetStream (stream);
  //mesh
  for (int i = 0; i < m_nodes; i++)
    for (int j = 0; j < m_nodes; ++j)
      if (i != j)
	{
	  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	  Ipv4Address AddressJ = m_TxInterface.GetAddress (j);
	  Ptr<Node> NodeJ = m_TxNodes.Get (j);
	  Ptr<Socket> sink = Socket::CreateSocket (NodeJ, tid);
	  InetSocketAddress local = InetSocketAddress (AddressJ, m_port);
	  sink->Bind (local);
	  sink->SetRecvCallback (
	      MakeCallback (&CS240NotFabrication::ReceivePacket, this));

	  AddressValue remoteAddress (InetSocketAddress (AddressJ, m_port));
	  onoff1.SetAttribute ("Remote", remoteAddress);

	  ApplicationContainer temp = onoff1.Install (m_TxNodes.Get (i));
	  temp.Start (Seconds (var->GetValue (1.0, 2.0)));
	  temp.Stop (Seconds (m_duration));
	}

}

void
CS240NotFabrication::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
    {
      m_data.m_RxBytes += packet->GetSize ();
      m_data.m_RxPackets++;
      //cout << m_data.m_RxPackets << endl;
    }
}

void
CS240NotFabrication::RunSimulation ()
{
  Simulator::Schedule (Seconds (0.0), &CS240NotFabrication::Tick, this);
  Simulator::Stop (Seconds (m_duration));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
CS240NotFabrication::ProcessOutputs ()
{
  cout << "Total bytes received: ";
  cout << m_data.m_RxBytes;
  cout << endl;
  cout << "Total packets received: ";
  cout << m_data.m_RxPackets;
  cout << endl;
}

void
CS240NotFabrication::Tick ()
{
  cout << "Now:" << Simulator::Now ().GetSeconds () << " Rx: "
      << m_data.m_RxPackets << "Pkts, " << m_data.m_RxBytes << "B" << " (+"
      << m_data.m_RxPackets - m_data.m_Op << ",+"
      << m_data.m_RxBytes - m_data.m_Ob << ")" << endl;
  m_data.m_Ob = m_data.m_RxBytes;
  m_data.m_Op = m_data.m_RxPackets;
  Simulator::Schedule (Seconds (1.0), &CS240NotFabrication::Tick, this);
}

int
main (int argc, char *argv[])
{
  CS240NotFabrication CHL;
  CHL.Simulate (argc, argv);
}
