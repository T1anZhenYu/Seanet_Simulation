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
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->GetSockName (localAddress);
    }

  m_socket->SetRecvCallback (MakeCallback (&SwitchApplicationv4::FrontEnd, this));
  packetin = CreateObject<DropTailQueue<Packet>> ();
  addressin = CreateObject<DropTailQueue<SeanetAddress>> ();
  m_AfterEndEvent = Simulator::Schedule (Seconds (0), &SwitchApplicationv4::AferEnd, this);
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

void
SwitchApplicationv4::AferEnd ()
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
          uint32_t is_dst = ssenh.Getdst();
          if(is_dst == NOT_DST){
            NS_LOG_INFO("Swith reiceive a packet not belong here");
            m_AfterEndEvent = Simulator::Schedule (Seconds (0.001), &SwitchApplicationv4::AferEnd, this);
            return ;
          }
          // SeanetEID se(buffer);
          // SwitchApplicationv4::AddEIDNATable(se,1);
          switch (application_type){
            case RESOLUTION_APPLICATION:{
              NS_LOG_INFO("Switch can't process this application type");
              break;
            }
            case SWITCH_APPLICATION:{
              uint8_t buffer[MAX_PAYLOAD_LEN];
              int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
              SwitchApplicationv4::SwitchProtocolHandle(buffer,buffer_len,protocol_type,from);
              break;
            }
            case NEIGH_INFO_APPLICATION:{
              NS_LOG_INFO("switch recieve neigh detect");
              SeqTsSizeHeader stsh;
              packet->RemoveHeader(stsh);
              uint8_t buffer[MAX_PAYLOAD_LEN];
              int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
              NeigthInfoProtocolHandle(buffer,buffer_len,protocol_type,from,stsh);
              break;
            }
            default:
            NS_LOG_INFO("Switch can't process this application type "<< application_type);
            break;
          }
          m_received++;
          // NS_LOG_INFO ("received packet " << m_received);
        }
    }
  
  m_AfterEndEvent = Simulator::Schedule (Seconds (0.001), &SwitchApplicationv4::AferEnd, this);

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
          Ptr<SeanetAddress> sa = CreateObject<SeanetAddress>();
          // SeanetAddress *sa_ = new SeanetAddress();

          packetin->Enqueue (packet);
          sa->fromAddress(from);
          
          uint8_t buffer[MAX_PAYLOAD_LEN];
          from.CopyAllTo(buffer,MAX_PAYLOAD_LEN);

          bool flag = addressin->Enqueue(sa);
          if(flag == false){
              NS_LOG_INFO("EnqueueFailed "<<flag);
          }
          
        }
    }
}

uint8_t SwitchApplicationv4::LookupEIDNATable(SeanetEID se){
  if (m_eid_table.find (se) != m_eid_table.end ())
    {
      uint8_t entry = m_eid_table[se];
      // NS_LOG_INFO ("SwitchApplicationv4 Found an entry: " << entry);

      return entry;
    }
  // NS_LOG_INFO ("SwitchApplicationv4 Nothing found");
  return 0;
}
uint8_t SwitchApplicationv4::AddEIDNATable(SeanetEID se, uint8_t value){
  uint8_t v = SwitchApplicationv4::LookupEIDNATable(se);
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
  if(SwitchApplicationv4::LookupEIDNATable(se)!=0){
    SeanetHeader ssenh(SWITCH_APPLICATION,REPLY_DATA);
    SwitchApplicationv4::SendPacket(buffer,EIDSIZE,ssenh,from);
  }
}

void SwitchApplicationv4::ReceiveDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from){
  SeanetEID se(buffer);
  // NS_LOG_INFO("Switch node get a data packet");
  SwitchApplicationv4::AddEIDNATable(se,1);
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
    m_socket->SendTo(p,0,i4a);
}
void SwitchApplicationv4::SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,SeqTsSizeHeader stsh,Address to, uint16_t port){
    Ptr<Packet> p = Create<Packet> (buffer,buffer_len); 
    p->AddHeader (stsh);
    p->AddHeader (ssenh);
    
    uint8_t addr[18];
    to.CopyTo(addr);
    Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
    InetSocketAddress i4a = InetSocketAddress (ipv4, port);
    m_socket->SendTo(p,0,i4a);
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
      m_RandomCacheEvent = Simulator::Schedule (Seconds (0.1), &SwitchApplicationv4::RandomCache, this,m_temp_cache_size);
    }
  }
void SwitchApplicationv4::NeigthInfoProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
                            uint8_t protocoal_type, Address from,SeqTsSizeHeader stsh){
    switch (protocoal_type)
    {
    case NEIGH_INFO_RECEIVE:{
      NS_LOG_INFO("switch recieve neigh detect");
      uint8_t addr[18];
      from.CopyTo(addr);
      Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
      // uint16_t port= addr[16] | (addr[17] << 8);
      InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
      SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_REPLY);
      SendPacket(buffer,buffer_len,ssenh,stsh,i4a,m_port);
      break;
    }
    default:
      break;
    }
  }
} // Namespace ns3
