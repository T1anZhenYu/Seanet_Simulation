/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
#include <cstdlib>
#include <cstdio>
#include "multicast-client-application-v4.h"
#include <ns3/string.h>
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-interface-address.h"
#define EID_UNIT 7000//最大不能超过10000
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MulticastClientApplicationv4");

NS_OBJECT_ENSURE_REGISTERED (MulticastClientApplicationv4);

TypeId
MulticastClientApplicationv4::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MulticastClientApplicationv4")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<MulticastClientApplicationv4> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (1),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&MulticastClientApplicationv4::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("SwitchAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&MulticastClientApplicationv4::m_switch_address),
                   MakeAddressChecker ())
    .AddAttribute ("SwitchPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::m_switch_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ResAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&MulticastClientApplicationv4::m_resolution_address),
                   MakeAddressChecker ())
    .AddAttribute ("ResPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::m_resolution_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
    .AddAttribute ("FunctionType",
                   "Read or Write or all",
                   StringValue ("Read"),
                   MakeStringAccessor (&MulticastClientApplicationv4::function_type),
                   MakeStringChecker ())
    .AddAttribute ("total_write_switch_num",
                   "total_write_switch_num",
                   UintegerValue (1),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::total_switch_num),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("total_multicast_group_num",
                   "total_multicast_group_num",
                   UintegerValue (1),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::total_multicast_group_num),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("switch_index",
                   "switch_index",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MulticastClientApplicationv4::switch_index),
                   MakeUintegerChecker<uint32_t>())
              
  ;
  return tid;
}

MulticastClientApplicationv4::MulticastClientApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_switch_socket = 0;
  function_type = "Read";
  m_count = 1;
  m_sendEvent = EventId ();
  m_readEvent = EventId ();
}

MulticastClientApplicationv4::~MulticastClientApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
}

void
MulticastClientApplicationv4::SetRemote (Address switch_ip,Address res_ip,uint16_t res_port, uint16_t switch_port)
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip << res_port <<switch_port);
  m_switch_address = switch_ip;
  m_switch_port = switch_port;
  m_resolution_address = res_ip;
  m_resolution_port = res_port;
}

void
MulticastClientApplicationv4::SetRemote (Address switch_ip,Address res_ip )
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip);
  m_switch_address = switch_ip;
  m_resolution_address = res_ip;
}

void
MulticastClientApplicationv4::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
MulticastClientApplicationv4::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_switch_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_switch_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_switch_port);
      if (m_switch_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_switch_socket->GetSockName (m_local_address);
    }
  Ptr<Node> mynode = GetNode();
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();

  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++ )
  {
    // Get the primary address
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress (i, 0);
    Ipv4Address addri = iaddr.GetLocal ();
    if (addri == Ipv4Address ("127.0.0.1"))
      continue;
    m_local_address = Address(addri);
    // InetSocketAddress mlocal = InetSocketAddress (addri, m_switch_port);
    // NS_LOG_INFO("client local address "<<mlocal.GetIpv4());
    break;
  }
  if(function_type=="Read"){
    m_count = total_switch_num*EID_UNIT;
  }else if(function_type == "Write"){
    m_count = EID_UNIT;
  }else{
    m_count = 10;
  }
  m_sent = 0;


  m_switch_socket->SetRecvCallback (MakeCallback (&MulticastClientApplicationv4::ReceiveCallback, this));
  m_switch_socket->SetAllowBroadcast (true);
  // m_resolution_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  // m_resolution_socket->SetAllowBroadcast (true);
  if(function_type=="Write"){
    Simulator::Schedule (Seconds (0.0), &MulticastClientApplicationv4::Write, this);
  }else if(function_type=="Read"){
    // NS_LOG_INFO("BEFORE READDD");
    Simulator::Schedule (Seconds (switch_index*20), &MulticastClientApplicationv4::Read, this);
  }else if(function_type=="All"){
    Simulator::Schedule (Seconds (0.0), &MulticastClientApplicationv4::Write, this);
    // Simulator::Schedule (Seconds (1.0), &MulticastClientApplicationv4::Write, this);
    Simulator::Schedule (Seconds (switch_index*20), &MulticastClientApplicationv4::Read, this);
  }
}

void
MulticastClientApplicationv4::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_readEvent);
}

void
MulticastClientApplicationv4::Write (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeanetHeader ssenh(MULTICAST_APPLICATION,REGIST_TO_SOURCE_DR);
  
  uint8_t buffer[30];
  memcpy(buffer,"11111111111111111111",20);
  
  if (m_sent < m_count){
      m_sent++;
      buffer[18]+=m_sent%100;
      buffer[17]+=(uint8_t)(m_sent/100);
      buffer[16]+=switch_index;
      NS_LOG_INFO("client Write "<<(uint32_t)m_sent<<" total "<<(uint32_t)m_count<<" EID "
                      <<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                      <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));
      SendPacket(buffer,20,ssenh,m_switch_address,m_switch_port);
      Simulator::Schedule (Seconds(0.01), &MulticastClientApplicationv4::Write, this);
  }
}

void
MulticastClientApplicationv4::Read (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_readEvent.IsExpired ());
  SeanetHeader ssenh(MULTICAST_APPLICATION ,REGIST_TO_DEST_DR);
  uint8_t buffer[30];
  memcpy(buffer,"11111111111111111111",20);
  if (m_sent < m_count){
      m_sent++;
      buffer[18]+= m_sent%100;
      buffer[17]+= (uint8_t)(m_sent/100)%(EID_UNIT/100);
      buffer[16]+= (uint8_t)(m_sent/EID_UNIT + switch_index)%total_switch_num;
      if(buffer[16]-'0'==2&&buffer[17]-'0'==2&&buffer[18]-'0'==2){
      NS_LOG_INFO("client Read "<<(uint32_t)m_sent<<" total "<<(uint32_t)m_count<<" EID "
                      <<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                      <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));
      }

      SendPacket(buffer,20,ssenh,m_switch_address,m_switch_port);
      Simulator::Schedule (Seconds(0.01), &MulticastClientApplicationv4::Read, this);
  }
}
void MulticastClientApplicationv4::ReceiveCallback (Ptr<Socket> socket){
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address localAddress;
  // NS_LOG_INFO("client receive call back");
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          // m_rxTrace (packet);
          // m_rxTraceWithAddresses (packet, from, m_local_address);
          SeanetHeader ssenh;
          packet->RemoveHeader (ssenh);
          uint32_t application_type = ssenh.GetApplicationType();
          uint32_t protocol_type = ssenh.GetProtocolType();

          switch (application_type){
            case RESOLUTION_APPLICATION:{
              uint8_t buffer[MAX_PAYLOAD_LEN];
              int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
              ResolutionProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            case SWITCH_APPLICATION:{
              uint8_t buffer[MAX_PAYLOAD_LEN];
              int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
              SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            case NEIGH_INFO_APPLICATION:{
              SeqTsSizeHeader stsh;
              packet->RemoveHeader(stsh);
              uint8_t buffer[MAX_PAYLOAD_LEN];
              int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
              NeigthInfoProtocolHandle(buffer,buffer_len,protocol_type,from,stsh);
              break;
            }
            case MULTICAST_APPLICATION:{
                uint8_t addr[20];
                from.CopyTo(addr);
                Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
                InetSocketAddress i4a = InetSocketAddress (ipv4, m_switch_port);

                m_local_address.CopyTo(addr);
                Ipv4Address m_ipv4 = Ipv4Address::Deserialize (addr);
                InetSocketAddress m_i4a = InetSocketAddress (m_ipv4, m_switch_port);

                NS_LOG_INFO("client "<<m_i4a.GetIpv4()<<" receive multicast reply from address"<<i4a.GetIpv4());
            }
            default:
            break;
          }
        }
    }
}
void MulticastClientApplicationv4::SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_DATA:
   
    MulticastClientApplicationv4::SwitchReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void MulticastClientApplicationv4::NeigthInfoProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
                            uint8_t protocoal_type, Address from,SeqTsSizeHeader stsh){
  uint8_t taddr[18];
  from.CopyTo(taddr);
  Ipv4Address ipv4=Ipv4Address::Deserialize (taddr);
  NS_LOG_INFO("client receive neigh result");
  switch (protocoal_type)
  {
  case NEIGH_INFO_RECEIVE:{
    uint16_t port= taddr[16] | (taddr[17] << 8);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_REPLY,IS_DST);
    SendPacket(buffer,buffer_len,ssenh,stsh,i4a,port);
    break;
  }
  case NEIGH_INFO_REPLY:{
    
    Time delay = Simulator::Now () - stsh.GetTs ();
    NS_LOG_INFO("client receive neigh result, delay:"<<delay.GetTimeStep());
    AddNeighDelay(ipv4,delay);
    break;
  }
  default:

    break;
  }
}

void MulticastClientApplicationv4::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_EID_NA:
    
    MulticastClientApplicationv4::ResolutionReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void MulticastClientApplicationv4::SwitchReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  uint8_t addr[20];
  from.CopyTo(addr);
  Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
  InetSocketAddress i4a = InetSocketAddress (ipv4, m_switch_port);
  NS_LOG_INFO("client receive switch reply: switch address"<<i4a.GetIpv4());
  se.Print();
  // NS_ASSERT (m_readEvent.IsExpired ());
  // m_readEvent = Simulator::Schedule (m_interval, &MulticastClientApplicationv4::Write, this);
}

void MulticastClientApplicationv4::ResolutionReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  uint32_t finished = buffer[EIDSIZE];
  uint32_t ipnum = buffer[EIDSIZE+1];
  
  // se.Print();
  // if(ipnum >= 1){
    Ipv4Address ipv4=Ipv4Address::Deserialize (buffer+EIDSIZE+2);
    uint16_t port= buffer[EIDSIZE+18] | (buffer[EIDSIZE+19] << 8);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    NS_LOG_INFO("MulticastClient receive resolution reply, ipnum "<<ipnum<<" finish "<<finished << " res address"<<i4a.GetIpv4());
    // SeanetHeader ssenh(SWITCH_APPLICATION,REQUEST_DATA,IS_DST);
    // uint8_t res[EIDSIZE];
    // se.getSeanetEID(res);
    // SendPacket(res,EIDSIZE,ssenh,i4a,m_switch_port);
  // }
}

void MulticastClientApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);

    m_switch_socket->SendTo(p,0,i4a);
}
void MulticastClientApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,SeqTsSizeHeader stsh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (stsh);
    p->AddHeader (ssenh);
    
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    m_switch_socket->SendTo(p,0,i4a);
}
void MulticastClientApplicationv4::NeighInfoDetec(Address dst_ip,uint16_t dst_port){
  SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_RECEIVE,IS_DST);
  uint8_t addr[18];
  dst_ip.CopyTo(addr);
  Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
  InetSocketAddress i4a = InetSocketAddress (ipv4, dst_port);
  NS_LOG_INFO("client send neigh detect "<< i4a.GetIpv4());
  SeqTsSizeHeader stsh;
  stsh.SetSeq(0);
  stsh.SetSize(10);
  uint8_t buffer[10];
  memset(buffer,'6',10);
  SendPacket(buffer,10,ssenh,stsh,dst_ip,dst_port);
}

Time MulticastClientApplicationv4::LookUpNeighDelay(Ipv4Address i4a){
  if (m_neigh_delay_table.find (i4a) != m_neigh_delay_table.end ())
    {
      Time entry = m_neigh_delay_table[i4a];
      // NS_LOG_LOGIC ("ResolutionApplicationv4 Found an entry: " << entry);
      return entry;
    }
  // NS_LOG_LOGIC ("ResolutionApplicationv4 Nothing found");
  Time entry(-1);
  return entry;
}

Time MulticastClientApplicationv4::AddNeighDelay(Ipv4Address i4a,Time t){
  Time entry = LookUpNeighDelay(i4a);
  if(entry.IsNegative()){
    m_neigh_delay_table[i4a] = t;
  }
  return t;
}
} // Namespace ns3
