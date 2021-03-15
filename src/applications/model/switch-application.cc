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

#include "switch-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwitchApplication");

NS_OBJECT_ENSURE_REGISTERED (SwitchApplication);

TypeId
SwitchApplication::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SwitchApplication")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<SwitchApplication> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&SwitchApplication::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&SwitchApplication::GetPacketWindowSize,
                                               &SwitchApplication::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddAttribute ("Resolutionaddr",
                        "The destination Address of the outbound packets",
                        AddressValue (),
                        MakeAddressAccessor (&SwitchApplication::resolution_addr),
                        MakeAddressChecker ())
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&SwitchApplication::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&SwitchApplication::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

SwitchApplication::SwitchApplication () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;

}

SwitchApplication::~SwitchApplication ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
SwitchApplication::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
SwitchApplication::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
SwitchApplication::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
SwitchApplication::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
SwitchApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SwitchApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
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

  m_socket->SetRecvCallback (MakeCallback (&SwitchApplication::FrontEnd, this));

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

  m_socket6->SetRecvCallback (MakeCallback (&SwitchApplication::FrontEnd, this));
  packetin = CreateObject<DropTailQueue<Packet>> ();
  addressin = CreateObject<DropTailQueue<SeanetAddress>> ();

  AferEnd ();
}

void
SwitchApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
  Simulator::Cancel(m_AfterEndEvent);
}

void
SwitchApplication::AferEnd ()
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
          sa->toAddress(from);
          SeanetHeader ssenh;
          packet->RemoveHeader (ssenh);
          // sa->toAddress(from);
          uint32_t application_type = ssenh.GetApplicationType();
          uint32_t protocol_type = ssenh.GetProtocolType();
          uint8_t buffer[MAX_PAYLOAD_LEN];
          int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
          // SeanetEID se(buffer);
          // SwitchApplication::AddEIDNATable(se,1);
          switch (application_type){
            case RESOLUTION_APPLICATION:{
              NS_LOG_INFO("Switch can't process this application type");
              break;
            }
            case SWITCH_APPLICATION:{
              SwitchApplication::SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            default:
            break;
          }
          // if (InetSocketAddress::IsMatchingType (from))
          //   {
          //     NS_LOG_INFO ("Switch TraceDelay: RX " << packet->GetSize () <<
          //                   " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
          //                   " application_type: " << application_type <<
          //                   " protocol_type: " << protocol_type <<
          //                   " EID:" <<buffer[0]<<buffer[1]<<buffer[2]<<buffer[3]<<buffer[4]<<
          //                   " Uid: " << packet->GetUid () <<
          //                   " RXtime: " << Simulator::Now ());
          //   }
          // else if (Inet6SocketAddress::IsMatchingType (from))
          //   {
          //     NS_LOG_INFO ("Switch TraceDelay: RX " << packet->GetSize () <<
          //                   " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
          //                   " local address "<< Inet6SocketAddress::ConvertFrom (localAddress).GetIpv6 () <<
          //                   " application_type: " << application_type <<
          //                   " protocol_type: " << protocol_type <<
          //                   " EID:" <<buffer[0]<<buffer[1]<<buffer[2]<<buffer[3]<<buffer[4]<<
          //                   " Uid: " << packet->GetUid () <<
          //                   " RXtime: " << Simulator::Now ());
          //   }

          // m_lossCounter.NotifyReceived (application_type);
          m_received++;
          // NS_LOG_INFO ("received packet " << m_received);
        }
    }
  
  m_AfterEndEvent = Simulator::Schedule (Seconds (0.1), &SwitchApplication::AferEnd, this);
}
void
SwitchApplication::FrontEnd (Ptr<Socket> socket)
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
          Ptr<SeanetAddress> sa = CreateObject<SeanetAddress>();
          // SeanetAddress *sa_ = new SeanetAddress();
          packetin->Enqueue (packet);
          sa->fromAddress(from);
          
          uint8_t buffer[100];
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

uint8_t SwitchApplication::LookupEIDNATable(SeanetEID se){
  if (m_eid_table.find (se) != m_eid_table.end ())
    {
      uint8_t entry = m_eid_table[se];
      // NS_LOG_INFO ("SwitchApplication Found an entry: " << entry);

      return entry;
    }
  // NS_LOG_INFO ("SwitchApplication Nothing found");
  return 0;
}
uint8_t SwitchApplication::AddEIDNATable(SeanetEID se, uint8_t value){
  uint8_t v = SwitchApplication::LookupEIDNATable(se);
  if(v == 0){
    m_eid_table[se] = value;
    return value;
  }
  return 0;
}
void SwitchApplication::SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len,uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REQUEST_DATA:
    SwitchApplication::RequestDataHandle(buffer,buffer_len, from);
    break;
  case RESEIVE_DATA:
    SwitchApplication::ReceiveDataHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}

void SwitchApplication::RequestDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  // NS_LOG_INFO("Switch node get a data request");
  if(SwitchApplication::LookupEIDNATable(se)!=0){
    SeanetHeader ssenh(SWITCH_APPLICATION,REPLY_DATA);
    SwitchApplication::SendPacket(buffer,EIDSIZE,ssenh,from);
  }
}

void SwitchApplication::ReceiveDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  // NS_LOG_INFO("Switch node get a data packet");
  SwitchApplication::AddEIDNATable(se,1);
  // NS_LOG_INFO("ReceiveDataHandle");
  SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
  SwitchApplication::SendPacket(buffer,EIDSIZE,ssenh,resolution_addr);
}

void SwitchApplication::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv6Address ipv6=Ipv6Address::Deserialize (addr);
    Inet6SocketAddress i6a = Inet6SocketAddress (ipv6, m_port);
    // NS_LOG_INFO("switch sendpacket to "<<i6a.GetIpv6());
    m_socket6->SendTo(p,0,i6a);
}
} // Namespace ns3
