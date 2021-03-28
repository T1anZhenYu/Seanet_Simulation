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
#include "seanet-header.h"
#include "resolution-application-v4.h"
#include "ns3/seanet-eid.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ResolutionApplicationv4");

NS_OBJECT_ENSURE_REGISTERED (ResolutionApplicationv4);

TypeId
ResolutionApplicationv4::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ResolutionApplicationv4")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<ResolutionApplicationv4> ()
          .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                         UintegerValue (100), MakeUintegerAccessor (&ResolutionApplicationv4::m_port),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketWindowSize",
                         "The size of the window used to compute the packet loss. This value "
                         "should be a multiple of 8.",
                         UintegerValue (32),
                         MakeUintegerAccessor (&ResolutionApplicationv4::GetPacketWindowSize,
                                               &ResolutionApplicationv4::SetPacketWindowSize),
                         MakeUintegerChecker<uint16_t> (8, 256))
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&ResolutionApplicationv4::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&ResolutionApplicationv4::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

ResolutionApplicationv4::ResolutionApplicationv4 () : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;
}

ResolutionApplicationv4::~ResolutionApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
ResolutionApplicationv4::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
ResolutionApplicationv4::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
ResolutionApplicationv4::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
ResolutionApplicationv4::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
ResolutionApplicationv4::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
ResolutionApplicationv4::StartApplication (void)
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

  m_socket->SetRecvCallback (MakeCallback (&ResolutionApplicationv4::FrontEnd, this));

  // packetin = CreateObject<DropTailQueue<Packet>> ();
  // addressin = CreateObject<DropTailQueue<SeanetAddress>> ();

  // AferEnd (localAddress);
}

void
ResolutionApplicationv4::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
  Simulator::Cancel(m_AfterEndEvent);
}

void
ResolutionApplicationv4::AferEnd (Address localAddress)
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
  
  m_AfterEndEvent = Simulator::Schedule (Seconds (0.01), &ResolutionApplicationv4::AferEnd, this, localAddress);
}
void
ResolutionApplicationv4::FrontEnd (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
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
}
ResolutionApplicationv4::LISTIP* ResolutionApplicationv4::LookupEIDNATable(SeanetEID se){
  if (m_eid_na_table.find (se) != m_eid_na_table.end ())
    {
      ResolutionApplicationv4::LISTIP* entry = m_eid_na_table[se];
      // NS_LOG_LOGIC ("ResolutionApplicationv4 Found an entry: " << entry);

      return entry;
    }
  // NS_LOG_LOGIC ("ResolutionApplicationv4 Nothing found");
  return NULL;
}
ResolutionApplicationv4::LISTIP* ResolutionApplicationv4::AddEIDNATable(SeanetEID se, Address to){
  LISTIP* lh = ResolutionApplicationv4::LookupEIDNATable(se);
  uint8_t* buf = new uint8_t[18];
  to.CopyTo(buf);
  Ipv4Address ipv4=Ipv4Address::Deserialize (buf);
  // Ipv4Address ipv4 = Ipv4Address::ConvertFrom(to);
  InetSocketAddress i4a = InetSocketAddress (ipv4, 4000);
  NS_LOG_INFO("resolution add info "<<i4a.GetIpv4());
  if(lh==NULL){
    lh = new ResolutionApplicationv4::LISTIP;

    lh->push_back(buf);
    m_eid_na_table[se] = lh;
  }else{
    // uint8_t* buf = new uint8_t[18];
    // to.CopyTo(buf);
    lh->push_back(buf);
  }
  return lh;
}

void ResolutionApplicationv4::ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,uint8_t protocoal_type, Address from){
  switch (protocoal_type)
  {
  case REQUEST_EID_NA:
    ResolutionApplicationv4::RequestHandle(buffer,buffer_len, from);
    break;
  case REGIST_EID_NA:
    // NS_LOG_INFO("Resolution regist");
    ResolutionApplicationv4::RegistHandle(buffer,buffer_len, from);
    break;
  default:
    break;
  }
}

void ResolutionApplicationv4::RequestHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  
  ResolutionApplicationv4::LISTIP * value = ResolutionApplicationv4::LookupEIDNATable(se);
  
  if(value!=NULL){
    
    uint8_t res[MAX_PAYLOAD_LEN];
    se.getSeanetEID(res);
    int ipnum = 0;
    for (ResolutionApplicationv4::LISTIP::iterator it = value->begin(); it != value->end(); ++it) {

      uint8_t* buf = *it;
      std::list<uint8_t*>::iterator itnext = it;
      itnext++;
      memcpy(res+EIDSIZE+2+18*(ipnum%10),buf,18);
      ipnum++;
      if(ipnum%10==0 && itnext!=value->end()){
        res[EIDSIZE]=PACKET_NOT_FINISH;//
        res[EIDSIZE+1] = 10;
        SeanetHeader ssenh(RESOLUTION_APPLICATION,REPLY_EID_NA);
        ResolutionApplicationv4::SendPacket(res,MAX_PAYLOAD_LEN,ssenh,from); 
        NS_LOG_INFO("1:Resolution reply request "<<ipnum<<" "<<buffer[15]<<buffer[16]<<buffer[17]<<buffer[18]
      <<buffer[19]);
        memset(res+EIDSIZE,0,MAX_PAYLOAD_LEN-EIDSIZE);    
      }else if(ipnum%10==0 && itnext==value->end()){
        res[EIDSIZE]=PACKET_FINISH;//
        res[EIDSIZE+1] = 10;
        SeanetHeader ssenh(RESOLUTION_APPLICATION,REPLY_EID_NA);
        ResolutionApplicationv4::SendPacket(res,MAX_PAYLOAD_LEN,ssenh,from);  
        NS_LOG_INFO("2:Resolution reply request "<<ipnum<<" "<<buffer[15]<<buffer[16]<<buffer[17]<<buffer[18]
      <<buffer[19]);
        memset(res+EIDSIZE,0,MAX_PAYLOAD_LEN-EIDSIZE);    
      }
    }
    if(ipnum%10!=0){
      res[EIDSIZE]=PACKET_FINISH;//
      res[EIDSIZE+1] = ipnum%10;
      // memcpy(res+EIDSIZE,value,EID_NA_TABLE_VALUE_SIZE);
      NS_LOG_INFO("3:Resolution reply request "<<ipnum<<" "<<buffer[15]<<buffer[16]<<buffer[17]<<buffer[18]
      <<buffer[19]);
      SeanetHeader ssenh(RESOLUTION_APPLICATION,REPLY_EID_NA);
      ResolutionApplicationv4::SendPacket(res,MAX_PAYLOAD_LEN,ssenh,from);
    }

  }
}

void ResolutionApplicationv4::RegistHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  // NS_LOG_INFO("Resolution info regist");
  SeanetEID se(buffer);
  ResolutionApplicationv4::AddEIDNATable(se,from);
}

void ResolutionApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (ssenh);
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
    
    m_socket->SendTo(p,0,i4a);
}
} // Namespace ns3
