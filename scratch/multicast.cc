/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 * Author: Valerio Sartini <valesar@gmail.com>
 *
 * This program conducts a simple experiment: It builds up a topology based on
 * either Inet or Orbis trace files. A random node is then chosen, and all the
 * other nodes will send a packet to it. The TTL is measured and reported as an histogram.
 *
 */

#include <ctime>

#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/multicast-client-application-v4.h"
#include "ns3/resolution-application-v4.h"
#include "ns3/switch-application-v4.h"
#include "ns3/topology-read-module.h"
#include "ns3/csma-module.h"
#include <list>

/**
 * \file
 * \ingroup topology
 * Example of TopologyReader: .read in a topology in a specificed format.
 */

//  Document the available input files
/**
 * \file RocketFuel_toposample_1239_weights.txt
 * Example TopologyReader input file in RocketFuel format;
 * to read this with topology-example-sim.cc use \c --format=Rocket
 */
/**
 * \file Inet_toposample.txt
 * Example TopologyReader input file in Inet format;
 * to read this with topology-example-sim.cc use \c --format=Inet
 */
/**
 * \file Inet_small_toposample.txt
 * Example TopologyReader input file in Inet format;
 * to read this with topology-example-sim.cc use \c --format=Inet
 */
/**
 * \file Orbis_toposample.txt
 * Example TopologyReader input file in Orbis format;
 * to read this with topology-example-sim.cc use \c --format=Orbis
 */

using namespace ns3;
#define ENTRY_SWITCH_NUM 30
#define WRITE_READ_RATE 10
NS_LOG_COMPONENT_DEFINE ("TopologyMulticastV4CSMA");

// ----------------------------------------------------------------------
// -- main
// ----------------------------------------------
int main (int argc, char *argv[])
{
  LogComponentEnable ("TopologyMulticastV4CSMA", LOG_LEVEL_INFO);
  // LogComponentEnable ("Ipv4NixVectorRouting", LOG_LEVEL_INFO);
  // LogComponentEnable ("UdpL4Protocol", LOG_LEVEL_INFO);
  // LogComponentEnable ("UdpSocketImpl", LOG_LEVEL_INFO);
  // LogComponentEnable ("Socket", LOG_LEVEL_INFO);
  
  // LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_INFO);
  // LogComponentEnable ("MulticastClientApplicationv4", LOG_LEVEL_INFO);
  // LogComponentEnable ("Ipv4Address", LOG_LEVEL_INFO);
  LogComponentEnable ("SwitchApplicationv4", LOG_LEVEL_INFO);
  // LogComponentEnable ("ResolutionApplicationv4", LOG_LEVEL_INFO);
  LogComponentEnable ("SeanetEID", LOG_LEVEL_INFO);

  
  std::string format ("Inet");
  std::string input ("src/topology-read/examples/Inet_dense_3037.txt");

  // Set up command line parameters used to control the experiment.
  CommandLine cmd (__FILE__);
  cmd.AddValue ("format", "Format to use for data input [Orbis|Inet|Rocketfuel].",
                format);
  cmd.AddValue ("input", "Name of the input file.",
                input);
  cmd.Parse (argc, argv);


  // ------------------------------------------------------------
  // -- Read topology data.
  // --------------------------------------------

  // Pick a topology reader based in the requested format.
  TopologyReaderHelper topoHelp;
  topoHelp.SetFileName (input);
  topoHelp.SetFileType (format);
  Ptr<TopologyReader> inFile = topoHelp.GetTopologyReader ();
  //这两个hash表，存储每个ip地址下存储的单播/组播id列表。
  sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash> UnicastTable, MultiCastTable;
  //作为解析节点功能抽象
  sgi::hash_map<SeanetEID, std::list<Ipv4Address*>*, SeanetEIDHash>ResolutionTable;
  NodeContainer nodes;

  if (inFile != 0)
    {
      nodes = inFile->Read ();
    }

  if (inFile->LinksSize () == 0)
    {
      NS_LOG_ERROR ("Problems reading the topology file. Failing.");
      return -1;
    }

  // ------------------------------------------------------------
  // -- Create nodes and network stacks
  // --------------------------------------------
  NS_LOG_INFO ("creating internet stack");
  InternetStackHelper stack;
  Ipv4NixVectorHelper nixRouting;
  stack.SetRoutingHelper (nixRouting);  // has effect on the next Install ()
  stack.Install (nodes);

  NS_LOG_INFO ("creating ip4 addresses");
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");

  int totlinks = inFile->LinksSize ();

  NS_LOG_INFO ("creating node containers, link number "<<totlinks);
  NodeContainer* nc = new NodeContainer[totlinks];
  TopologyReader::ConstLinksIterator iter;
  int i = 0;
  for ( iter = inFile->LinksBegin (); iter != inFile->LinksEnd (); iter++, i++ )
    {
      nc[i] = NodeContainer (iter->GetFromNode (), iter->GetToNode ());
    }

  NS_LOG_INFO ("creating net device containers");
  NetDeviceContainer* ndc = new NetDeviceContainer[totlinks];
  PointToPointHelper p2p;
  for (int i = 0; i < totlinks; i++)
    {
      // p2p.SetChannelAttribute ("Delay", TimeValue(MilliSeconds(weight[i])));
      p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
      p2p.SetDeviceAttribute ("DataRate", StringValue ("500Mbps"));
      ndc[i] = p2p.Install (nc[i]);
    }

  // it crates little subnets, one for each couple of nodes.
  NS_LOG_INFO ("creating ipv4 interfaces");
  Ipv4InterfaceContainer* ipic = new Ipv4InterfaceContainer[totlinks];
  for (int i = 0; i < totlinks; i++)
    {
      ipic[i] = address.Assign (ndc[i]);
    }


  uint32_t totalNodes = nodes.GetN ();
  Ptr<UniformRandomVariable> unifRandom = CreateObject<UniformRandomVariable> ();
  unifRandom->SetAttribute ("Min", DoubleValue (0));
  unifRandom->SetAttribute ("Max", DoubleValue (totalNodes - 1));

  unsigned int randomResNumber = unifRandom->GetInteger (0, totalNodes - 1);

  Ptr<Node> randomResNode = nodes.Get (randomResNumber);
  Ptr<Ipv4> ipv4Res = randomResNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddrRes = ipv4Res->GetAddress (1,0);
  Ipv4Address ipv4AddrRes = iaddrRes.GetLocal ();
  NS_LOG_INFO("nods num :"<<nodes.GetN());
  // ipv4AddrRes.Print();

  unsigned int randomClientNumber = unifRandom->GetInteger (0, totalNodes - 1);
  Ptr<Node> randomClientNode = nodes.Get (randomClientNumber);
  // Ptr<Ipv4> ipv4Client = randomClientNode->GetObject<Ipv4> ();
  // Ipv4InterfaceAddress iaddrClient = ipv4Client->GetAddress (1,0);
  // Ipv4Address ipv4AddrClient = iaddrClient.GetLocal ();
  // NS_LOG_INFO("client address"<<InetSocketAddress::ConvertFrom (Address(ipv4AddrClient)).GetIpv4());


  // ------------------------------------------------------------
  // -- Send around packets to check the ttl
  // --------------------------------------------

  NodeContainer switchNodes, entrySwitchNodes;
  for ( unsigned int i = 0; i < nodes.GetN (); i++ )
    {
      if (i != randomResNumber && i != randomClientNumber)
        {
          Ptr<Node> switchNode = nodes.Get (i);
          if(i % ENTRY_SWITCH_NUM == 1){
            entrySwitchNodes.Add(switchNode);
          }else{
            switchNodes.Add (switchNode);
          }
        }
    }
  
  uint16_t port = 4000;
  SwitchApplicationHelperv4 switchah (ipv4AddrRes,port);
  switchah.SetAttribute("TreeType",StringValue("Seanet"));
  ApplicationContainer apps = switchah.Install (switchNodes,&UnicastTable,&MultiCastTable,&ResolutionTable,false);
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (50000.0));
  apps = switchah.Install (entrySwitchNodes,&UnicastTable,&MultiCastTable,&ResolutionTable,true);
  ResolutionApplicationHelperv4 resah (port);
  apps = resah.Install (randomResNode);
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (50000.0));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("500Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("1ms"));
  NodeContainer* csmaNodes = new NodeContainer[entrySwitchNodes.GetN()];
  NetDeviceContainer* csmaDevices = new NetDeviceContainer[entrySwitchNodes.GetN()];
  // Ipv4InterfaceContainer* csmaipic = new Ipv4InterfaceContainer[entrySwitchNodes.GetN()];
  NS_LOG_INFO("entryswitch number "<<entrySwitchNodes.GetN ());
  for ( unsigned int i = 0; i < entrySwitchNodes.GetN (); i++ )
    {
      // address.NewNetwork();
      csmaNodes[i].Add(entrySwitchNodes.Get(i));
      // csmaNodes[i].Add(randomResNode);
      csmaNodes[i].Create(1);
      csmaDevices[i] = csma.Install(csmaNodes[i]);
      stack.Install(csmaNodes[i].Get(1));
      // address.NewNetwork();
      address.Assign(csmaDevices[i]);
      // InetSocketAddress mlocal = InetSocketAddress (csmaipic[i].GetAddress(1,0), 4000);
      // NS_LOG_INFO("Assigning address "<<mlocal.GetIpv4());

      Ptr<Node> entryswitch = csmaNodes[i].Get (0);
      Ptr<Ipv4> ipv4entryswitch = entryswitch->GetObject<Ipv4> ();
      Ipv4InterfaceAddress iaddrentryswitch = ipv4entryswitch->GetAddress (1,0);
      Ipv4Address ipv4Addrentryswitch = iaddrentryswitch.GetLocal ();

      // Ptr<Node> newresaddr = csmaNodes[i].Get (1);
      // Ptr<Ipv4> ipv4newresaddr = newresaddr->GetObject<Ipv4> ();
      // Ipv4InterfaceAddress iaddrnewresaddr = ipv4newresaddr->GetAddress (1,0);
      // Ipv4Address ipv4Addrnewresaddr = iaddrnewresaddr.GetLocal ();

      uint32_t MaxPacketSize = 1024;
      Time interPacketInterval = Seconds (0.05);
      uint32_t maxPacketCount = 3200;
      MulticastClientApplicationHelperv4 clientah(ipv4Addrentryswitch,port,ipv4AddrRes,port);
      clientah.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      clientah.SetAttribute ("Interval", TimeValue (interPacketInterval));
      clientah.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
      if(i %WRITE_READ_RATE == 0){
        clientah.SetAttribute("FunctionType",StringValue("Write"));
        clientah.SetAttribute("switch_index",UintegerValue(i/WRITE_READ_RATE));
        clientah.SetAttribute("total_write_switch_num",UintegerValue(entrySwitchNodes.GetN ()/WRITE_READ_RATE));
        clientah.SetAttribute("total_multicast_group_num",UintegerValue(10000*10));
        apps = clientah.Install(csmaNodes[i].Get (1));
        apps.Start (Seconds (i/WRITE_READ_RATE*5 + 15000));
        apps.Stop (Seconds (50000.0));
      }else{
        clientah.SetAttribute("FunctionType",StringValue("Read"));
        clientah.SetAttribute("switch_index",UintegerValue(i-i/WRITE_READ_RATE));
        clientah.SetAttribute("total_write_switch_num",UintegerValue(entrySwitchNodes.GetN ()/WRITE_READ_RATE));
        clientah.SetAttribute("total_multicast_group_num",UintegerValue(10000*10));
        apps = clientah.Install(csmaNodes[i].Get (1));
        apps.Start (Seconds (200*i+20000));
        apps.Stop (Seconds (50000.0));
      }


    }


  // ------------------------------------------------------------
  // -- Run the simulation
  // --------------------------------------------
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // p2p.EnablePcapAll("topology-p2p");
  // // csma.EnablePcapAll("topology-csma");
  // AsciiTraceHelper ascii;
  // p2p.EnableAsciiAll (ascii.CreateFileStream ("multicastp2p.tr"));
  // csma.EnableAsciiAll (ascii.CreateFileStream ("multicastcsma.tr"));
  NS_LOG_INFO ("Run Simulation.");

  Simulator::Run ();
  Simulator::Destroy ();

  sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash>::iterator it = UnicastTable.begin();
  while(it != UnicastTable.end()){
    InetSocketAddress mlocal = InetSocketAddress(it->first,4000);
    std::list<uint8_t*>* lh = it->second;
    
    if(lh != NULL){
      std::cout<<"UnicastTable "<<mlocal.GetIpv4()<<" interface "<<it->first.GetInterfaceNum();
      for(std::list<uint8_t*>::iterator lhit= lh->begin(); lhit!=lh->end(); lhit++){
        uint8_t*buf = *lhit;
        std::cout<<"EID "<<buf[15]-'0'<<" "<<buf[16]-'0'<<" "<<buf[17]-'0'<<" "<<buf[18]-'0'<<" "<<buf[19]<<" ";
      }
      std::cout<<std::endl;
    }
    it++;   
  }
  it = MultiCastTable.begin();
  while(it != MultiCastTable.end()){
    InetSocketAddress mlocal = InetSocketAddress(it->first,4000);
    std::list<uint8_t*>* lh = it->second;
    
    if(lh != NULL){
      std::cout<<"MulticastTable "<<mlocal.GetIpv4()<<" interface "<<it->first.GetInterfaceNum();
      for(std::list<uint8_t*>::iterator lhit= lh->begin(); lhit!=lh->end(); lhit++){
        uint8_t*buf = *lhit;
        std::cout<<"EID "<<buf[15]-'0'<<" "<<buf[16]-'0'<<" "<<buf[17]-'0'<<" "<<buf[18]-'0'<<" "<<buf[19]-'0'<<" "<<" ";
      }
      std::cout<<std::endl;
    }
    it++;   
  }

  delete[] ipic;
  delete[] ndc;
  delete[] nc;

  NS_LOG_INFO ("Done.");

  return 0;

  // end main
}
