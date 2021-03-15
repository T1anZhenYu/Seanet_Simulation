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
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "storage-client-application.h"
#include "seanet-header.h"
#include <cstdlib>
#include <cstdio>
#include "ns3/seanet-address.h"
#include "ns3/seanet-eid.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StorageClientApplication");

NS_OBJECT_ENSURE_REGISTERED (StorageClientApplication);

TypeId
StorageClientApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StorageClientApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<StorageClientApplication> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplication::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&StorageClientApplication::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("SwitchAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StorageClientApplication::m_switch_address),
                   MakeAddressChecker ())
    .AddAttribute ("SwitchPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplication::m_switch_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ResAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&StorageClientApplication::m_resolution_address),
                   MakeAddressChecker ())
    .AddAttribute ("ResPort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StorageClientApplication::m_resolution_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&StorageClientApplication::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
  ;
  return tid;
}

StorageClientApplication::StorageClientApplication ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_switch_socket = 0;
  m_sendEvent = EventId ();
  m_readEvent = EventId ();
}

StorageClientApplication::~StorageClientApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
StorageClientApplication::SetRemote (Address switch_ip,Address res_ip,uint16_t res_port, uint16_t switch_port)
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip << res_port <<switch_port);
  m_switch_address = switch_ip;
  m_switch_port = switch_port;
  m_resolution_address = res_ip;
  m_resolution_port = res_port;
}

void
StorageClientApplication::SetRemote (Address switch_ip,Address res_ip )
{
  NS_LOG_FUNCTION (this << switch_ip << res_ip);
  m_switch_address = switch_ip;
  m_resolution_address = res_ip;
}

void
StorageClientApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
StorageClientApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_switch_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_switch_socket = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), m_switch_port);
      if (m_switch_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_switch_socket->GetSockName (m_local_address);
    }

  m_switch_socket->SetRecvCallback (MakeCallback (&StorageClientApplication::ReceiveCallback, this));
  m_switch_socket->SetAllowBroadcast (true);
  // m_resolution_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  // m_resolution_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &StorageClientApplication::Write, this);
}

void
StorageClientApplication::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_readEvent);
}

void
StorageClientApplication::Write (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeanetHeader ssenh(SWITCH_APPLICATION,RESEIVE_DATA);
  
  uint8_t buffer[30];
  memcpy(buffer,"33333333333333333333",21);
  SendPacket(buffer,20,ssenh,m_switch_address,m_switch_port);
  // Ptr<uint8_t> buffer = Create<uint8_t> (10);
  // memcpy(buffer,"111111111",9);

  // delete buffer;
  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &StorageClientApplication::Read, this);
    }
}

void
StorageClientApplication::Read (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_readEvent.IsExpired ());
  SeanetHeader ssenh(RESOLUTION_APPLICATION ,REQUEST_DATA);
  
  uint8_t buffer[30];
  memcpy(buffer,"33333333333333333333",21);
  NS_LOG_INFO("Storage client read");
  SendPacket(buffer,20,ssenh,m_resolution_address,m_resolution_port);
  if (m_sent < m_count)
    {
      m_readEvent = Simulator::Schedule (m_interval, &StorageClientApplication::Write, this);
    }
}
void StorageClientApplication::ReceiveCallback (Ptr<Socket> socket){
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address localAddress;
  NS_LOG_INFO("client receive call back");
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
          uint8_t buffer[MAX_PAYLOAD_LEN];
          int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
          switch (application_type){
            case RESOLUTION_APPLICATION:{
              ResolutionProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            case SWITCH_APPLICATION:{
              SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            default:
            break;
          }
        }
    }
}
void StorageClientApplication::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_EID_NA:
    
    StorageClientApplication::ResolutionReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void StorageClientApplication::SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REPLY_DATA:
   
    StorageClientApplication::SwitchReplyHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}
void StorageClientApplication::SwitchReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
   NS_LOG_INFO("client receive switch reply: ");
  se.Print();
}

void StorageClientApplication::ResolutionReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  uint32_t ipnum = buffer[EIDSIZE];
  NS_LOG_INFO("client receive resolution reply, ipnum "<<ipnum);
  // se.Print();
  if(ipnum >= 1){
    Ipv6Address ipv6=Ipv6Address::Deserialize (buffer+EIDSIZE+1);
    uint16_t port= buffer[EIDSIZE+17] | (buffer[EIDSIZE+18] << 8);
    Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, port);
    SeanetHeader ssenh(SWITCH_APPLICATION,REQUEST_DATA);
    uint8_t res[EIDSIZE];
    se.getSeanetEID(res);
    SendPacket(res,EIDSIZE,ssenh,i6a,m_switch_port);
  }
}

void StorageClientApplication::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv6Address ipv6=Ipv6Address::Deserialize (addr);
    Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, port);
    // NS_LOG_INFO("client sendpacket to "<<i6a.GetIpv6());
    m_switch_socket->SendTo(p,0,i6a);
}


} // Namespace ns3
