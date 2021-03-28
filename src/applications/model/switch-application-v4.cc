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
#include <ns3/string.h>
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
                           MakeTraceSourceAccessor (&SwitchApplicationv4::rm_rx_trace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&SwitchApplicationv4::m_rx_trace_with_addresses),
                           "ns3::Packet::TwoAddressTracedCallback")
          .AddAttribute ("TreeType","SPT, RPT, Seanet",
                          StringValue ("Seanet"),
                          MakeStringAccessor (&SwitchApplicationv4::tree_type),
                          MakeStringChecker ());
                          
  return tid;
}

SwitchApplicationv4::SwitchApplicationv4 () : m_loss_counter (0)
{
  NS_LOG_FUNCTION (this);
  m_received = 0;

}
void SwitchApplicationv4::SetNeighInfoTable
  (sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash>* sct,
  sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash>* mct,
  sgi::hash_map<SeanetEID, std::list<Ipv4Address*>*, SeanetEIDHash>*ent){
    unicast_table = sct;
    // NS_LOG_INFO("Initialize HASHTABLE "<<unicast_table.size()<<" "<<unicast_table.begin()->second);
    multicast_table = mct;
    resolution_table = ent;
}
SwitchApplicationv4::~SwitchApplicationv4 ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
SwitchApplicationv4::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_loss_counter.GetBitMapSize ();
}

void
SwitchApplicationv4::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_loss_counter.SetBitMapSize (size);
}

uint32_t
SwitchApplicationv4::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_loss_counter.GetLost ();
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
      // Address mlocal_address = mydevice->GetAddress();
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);

      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      // m_socket->GetSockName (local_address);
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
        (*multicast_table)[addri]=NULL;
        (*unicast_table)[addri]=NULL;
        local_address = Address(addri);
        // InetSocketAddress mlocal = InetSocketAddress (addri, m_port);
        // NS_LOG_INFO("switch local address "<<mlocal.GetIpv4());
        break;
      }

    }

  m_socket->SetRecvCallback (MakeCallback (&SwitchApplicationv4::FrontEnd, this));
  packetin = CreateObject<DropTailQueue<Packet>> ();
  addressin = CreateObject<DropTailQueue<SeanetAddress>> ();
  if(is_entry_switch && tree_type != "SPT"){
    Simulator::Schedule (Seconds (0), &SwitchApplicationv4::DetecAllNeighborDelay, this);
    // NS_LOG_INFO("after detect all neight");
  }
  have_detected = false;
  m_temp_cache_size = 0;
}
void SwitchApplicationv4::SetEntrySwitch(bool isEntry){
  is_entry_switch = isEntry;
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
float SwitchApplicationv4::GetSwitchSocre(Ipv4Address ad){
  std::list<uint8_t*>*UeidL,*MeidL;
  int UeidLNum = 0;
  int MeidLNum = 0;
  // double delay = 0;
  if(multicast_table->find(ad)!= multicast_table->end() && 
      unicast_table->find(ad)!= unicast_table->end()&&
       multicast_table->find(ad)->second!=NULL && 
       unicast_table->find(ad)->second != NULL){
    UeidL = unicast_table->find(ad)->second;
    MeidL = multicast_table->find(ad)->second;
    std::list<uint8_t*>::iterator temp = UeidL->begin();
    
    while(temp!=UeidL->end()){
      UeidLNum++;
      temp++;
    }
    temp = MeidL->begin();
    while(temp != MeidL->end()){
      MeidLNum++;
      temp++;
    }
    //  NS_LOG_INFO("In GetSwitchSocre "<<0.5*UeidLNum + 0.5*MeidLNum);
  }
  // if(delay_table.find(ad)!=delay_table.end()){
  //   delay = delay_table.find(ad)->second.GetDouble();
  //   // NS_LOG_INFO("In GetSwitchSocre "<<UeidLNum + MeidLNum+delay/4000000<<" address "<<InetSocketAddress(ad,m_port).GetIpv4());
  // }else{
  //   delay = 4000000;
  // }
  // NS_LOG_INFO("In GetSwitchSocre "<<5000*UeidLNum + 5000*MeidLNum+delay);
  return UeidLNum + MeidLNum;
}
Ipv4Address SwitchApplicationv4::GetRootNode(){
  sgi::hash_map<Ipv4Address, Time, Ipv4AddressHash>::iterator it = delay_table.begin();
  Ipv4Address selected_address;
  if(it!=delay_table.end()){
    selected_address=it->first;
  }else{
    return Ipv4Address::ConvertFrom(local_address);
  }
  double selected_score = 10000;
  double temp_score = 0;
  
  for(;it!=delay_table.end();it++){
    temp_score = GetSwitchSocre(it->first);
    if(temp_score<=selected_score){
      
      selected_address = it->first;
      selected_score=temp_score;
      // NS_LOG_INFO("temp score "<<temp_score<<" address "<<InetSocketAddress(selected_address).GetIpv4());
    }
  }

  return selected_address;
}
//如果有重复，返回true
bool SwitchApplicationv4::CheckDuplicate(std::list<uint8_t*>*head, uint8_t* buf){
  for(std::list<uint8_t*>::iterator it = head->begin(); it!=head->end();it++){
    if(memcmp(*it,buf,EIDSIZE)==0){
      return true;
    }
  }
  return false;
}
void
SwitchApplicationv4::FrontEnd (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  
  // Address local_address;
  while ((packet = socket->RecvFrom (from)))
    {
      resolution_addr.m_type = from.m_type;
      if (packet->GetSize () > 0)
        {
          // NS_LOG_INFO("front end get a packet");
          rm_rx_trace (packet);
          m_rx_trace_with_addresses (packet, from, local_address);
          if (packet->GetSize () > 0)
            {
              SeanetHeader ssenh;
              packet->RemoveHeader (ssenh);
              uint32_t application_type = ssenh.GetApplicationType();
              uint32_t protocol_type = ssenh.GetProtocolType();
              uint32_t is_dst = ssenh.Getdst();
              uint32_t interface_num = ssenh.GetInterface();
              //local address 
              Ipv4Address i4a = Ipv4Address::ConvertFrom(local_address);
              i4a.SetInterfaceNum(interface_num);
              InetSocketAddress mlocal = InetSocketAddress (i4a, m_port);
              //freom address
              uint8_t fromaddr[18];
              from.CopyTo(fromaddr);
              Ipv4Address fromipv4=Ipv4Address::Deserialize (fromaddr);
              InetSocketAddress fromipv4isa = InetSocketAddress (fromipv4, m_port);
              uint8_t buffer[EIDSIZE];
              uint32_t buffer_len = packet->CopyData(buffer, EIDSIZE);
              if(is_dst == NOT_DST){
                switch (application_type)
                {
                case MULTICAST_APPLICATION:{
                  if(protocol_type == REGIST_TO_RN){
                    uint8_t* buf = new uint8_t[EIDSIZE];
                    packet->CopyData(buf,EIDSIZE);
                    AddCastTable(unicast_table,i4a,buf);    
                    i4a.SetInterfaceNum(0);
                    AddCastTable(unicast_table,i4a,buf); 
                  }else if(protocol_type == GRAFITING_REQUEST){//收端DR向嫁接节点发送嫁接信令，沿路节点加入组播树，向解析注册；同时回复时延探测  
                   
                      uint8_t* buf = new uint8_t[EIDSIZE];
                      packet->CopyData(buf,EIDSIZE);
                      AddCastTable(multicast_table,i4a,buf);    
                      i4a.SetInterfaceNum(0);
                      AddCastTable(multicast_table,i4a,buf);
                      SeanetEID se(buffer);
                      AddEIDNAINFO(se,i4a);
                      NeighInfoReply(from,m_port);
                        
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
                    NS_LOG_INFO("Switch can't process RESOLUTION_APPLICATION type");
                    // uint8_t buffer[MAX_PAYLOAD_LEN];
                    // int buffer_len = packet->CopyData(buffer,MAX_PAYLOAD_LEN);
                    // SwitchApplicationv4::ResolutionProtocolHandle(buffer,buffer_len,protocol_type,from);
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
                      Ipv4Address rni4a;
                      if(tree_type == "Seanet"){
                        rni4a= SwitchApplicationv4::GetRootNode();
                      }else if(tree_type == "SPT"){
                        rni4a = i4a;// SPT树的RN节点是自己
                      }
                      if(rni4a == i4a){//如果找到RN是自己，直接进行注册就可以
                        NS_LOG_INFO("6.3:Source DR "<<mlocal.GetIpv4()<<" find RN is Self "<<" EID "
                      <<(uint32_t)(buffer[15]-'0')<<" "<<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                      <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));
                        uint8_t* buf = new uint8_t[EIDSIZE];
                        packet->CopyData(buf,EIDSIZE);        
                        AddCastTable(multicast_table,i4a,buf);
                        AddCastTable(unicast_table,i4a,buf);                    
                        i4a.SetInterfaceNum(0);
                        AddCastTable(multicast_table,i4a,buf);
                        AddCastTable(unicast_table,i4a,buf);                      
                        SeanetEID se(buffer);
                        AddEIDNAINFO(se,i4a);     
                      }else{//找的RN不是自己
                        InetSocketAddress rni4aisa = InetSocketAddress (rni4a, m_port);
                        SeanetHeader ssenh(MULTICAST_APPLICATION,REGIST_TO_RN);
                        SendPacket(buffer,buffer_len,ssenh,Address(rni4a));
                        NS_LOG_LOGIC("6.5:Source DR "<<mlocal.GetIpv4()<<" find RN "<<rni4aisa.GetIpv4()<<" EID "
                        <<(uint32_t)(buffer[15]-'0')<<" "<<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                        <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));
                      }
                    }else if(protocol_type == REGIST_TO_RN){//RN收到该包后，向解析注册  
                      uint8_t* buf = new uint8_t[EIDSIZE];
                      packet->CopyData(buf,EIDSIZE);        
                      AddCastTable(multicast_table,i4a,buf);
                      AddCastTable(unicast_table,i4a,buf);                    
                      i4a.SetInterfaceNum(0);
                      AddCastTable(multicast_table,i4a,buf);
                      AddCastTable(unicast_table,i4a,buf);                      
                      SeanetEID se(buffer);
                      AddEIDNAINFO(se,i4a);    
                      NeighInfoReply(fromipv4,m_port);       
                    }else if(protocol_type == GRAFITING_REQUEST){//组播节点收到嫁接请求，回复数据,回复时延探测
                      // NS_LOG_INFO("Multicast Node"<< mlocal.GetIpv4()<< " receive request from "<< fromipv4isa.GetIpv4());
                      SeanetHeader ssenh(MULTICAST_APPLICATION,MULTICAST_DATA_TRANS);
                      SendPacket(buffer,buffer_len,ssenh,from);    
                      NeighInfoReply(from,m_port);   
                      uint8_t* buf = new uint8_t[EIDSIZE];
                      packet->CopyData(buf,EIDSIZE);        
                      AddCastTable(multicast_table,i4a,buf);
                      // AddCastTable(unicast_table,i4a,buf);                    
                      i4a.SetInterfaceNum(0);
                      AddCastTable(multicast_table,i4a,buf);
                      // AddCastTable(unicast_table,i4a,buf);     
                      NS_LOG_LOGIC("12.3:switch address "<<mlocal.GetIpv4()<<" is_dst "<<is_dst
              <<" from address "<<fromipv4isa.GetIpv4()<<" application_type "<<application_type
              <<" protoco type "<<protocol_type<<" interface "<<interface_num<<" EID "<<(uint32_t)(buffer[15]-'0')<<" "<<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                        <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));  
                    }else if(protocol_type == REGIST_TO_DEST_DR){
                      //收端DR收到客户端的组播接收请求,选择最近的组播管理节点。这里可以直接向解析发送请求，解析回复iplist
                      // NS_LOG_INFO("DR receive multicast request");
                      uint8_t* buf = new uint8_t[EIDSIZE];
                      packet->CopyData(buf,EIDSIZE);        
                      AddCastTable(multicast_table,i4a,buf);
                      // AddCastTable(unicast_table,i4a,buf);                    
                      i4a.SetInterfaceNum(0);
                      AddCastTable(multicast_table,i4a,buf);
                      // AddCastTable(unicast_table,i4a,buf);     
                      
                      SeanetEID se(buffer);
                      Ipv4Address neartesti4a = FindNearestNode(se);
                      SeanetHeader ssenh(MULTICAST_APPLICATION,GRAFITING_REQUEST);
                      SendPacket(buffer,buffer_len,ssenh,neartesti4a); 
                      // SeanetHeader ssenh(RESOLUTION_APPLICATION,REQUEST_EID_NA);
                      // SendPacket(buffer,buffer_len,ssenh,resolution_addr); 
                      NS_LOG_LOGIC("12.5:switch address "<<mlocal.GetIpv4()<<" sendto nearerst "<<
              " address "<<InetSocketAddress(neartesti4a,m_port).GetIpv4()<<" EID "  <<(uint32_t)(buffer[15]-'0')<<" "<<(uint32_t)(buffer[16]-'0')<<" "<<(uint32_t)(buffer[17]-'0')<<" "
                      <<(uint32_t)(buffer[18]-'0')<<" "<<(uint32_t)(buffer[19]-'0'));
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
void SwitchApplicationv4::AddCastTable( sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash> *table,Ipv4Address i4a,uint8_t* buf){
  if(table->find(i4a)==table->end()){
    std::list<uint8_t*>* p = new std::list<uint8_t*>;
    p->push_back(buf);
    (*table)[i4a]=p;
  }else{
    if(table->find(i4a)->second==NULL){
      // NS_LOG_INFO("RN receive Unicast regist, add Unicast eid");
      std::list<uint8_t*>* p = new std::list<uint8_t*>;
      p->push_back(buf);
      (*table)[i4a]=p;             
    }else{
      if(!CheckDuplicate(table->find(i4a)->second,buf)){
        table->find(i4a)->second->push_back(buf);
      }
    }
  }

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

Ipv4Address SwitchApplicationv4::FindNearestNode(SeanetEID se){
  Time shortestime(1000000000);
  Ipv4Address shortestip;      
  if(resolution_table->find(se)==resolution_table->end() || resolution_table->find(se)->second == NULL){
    
    uint8_t eid[20];
    se.getSeanetEID(eid);
    // if(eid[16]=='9' && eid[17]=='2' && eid[18]=='2'){
      NS_LOG_INFO("EID NA TABLE NOT found ");
      se.Print();
    // }
  }else{

    if(this->tree_type == "SPT"){//第一个注册的点肯定是源节点。
      // NS_LOG_LOGIC("SPT "<<InetSocketAddress(Ipv4Address::ConvertFrom(*(resolution_table->find(se)->second->back())),m_port).GetIpv4());
      shortestip = *(resolution_table->find(se)->second->back());
      return shortestip;
    }
    std::list<Ipv4Address*>::iterator it = resolution_table->find(se)->second->begin();
    shortestip = *(*it);
    for(;it!=resolution_table->find(se)->second->end();it++){
      Ipv4Address temp = *(*it);
      if(delay_table.find(temp)!=delay_table.end()){
        if(delay_table[temp]<shortestime){
          shortestime=delay_table[temp];
          shortestip=temp;
        }
      }
    }
  }
  return shortestip;
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
      NS_LOG_INFO("switch "<<InetSocketAddress(Ipv4Address::ConvertFrom(local_address),m_port).GetIpv4()<<" eid "<<buffer[15]<<buffer[16]<<buffer[17]<<buffer[19]<<buffer[19]
      <<" receive resolve answer "<<ipnum);
      for(std::size_t i = 0; i < ipnum; i++){
        Ipv4Address ipv4=Ipv4Address::Deserialize (buffer+EIDSIZE+2 + 18 * i);
        if(i == 0){
          shortestip = ipv4;
        }
        if(delay_table.find(ipv4)!=delay_table.end()){
          if(delay_table[ipv4]<shortestime){
            shortestime = delay_table[ipv4];
            shortestip = ipv4;
          }
        }else{
          NS_LOG_INFO("ResolutionProtocolHandle, can't find delay info");
          SwitchApplicationv4::NeighInfoDetec(ipv4,m_port);
        }
      }
      if(finished==1){
        uint8_t buf[MAX_PAYLOAD_LEN];
        se.getSeanetEID(buf);
        SeanetHeader ssenh(MULTICAST_APPLICATION,GRAFITING_REQUEST);
        SendPacket(buf,MAX_PAYLOAD_LEN,ssenh,shortestip); 
        InetSocketAddress i4a = InetSocketAddress (shortestip, m_port);
        NS_LOG_INFO("grafting to nearest switch "<<i4a.GetIpv4()<<" eid "<<buf[15]<<buf[16]<<buf[17]<<buf[19]<<buf[19]);
      }
      // m_resolution_table[se]=shortestip;

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
  Ipv4Address i4a = Ipv4Address::ConvertFrom(local_address);
  AddEIDNAINFO(se,i4a);
  // SeanetHeader ssenh(RESOLUTION_APPLICATION,REGIST_EID_NA);
  // SwitchApplicationv4::SendPacket(buffer,EIDSIZE,ssenh,resolution_addr);
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
      NS_LOG_LOGIC("Switch send failed");
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
      // local_address.CopyTo(addr);
      // Ipv4Address lipv4 = Ipv4Address::Deserialize (addr);
      // NS_LOG_INFO("switch "<<InetSocketAddress(lipv4,m_port).GetIpv4()<<  " recieve neigh detect, reply to "<<i4a.GetIpv4());
      // SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_REPLY);
      // SendPacket(buffer,buffer_len,ssenh,stsh,from,m_port);
      NeighInfoReply(from,m_port);
      break;
    }
    case NEIGH_INFO_REPLY:{
      uint8_t addr[18];
      from.CopyTo(addr);
      Ipv4Address ipv4=Ipv4Address::Deserialize (addr);
      ipv4.SetInterfaceNum(0);
      // InetSocketAddress i4a = InetSocketAddress (ipv4, m_port);
      local_address.CopyTo(addr);
      // Ipv4Address lipv4 = Ipv4Address::Deserialize (addr);
      Time delay = Simulator::Now () - stsh.GetTs ();

      if(multicast_table->find(ipv4)!=multicast_table->end()){
        delay_table[ipv4]=delay;
      //   NS_LOG_INFO("switch"<< InetSocketAddress(lipv4,m_port).GetIpv4()
      // <<" receive neigh result, delay:"<<delay.GetTimeStep()<<" from address "<<i4a.GetIpv4());
      }
      break;
  }
    default:
      break;
    }
  }

void SwitchApplicationv4::AddEIDNAINFO(SeanetEID se,Ipv4Address i4a){
  
  i4a.SetInterfaceNum(0);
  if(resolution_table->find(se)==resolution_table->end() || resolution_table->find(se)->second == NULL){
    
    Ipv4Address* pi4a = new Ipv4Address(i4a);
    *pi4a = i4a;
    std::list<Ipv4Address*>*p = new std::list<Ipv4Address*>;
    p->push_back(pi4a);
    NS_LOG_LOGIC("Add info into Resolve table");
    se.Print();
    resolution_table->insert(std::make_pair(se,p));
    if(resolution_table->find(se)==resolution_table->end()){
      NS_LOG_INFO("Insert INfo failed");
    }
    // resolution_table->find(se)->second=p;
    
  }else{
    int flag = 0;
    for(std::list<Ipv4Address*>::iterator it = resolution_table->find(se)->second->begin();it!=resolution_table->find(se)->second->end();it++){
      if(*(*it)==i4a){
        flag = 1;
      }
    }
    if(flag != 1){
      Ipv4Address* pi4a = new Ipv4Address(i4a);
      *pi4a = i4a;
      resolution_table->find(se)->second->push_back(pi4a);
    }
  }
}
void SwitchApplicationv4::DetecAllNeighborDelay(){
  
  sgi::hash_map<Ipv4Address, std::list<uint8_t*>*, Ipv4AddressHash>::iterator mctit= multicast_table->begin();
  int total = 0;
  while(mctit != multicast_table->end()){
    if(total%10==0){
      Ipv4Address i4a = mctit->first;
      Ipv4Address locali4a = Ipv4Address::ConvertFrom(local_address);
      // NS_LOG_INFO("DetecAllNeighborDelay from "<<InetSocketAddress (locali4a, m_port).GetIpv4()<<" to "
      // <<InetSocketAddress (i4a, m_port).GetIpv4());
      if(i4a!=locali4a){
        Simulator::Schedule(Seconds(0.00001),&SwitchApplicationv4::NeighInfoDetec,this,Address(i4a),m_port);
      }
    }
    total++;
    mctit++;
  }
  NS_LOG_INFO("After DetecAllNeighborDelay");
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
void SwitchApplicationv4::NeighInfoReply(Address dst_ip,uint16_t dst_port){
  SeanetHeader ssenh(NEIGH_INFO_APPLICATION,NEIGH_INFO_REPLY,IS_DST);
  SeqTsSizeHeader stsh;
  stsh.SetSeq(0);
  stsh.SetSize(10);
  uint8_t buffer[10];
  memset(buffer,'6',10);
  SendPacket(buffer,10,ssenh,stsh,dst_ip,dst_port);
}
} // Namespace ns3
