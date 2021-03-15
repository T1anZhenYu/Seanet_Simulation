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
#include "storage-client-application-v4.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StorageClientApplicationv4");

NS_OBJECT_ENSURE_REGISTERED (StorageClientApplicationv4);

TypeId
StorageClientApplicationv4::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StorageClientApplicationv4")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<StorageClientApplicationv4> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplicationv4::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&StorageClientApplicationv4::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("SwitchAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StorageClientApplicationv4::m_switch_address),
                   MakeAddressChecker ())
    .AddAttribute ("SwitchPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplicationv4::m_switch_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ResAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StorageClientApplicationv4::m_resolution_address),
                   MakeAddressChecker ())
    .AddAttribute ("ResPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplicationv4::m_resolution_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&StorageClientApplicationv4::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
  ;
  return tid;
}

StorageClientApplicationv4::StorageClientApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_switch_socket = 0;
  m_sendEvent = EventId ();
  m_readEvent = EventId ();
}

StorageClientApplicationv4::~StorageClientApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
}

void
StorageClientApplicationv4::SetRemote (Address switch_ip,Address res_ip,uint16_t res_port, uint16_t switch_port)
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip << res_port <<switch_port);
  m_switch_address = switch_ip;
  m_switch_port = switch_port;
  m_resolution_address = res_ip;
  m_resolution_port = res_port;
}

void
StorageClientApplicationv4::SetRemote (Address switch_ip,Address res_ip )
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip);
  m_switch_address = switch_ip;
  m_resolution_address = res_ip;
}

void
StorageClientApplicationv4::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
StorageClientApplicationv4::StartApplication (void)
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

  m_switch_socket->SetRecvCallback (MakeCallback (&StorageClientApplicationv4::ReceiveCallback, this));
  m_switch_socket->SetAllowBroadcast (true);
  // m_resolution_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  // m_resolution_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &StorageClientApplicationv4::Write, this);

  NeighInfoDetec(m_switch_address,m_switch_port);
}

void
StorageClientApplicationv4::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_readEvent);
}

void
StorageClientApplicationv4::Write (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeanetHeader ssenh(SWITCH_APPLICATION,RESEIVE_DATA,IS_DST);
  
  uint8_t buffer[30];
  memcpy(buffer,"33333333333333333333",21);
  // NS_LOG_INFO("Storage client write");
  SendPacket(buffer,20,ssenh,m_switch_address,m_switch_port);


  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &StorageClientApplicationv4::Read, this);
      m_sent++;
    }
}

void
StorageClientApplicationv4::Read (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_readEvent.IsExpired ());
  SeanetHeader ssenh(RESOLUTION_APPLICATION ,REQUEST_DATA,IS_DST);
  
  uint8_t buffer[30];
  memcpy(buffer,"33333333333333333333",21);
  // NS_LOG_INFO("Storage client read");
  SendPacket(buffer,20,ssenh,m_resolution_address,m_resolution_port);
  if (m_sent < m_count)
    {
      m_readEvent = Simulator::Schedule (m_interval, &StorageClientApplicationv4::Write, this);
    }
}
void StorageClientApplicationv4::ReceiveCallback (Ptr<Socket> socket){
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
            default:
            break;
          }
        }
    }
}
void StorageClientApplicationv4::SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_DATA:
   
    StorageClientApplicationv4::SwitchReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void StorageClientApplicationv4::NeigthInfoProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
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

void StorageClientApplicationv4::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_EID_NA:
    
    StorageClientApplicationv4::ResolutionReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void StorageClientApplicationv4::SwitchReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  uint8_t addr[20];
  from.CopyTo(addr);
  Ipv4Address ipv4 = Ipv4Address::Deserialize (addr);
  InetSocketAddress i4a = InetSocketAddress (ipv4, m_switch_port);
  NS_LOG_INFO("client receive switch reply: switch address"<<i4a.GetIpv4());
  se.Print();
  // NS_ASSERT (m_readEvent.IsExpired ());
  // m_readEvent = Simulator::Schedule (m_interval, &StorageClientApplicationv4::Write, this);
}

void StorageClientApplicationv4::ResolutionReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  uint32_t ipnum = buffer[EIDSIZE];
  
  // se.Print();
  if(ipnum >= 1){
    Ipv4Address ipv4=Ipv4Address::Deserialize (buffer+EIDSIZE+1);
    uint16_t port= buffer[EIDSIZE+17] | (buffer[EIDSIZE+18] << 8);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    // NS_LOG_INFO("client receive resolution reply, ipnum "<<ipnum << " res address"<<i4a.GetIpv4());
    SeanetHeader ssenh(SWITCH_APPLICATION,REQUEST_DATA,IS_DST);
    uint8_t res[EIDSIZE];
    se.getSeanetEID(res);
    SendPacket(res,EIDSIZE,ssenh,i4a,m_switch_port);
  }
}

void StorageClientApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);

    m_switch_socket->SendTo(p,0,i4a);
}
void StorageClientApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,SeqTsSizeHeader stsh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (stsh);
    p->AddHeader (ssenh);
    
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    m_switch_socket->SendTo(p,0,i4a);
}
void StorageClientApplicationv4::NeighInfoDetec(Address dst_ip,uint16_t dst_port){
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

Time StorageClientApplicationv4::LookUpNeighDelay(Ipv4Address i4a){
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

Time StorageClientApplicationv4::AddNeighDelay(Ipv4Address i4a,Time t){
  Time entry = LookUpNeighDelay(i4a);
  if(entry.IsNegative()){
    m_neigh_delay_table[i4a] = t;
  }
  return t;
}
} // Namespace ns3
