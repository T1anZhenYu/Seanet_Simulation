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
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "packet-loss-counter.h"
#include "ns3/drop-tail-queue.h"
#include "seanet-header.h"
#include "storage-resolution-application.h"
#include "ns3/seanet-eid.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ResolutionApplication");

NS_OBJECT_ENSURE_REGISTERED (ResolutionApplication);

TypeId
ResolutionApplication::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ResolutionApplication")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<ResolutionApplication> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&ResolutionApplication::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&ResolutionApplication::GetPacketWindowSize,
                                               &ResolutionApplication::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&ResolutionApplication::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&ResolutionApplication::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

ResolutionApplication::ResolutionApplication () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;
}

ResolutionApplication::~ResolutionApplication ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
ResolutionApplication::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
ResolutionApplication::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
ResolutionApplication::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
ResolutionApplication::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
ResolutionApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
ResolutionApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  Address localAddress;
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->GetSockName (localAddress);
    }

  m_socket->SetRecvCallback (MakeCallback (&ResolutionApplication::FrontEnd, this));

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
      if (m_socket6->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket6->GetSockName (localAddress);
    }

  m_socket6->SetRecvCallback (MakeCallback (&ResolutionApplication::FrontEnd, this));
  packetin = CreateObject<DropTailQueue<Packet>> ();
  addressin = CreateObject<DropTailQueue<SeanetAddress>> ();

  AferEnd (localAddress);
}

void
ResolutionApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
  Simulator::Cancel(m_AfterEndEvent);
}

void
ResolutionApplication::AferEnd (Address localAddress)
{
  // Ptr<Address> pfrom = addressin->Dequeue();
  Address from;
  Ptr<Packet> packet = packetin->Dequeue ();
  Ptr<SeanetAddress> sa = addressin->Dequeue();
  if (packet != NULL && sa != NULL){
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (packet->GetSize () > 0)
        {
          // printf("sa is not empty m_type %d, m_len %d ,m_buffer %d %d %d %d %d %d %d\n",sa->m_type,
          //         sa->m_len, sa->m_data[0],sa->m_data[1],sa->m_data[2],sa->m_data[3],sa->m_data[4],
          //         sa->m_data[5],sa->m_data[6]);
          sa->toAddress(from);
          SeanetHeader ssenh;
          packet->RemoveHeader (ssenh);
          // sa->toAddress(from);
          uint32_t application_type = ssenh.GetApplicationType();
          uint32_t protocol_type = ssenh.GetProtocolType();
          uint8_t buffer[MAX_PAYLOAD_LEN];
          int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
          switch (application_type){
            case RESOLUTION_APPLICATION:{
              ResolutionProtocolHandle(buffer,buffer_len,protocol_type,from);
              // NS_LOG_INFO("Resolution receive a packet");
              break;
            }
            case SWITCH_APPLICATION:{
              NS_LOG_INFO("Resolution can't process this application type");
              // SwitchApplication::SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            default:
            break;
          }
        }
    }
  
  m_AfterEndEvent = Simulator::Schedule (Seconds (0.1), &ResolutionApplication::AferEnd, this, localAddress);
}
void
ResolutionApplication::FrontEnd (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          Ptr<SeanetAddress> sa = CreateObject<SeanetAddress>();
          // SeanetAddress *sa_ = new SeanetAddress();
          packetin->Enqueue (packet);
          sa->fromAddress(from);
          
          uint8_t buffer[1000];
          from.CopyAllTo(buffer,100);

          // printf("m_type %d, m_len %d ,m_buffer %d %d %d %d %d %d %d, total len %d\n",sa->m_type,
          //    sa->m_len, sa->m_data[0],sa->m_data[1],sa->m_data[2],sa->m_data[3],sa->m_data[4],
          //    sa->m_data[5],sa->m_data[6],len);
          // NS_LOG_INFO("len "<< len<<" mtype "<<sa->m_type<<" mlen " << sa->m_len<< " buffer "<<buffer+2);
          bool flag = addressin->Enqueue(sa);
          if(flag == false){
              NS_LOG_INFO("EnqueueFailed "<<flag);
          }
          
        }
    }
}
uint8_t* ResolutionApplication::LookupEIDNATable(SeanetEID se){
  if (m_eid_na_table.find (se) != m_eid_na_table.end ())
    {
      uint8_t* entry = m_eid_na_table[se];
      // NS_LOG_LOGIC ("ResolutionApplication Found an entry: " << entry);

      return entry;
    }
  // NS_LOG_LOGIC ("ResolutionApplication Nothing found");
  return NULL;
}
uint8_t* ResolutionApplication::AddEIDNATable(SeanetEID se, Address to){
  uint8_t * buf = ResolutionApplication::LookupEIDNATable(se);
  if(buf == NULL){
    buf = new uint8_t[EID_NA_TABLE_VALUE_SIZE];
    memset(buf,0,EID_NA_TABLE_VALUE_SIZE);
    to.CopyTo(buf+1);
    buf[0] = 1;
    // Ipv6Address ipv6=Ipv6Address::Deserialize (buf+1);
    // uint16_t port= buf[17] | (buf[18] << 8);
    // Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, port);
    // NS_LOG_INFO("buf is null , ipv6 address : "<<i6a.GetIpv6());
    m_eid_na_table[se] = buf;
  }else{
 
    uint8_t ipaddress[18];
    to.CopyTo(ipaddress);
    uint8_t address_num = buf[0];
    if(address_num<3){
      memcpy(buf+1+18*address_num,ipaddress,18);
      address_num++;
      buf[0] = address_num;
    }
    m_eid_na_table[se] = buf;
  }
  return buf;
}

void ResolutionApplication::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REQUEST_EID_NA:
    ResolutionApplication::RequestHandle(buffer,buffer_len, from);
    break;
  case REGIST_EID_NA:
    // NS_LOG_INFO("Resolution regist");
    ResolutionApplication::RegistHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}

void ResolutionApplication::RequestHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  
  uint8_t * value = ResolutionApplication::LookupEIDNATable(se);
  
  if(value!=NULL){
    NS_LOG_INFO("Resolution info request");
    uint8_t res[MAX_PAYLOAD_LEN];
    se.getSeanetEID(res);
    memcpy(res+EIDSIZE,value,EID_NA_TABLE_VALUE_SIZE);
    SeanetHeader ssenh(RESOLUTION_APPLICATION,REPLY_EID_NA);
    ResolutionApplication::SendPacket(res,MAX_PAYLOAD_LEN,ssenh,from);
  }
}

void ResolutionApplication::RegistHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  NS_LOG_INFO("Resolution info regist");
  ResolutionApplication::AddEIDNATable(se,from);
  // Ipv6Address ipv6=Ipv6Address::Deserialize (res+1);
  // uint16_t port= res[17] | (res[18] << 8);
  // Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, port);
  // uint32_t ipnum = res[0];
  // NS_LOG_INFO("Resolution regist table value : ip num "<<ipnum<<" ip addr"<<i6a.GetIpv6());
}

void ResolutionApplication::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv6Address ipv6=Ipv6Address::Deserialize (addr);
    Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, m_port);
    // NS_LOG_INFO("resolution sendpacket to "<<i6a.GetIpv6());
    m_socket6->SendTo(p,0,i6a);
}
} // Namespace ns3
