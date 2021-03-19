/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "packet-loss-counter.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/random-variable-stream.h"
#include "switch-application-v4.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include "ns3/ipv4.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwitchApplicationv4");

NS_OBJECT_ENSURE_REGISTERED (SwitchApplicationv4);

TypeId
SwitchApplicationv4::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SwitchApplicationv4")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<SwitchApplicationv4> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&SwitchApplicationv4::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("CacheSize", "Max cache size for each switch",
                         UintegerValue (100), MakeUintegerAccessor (&SwitchApplicationv4::m_cache_size),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&SwitchApplicationv4::GetPacketWindowSize,
                                               &SwitchApplicationv4::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddAttribute ("Resolutionaddr",
                        "The destination Address of the outbound packets",
                        AddressValue (),
                        MakeAddressAccessor (&SwitchApplicationv4::resolution_addr),
                        MakeAddressChecker ())
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&SwitchApplicationv4::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&SwitchApplicationv4::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

SwitchApplicationv4::SwitchApplicationv4 () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;

}
void SwitchApplicationv4::SetNeighInfoTable
  (sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* sct,
  sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* mct){
    UnicastTable = sct;
    // NS_LOG_INFO("Initialize HASHTABLE "<<UnicastTable.size()<<" "<<UnicastTable.begin()->second);
    MultiCastTable = mct;
}
SwitchApplicationv4::~SwitchApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
SwitchApplicationv4::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
SwitchApplicationv4::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
SwitchApplicationv4::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
SwitchApplicationv4::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
SwitchApplicationv4::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SwitchApplicationv4::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      Ptr<Node> mynode = GetNode();
      Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();

      // Ptr<NetDevice>mydevice = mynode->GetDevice(0);
      // Address mlocalAddress = mydevice->GetAddress();
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);

      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      // m_socket->GetSockName (localAddress);
      for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++ )
      {
        // Get the primary address
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress (i, 0);
        
        Ipv4Address addri = iaddr.GetLocal ();
        if (addri == Ipv4Address ("127.0.0.1"))
          continue;
        // if (addri == Ipv4Address ("10.0.0.16")){
        //   Simulator::Schedule (Seconds (1), &SwitchApplicationv4::DetecAllNeighborDelay, this);
        // }
        (*MultiCastTable)[addri]=0;
        (*UnicastTable)[addri]=0;
        localAddress = Address(addri);
        InetSocketAddress mlocal = InetSocketAddress (addri, m_port);
        NS_LOG_INFO("switch local address "<<mlocal.GetIpv4());
        break;
      }

    }

  m_socket->SetRecvCallback (MakeCallback (&SwitchApplicationv4::FrontEnd, this));
  packetin = CreateObject<DropTailQueue<Packet>> ();
  addressin = CreateObject<DropTailQueue<SeanetAddress>> ();
  // m_AfterEndEvent = Simulator::Schedule (Seconds (0), &SwitchApplicationv4::AferEnd, this);
  Simulator::Schedule (Seconds (1), &SwitchApplicationv4::DetecAllNeighborDelay, this);
  
  m_temp_cache_size = 0;

  // m_RandomCacheEvent = Simulator::Schedule (Seconds (0), &SwitchApplicationv4::RandomCache, this,m_temp_cache_size);
  // AferEnd ();
}

void
SwitchApplicationv4::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
  Simulator::Cancel(m_AfterEndEvent);
}
Ipv4Address SwitchApplicationv4::GetRootNode(){
  sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>::iterator scti = UnicastTable->begin();
  Ipv4Address selected_address = scti->first;
  double selected_score = 10000;
  double temp_score = 0;

  while(scti!=UnicastTable->end()){
    Ipv4Address tempkey = scti->first;
    if(MultiCastTable->find(tempkey)== MultiCastTable->end()){
      InetSocketAddress mlocal = InetSocketAddress (tempkey, m_port);
      NS_LOG_INFO("GetRootNode not find key "<< mlocal.GetIpv4());
      (*MultiCastTable)[tempkey]=0;
      // break;
    }
    temp_score = scti->second* 0.5 + ((MultiCastTable->find(tempkey))->second)*0.5;
    if(temp_score < selected_score){
      selected_score = temp_score;
      selected_address = scti->first;
    }
    scti++;
  }
  return selected_address;
}

void
SwitchApplicationv4::FrontEnd (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      resolution_addr.m_type = from.m_type;
      if (packet->GetSize () > 0)
        {
          // NS_LOG_INFO("front end get a packet");
          m_rxTrace (packet);
          m_rxTraceWithAddresses (packet, from, localAddress);
          if (packet->GetSize () > 0)
            {
              SeanetHeader ssenh;
              packet->RemoveHeader (ssenh);
              uint32_t application_type = ssenh.GetApplicationType();
              uint32_t protocol_type = ssenh.GetProtocolType();
              uint32_t is_dst = ssenh.Getdst();
              Ipv4Address i4a = Ipv4Address::ConvertFrom(localAddress);
              InetSocketAddress mlocal = InetSocketAddress (i4a, m_port);
              uint8_t fromaddr[18];
              from.CopyTo(fromaddr);
              Ipv4Address fromipv4=Ipv4Address::Deserialize (fromaddr);
          // uint16_t port= addr[16] | (addr[17] << 8);
              InetSocketAddress fromipv4isa = InetSocketAddress (fromipv4, m_port);
              NS_LOG_LOGIC("switch address "<<mlocal.GetIpv4()<<" is_dst "<<is_dst
              <<" from address "<<fromipv4isa.GetIpv4()<<" application_type "<<application_type
              <<" protoco type "<<protocol_type);
              if(is_dst == NOT_DST){
                switch (application_type)
                {
                case MULTICAST_APPLICATION:
                  if(protocol_type == REGIST_TO_RN){
                    uint8_t addr[20];
                    from.CopyTo(addr);
                    Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
                    InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);

                    localAddress.CopyTo(addr);
                    Ipv4Address m_ipv4 = Ipv4Address::Deserialize (addr);
                    InetSocketAddress m_i4a = InetSocketAddress (m_ipv4, m_port);
                    NS_LOG_INFO("Unicast Trans passing Node "<< m_i4a.GetIpv4()<< " from "<< i4a.GetIpv4());
               
                    (*UnicastTable)[m_ipv4]=1;
                  }else if(protocol_type == MULTICAST_DATA_TRANS){
                    uint8_t addr[20];
                    from.CopyTo(addr);
                    Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
                    InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);

                    localAddress.CopyTo(addr);
                    Ipv4Address m_ipv4 = Ipv4Address::Deserialize (addr);
                    InetSocketAddress m_i4a = InetSocketAddress (m_ipv4, m_port);
                    NS_LOG_INFO("Multicast Trans passing Node "<< m_i4a.GetIpv4()<< " from "<< i4a.GetIpv4());
            
                    if(MultiCastTable->find(m_ipv4)!=MultiCastTable->end()){
                      (*MultiCastTable)[m_ipv4]=1;
                      uint8_t buffer[MAX_PAYLOAD_LEN];
                      int buffer_len = packet->CopyData(buffer, MAX_PAYLOAD_LEN);
                      SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
                      SendPacket(buffer,buffer_len,ssenh,resolution_addr);
                    }


                  }
                case NEIGH_INFO_APPLICATION:{
                  // NS_LOG_INFO("Passing neigh detection");
                  break;
                }
                default:
                  break;
                }
                return ;
              }else{
                switch (application_type){
                  case RESOLUTION_APPLICATION:{
                    // NS_LOG_INFO("Switch can't process this application type");
                    uint8_t buffer[MAX_PAYLOAD_LEN];
                    int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
                    SwitchApplicationv4::ResolutionProtocolHandle(buffer,buffer_len,protocol_type,from);
                    break;
                  }
                  case SWITCH_APPLICATION:{
                    uint8_t buffer[MAX_PAYLOAD_LEN];
                    int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
                    SwitchApplicationv4::SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
                    break;
                  }
                  case NEIGH_INFO_APPLICATION:{
                    // NS_LOG_INFO("switch recieve neigh detect");
                    SeqTsSizeHeader stsh;
                    packet->RemoveHeader(stsh);
                    uint8_t buffer[MAX_PAYLOAD_LEN];
                    int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
                    NeigthInfoProtocolHandle(buffer,buffer_len,protocol_type,from,stsh);
                    break;
                  }
                  case MULTICAST_APPLICATION:{
                    if(protocol_type == REGIST_TO_SOURCE_DR){//源端DR收到该包后，向RN节点注册
                      NS_LOG_INFO("DR receive multicast regist");
                      Ipv4Address rni4a = SwitchApplicationv4::GetRootNode();
                      uint8_t buffer[MAX_PAYLOAD_LEN];
                      int buffer_len = packet->CopyData(buffer, MAX_PAYLOAD_LEN);
                      SeanetHeader ssenh(MULTICAST_APPLICATION,REGIST_TO_RN);
                      SendPacket(buffer,buffer_len,ssenh,Address(rni4a));
                    }else if(protocol_type == REGIST_TO_RN){//RN收到该包后，向解析注册
                      NS_LOG_INFO("RN receive multicast regist");
                      Ipv4Address i4a = Ipv4Address::ConvertFrom(localAddress);
                      (*UnicastTable)[i4a]=1;
                      (*MultiCastTable)[i4a]=1;
                      uint8_t buffer[MAX_PAYLOAD_LEN];
                      int buffer_len = packet->CopyData(buffer, MAX_PAYLOAD_LEN);
                      SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
                      SendPacket(buffer,buffer_len,ssenh,resolution_addr);
                    }else if(protocol_type == GRAFITING_REQUEST){//组播节点收到嫁接请求，回复数据
                      uint8_t addr[20];
                      from.CopyTo(addr);
                      Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
                      InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);

                      localAddress.CopyTo(addr);
                      Ipv4Address m_ipv4 = Ipv4Address::Deserialize (addr);
                      InetSocketAddress m_i4a = InetSocketAddress (m_ipv4, m_port);
                      NS_LOG_INFO("Multicast Node"<< m_i4a.GetIpv4()<< " receive request from "<< i4a.GetIpv4());
                      uint8_t buffer[MAX_PAYLOAD_LEN];
                      int buffer_len = packet->CopyData(buffer, MAX_PAYLOAD_LEN);
                      SeanetHeader ssenh(MULTICAST_APPLICATION,MULTICAST_DATA_TRANS);
                      SendPacket(buffer,buffer_len,ssenh,from);           
                    }else if(protocol_type == REGIST_TO_DEST_DR){
                      //收端DR收到客户端的组播接收请求,选择最近的组播管理节点。这里可以直接向解析发送请求，解析回复iplist
                      NS_LOG_INFO("DR receive multicast request");
                      uint8_t buffer[MAX_PAYLOAD_LEN];
                      int buffer_len = packet->CopyData(buffer, MAX_PAYLOAD_LEN);
                      SeanetHeader ssenh(RESOLUTION_APPLICATION,REQUEST_EID_NA);
                      SendPacket(buffer,buffer_len,ssenh,resolution_addr); 
                    }else if(protocol_type == MULTICAST_DATA_TRANS){
                      
                      uint8_t addr[20];
                      from.CopyTo(addr);
                      Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
                      InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);

                      localAddress.CopyTo(addr);
                      Ipv4Address m_ipv4 = Ipv4Address::Deserialize (addr);
                      InetSocketAddress m_i4a = InetSocketAddress (m_ipv4, m_port);
                      (*MultiCastTable)[m_ipv4]=1;
                      
                      NS_LOG_INFO("DR Node "<< m_i4a.GetIpv4()<< " receive multicast data from "<< i4a.GetIpv4());
                    }
                    break;
                  }
                  default:
                  NS_LOG_INFO("Switch can't process this application type "<< application_type);
                  break;
                }
              }
              // SeanetEID se(buffer);
              // SwitchApplicationv4::AddEIDTable(se,1);

              m_received++;
              // NS_LOG_INFO ("received packet " << m_received);
            }
            
        }
    }
}

uint8_t SwitchApplicationv4::LookupEIDTable(SeanetEID se){
  if (m_eid_table.find (se) != m_eid_table.end ())
    {
      uint8_t entry = m_eid_table[se];
      // NS_LOG_INFO ("SwitchApplicationv4 Found an entry: " << entry);

      return entry;
    }
  // NS_LOG_INFO ("SwitchApplicationv4 Nothing found");
  return 0;
}
uint8_t SwitchApplicationv4::AddEIDTable(SeanetEID se, uint8_t value){
  uint8_t v = SwitchApplicationv4::LookupEIDTable(se);
  if(v == 0){
    m_eid_table[se] = value;
    return value;
  }
  return 0;
}
void SwitchApplicationv4::SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len,uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REQUEST_DATA:
    SwitchApplicationv4::RequestDataHandle(buffer,buffer_len, from);
    break;
  case RESEIVE_DATA:
    SwitchApplicationv4::ReceiveDataHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}

void SwitchApplicationv4::RequestDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  // NS_LOG_INFO("Switch node get a data request");
  if(SwitchApplicationv4::LookupEIDTable(se)!=0){
    SeanetHeader ssenh(SWITCH_APPLICATION,REPLY_DATA);
    SwitchApplicationv4::SendPacket(buffer,EIDSIZE,ssenh,from);
  }
}
void SwitchApplicationv4::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len, uint8_t protocol_type, Address from){
    switch (protocol_type)
    {
    case REPLY_EID_NA:{
      SeanetEID se(buffer);
      uint32_t finished = buffer[EIDSIZE];
      uint32_t ipnum = buffer[EIDSIZE+1];
      Time shortestime(1000000000);
      Ipv4Address shortestip;
      if(m_eid_na_table.find(se)!=m_eid_na_table.end()){
        shortestip=m_eid_na_table[se];
        shortestime = DelayTable[shortestip];
      }
      for(int i = 0; i < ipnum; i++){
        Ipv4Address ipv4=Ipv4Address::Deserialize (buffer+EIDSIZE+2 + 18 * i);
        if(DelayTable.find(ipv4)!=DelayTable.end()){
          if(DelayTable[ipv4]<shortestime){
            shortestime = DelayTable[ipv4];
            shortestip = ipv4;
          }
        }else{
          SwitchApplicationv4::NeighInfoDetec(ipv4,m_port);
        }
      }
      if(finished==1){
        uint8_t buf[MAX_PAYLOAD_LEN];
        se.getSeanetEID(buf);
        SeanetHeader ssenh(MULTICAST_APPLICATION,GRAFITING_REQUEST);
        SendPacket(buf,MAX_PAYLOAD_LEN,ssenh,shortestip); 
        InetSocketAddress i4a = InetSocketAddress (shortestip, m_port);
        NS_LOG_INFO("grafting to nearest switch "<<i4a.GetIpv4());
      }
      m_eid_na_table[se]=shortestip;

      break;
    }
    default:
      break;
    }
}
void SwitchApplicationv4::ReceiveDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  // NS_LOG_INFO("Switch node get a data packet");
  SwitchApplicationv4::AddEIDTable(se,1);
  // NS_LOG_INFO("ReceiveDataHandle");
  SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
  SwitchApplicationv4::SendPacket(buffer,EIDSIZE,ssenh,resolution_addr);
}

void SwitchApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
    int res = m_socket->SendTo(p,0,i4a);
    if(res == -1){
      NS_LOG_INFO("Switch send failed");
    } 
    // Simulator::Schedule (Seconds (0.001), &(Socket::SendTo),p,0,i4a);
}
void SwitchApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,SeqTsSizeHeader stsh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (stsh);
    p->AddHeader (ssenh);
    
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    int res = m_socket->SendTo(p,0,i4a);
    if(res == -1){
      NS_LOG_INFO("Switch send failed");
    }
    
}
void SwitchApplicationv4::RandomCache(uint16_t size){
  NS_ASSERT(m_RandomCacheEvent.IsExpired());
  Ptr<UniformRandomVariable> unifRandom = CreateObject<UniformRandomVariable> ();
  
  unifRandom->SetAttribute ("Min", DoubleValue (0));
  unifRandom->SetAttribute ("Max", DoubleValue (TOTAL_FILE_NUM - 1));
  // for(int i = 0; i < size; i++){
    uint32_t randomEID = unifRandom->GetInteger (0, TOTAL_FILE_NUM - 1);
    // char buffer[EIDSIZE];
    // NS_LOG_INFO("in switch RandomCache "<< randomEID);
    std::stringstream ss;

    ss << std::setw(20) << std::setfill('0') << randomEID ;
    std::string buf = ss.str();
    // NS_LOG_INFO("buf len "<< buf.length());
    char buf_c[EIDSIZE];
    memcpy(buf_c,buf.c_str(),EIDSIZE);

    SeanetEID se((uint8_t*)buf_c);
    // se.Print();
    SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
    SwitchApplicationv4::SendPacket((uint8_t*)buf_c,EIDSIZE,ssenh,resolution_addr);
    m_temp_cache_size++;
    if(m_temp_cache_size < m_cache_size){
      m_RandomCacheEvent = Simulator::Schedule (Seconds (0.1), 
              &SwitchApplicationv4::RandomCache, this,m_temp_cache_size);
    }
  }
void SwitchApplicationv4::NeigthInfoProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
                            uint8_t protocoal_type, Address from,SeqTsSizeHeader stsh){
    switch (protocoal_type)
    {
    case NEIGH_INFO_RECEIVE:{
      
      // uint8_t addr[18];
      // from.CopyTo(addr);
      // Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
      // // uint16_t port= addr[16] | (addr[17] << 8);
      // // InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
      // localAddress.CopyTo(addr);
      // Ipv4Address lipv4 = Ipv4Address::Deserialize (addr);
      // NS_LOG_INFO("switch "<<InetSocketAddress(lipv4,m_port).GetIpv4()<<  " recieve neigh detect, reply to "<<i4a.GetIpv4());
      SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_REPLY);
      SendPacket(buffer,buffer_len,ssenh,stsh,from,m_port);
      break;
    }
    case NEIGH_INFO_REPLY:{
      uint8_t addr[18];
      from.CopyTo(addr);
      Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
      // InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
      // localAddress.CopyTo(addr);
      // Ipv4Address lipv4 = Ipv4Address::Deserialize (addr);
      Time delay = Simulator::Now () - stsh.GetTs ();
      // NS_LOG_INFO("switch"<< InetSocketAddress(lipv4,m_port).GetIpv4()<<" receive neigh result, delay:"<<delay.GetTimeStep()<<" address "<<i4a.GetIpv4());
      DelayTable[ipv4]=delay;

      break;
  }
    default:
      break;
    }
  }
void SwitchApplicationv4::DetecAllNeighborDelay(){
  sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>::iterator mctit= MultiCastTable->begin();
  while(mctit != MultiCastTable->end()){
    Ipv4Address i4a = mctit->first;

    Simulator::Schedule(Seconds(0.1),&SwitchApplicationv4::NeighInfoDetec,this,Address(i4a),m_port);
    mctit++;
  }
}
void SwitchApplicationv4::NeighInfoDetec(Address dst_ip,uint16_t dst_port){
  SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_RECEIVE,IS_DST);
  SeqTsSizeHeader stsh;
  stsh.SetSeq(0);
  stsh.SetSize(10);
  uint8_t buffer[10];
  memset(buffer,'6',10);
  SendPacket(buffer,10,ssenh,stsh,dst_ip,dst_port);
}
} // Namespace ns3
