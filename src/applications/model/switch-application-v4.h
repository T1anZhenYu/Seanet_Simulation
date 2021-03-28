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
 *
 */

#ifndef STORAGE_SWITCH_APP_V4_H
#define STORAGE_SWITCH_APP_V4_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/seanet-address.h"
#include "ns3/traced-callback.h"
#include "packet-loss-counter.h"
#include "ns3/queue.h"
#include "ns3/seanet-eid.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/seanet-protocol.h"
#include "ns3/seanet-header.h"
#include "ns3/seq-ts-size-header.h"
namespace ns3 {
/**
 * \ingroup applications
 * \defgroup udpclientserver UdpClientServer
 */

/**
 * \ingroup udpclientserver
 *
 * \brief A UDP server, receives UDP packets from a remote host.
 *
 * UDP packets carry a 32bits sequence number followed by a 64bits time
 * stamp in their payloads. The application uses the sequence number
 * to determine if a packet is lost, and the time stamp to compute the delay.
 */
// typedef sgi::hash_map<Ipv4Address, float, Ipv4AddressHash> NeighInfoTable;
  
class SwitchApplicationv4 : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  SwitchApplicationv4 ();
  virtual ~SwitchApplicationv4 ();
  /**
   * \brief Returns the number of lost packets
   * \return the number of lost packets
   */
  uint32_t GetLost (void) const;

  /**
   * \brief Returns the number of received packets
   * \return the number of received packets
   */
  uint64_t GetReceived (void) const;

  /**
   * \brief Returns the size of the window used for checking loss.
   * \return the size of the window used for checking loss.
   */
  uint16_t GetPacketWindowSize () const;
  void SetNeighInfoTable(sgi::hash_map<Ipv4Address, std::list<uint8_t*>*, Ipv4AddressHash>* sct,
  sgi::hash_map<Ipv4Address, std::list<uint8_t*>*, Ipv4AddressHash>* mct,
  sgi::hash_map<SeanetEID, std::list<Ipv4Address*>*, SeanetEIDHash>*ent);
  bool CheckDuplicate(std::list<uint8_t*>*head, uint8_t* buf);
  /**
   * \brief Set the size of the window used for checking loss. This value should
   *  be a multiple of 8
   * \param size the size of the window used for checking loss. This value should
   *  be a multiple of 8
   */
  void SetPacketWindowSize (uint16_t size);

  uint8_t LookupEIDTable(SeanetEID se);
  uint8_t AddEIDTable(SeanetEID se, uint8_t value);
  void AddEIDNAINFO(SeanetEID se,Ipv4Address i4a);
  Ipv4Address FindNearestNode(SeanetEID se);
  Ipv4Address LookupEIDNAINFO(SeanetEID se);
  void SetEntrySwitch(bool isEntry);
  void RandomCache(uint16_t size);
  void NeigthInfoProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
        uint8_t protocoal_type, Address from,SeqTsSizeHeader stsh);
  void DetecAllNeighborDelay();
  void SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to);
  void SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,SeqTsSizeHeader stsh,Address to, uint16_t port);
protected:
  virtual void DoDispose (void);

private:
  
  Ipv4Address GetRootNode();
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  //Handle packet with switch protocol
  void SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len, uint8_t protocoal_type, Address from);
  //handle data request from other nodes
  void RequestDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from);
  //handle data receive from other nodes
  void ReceiveDataHandle(uint8_t* buffer, uint8_t buffer_len, Address from);
  void NeighInfoDetec(Address dst_ip,uint16_t dst_port);
  int IsDestination(Address address);
  void ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len, uint8_t protocoal_type, Address from);
  float GetSwitchSocre(Ipv4Address ad);
  void NeighInfoReply(Address dst_ip,uint16_t dst_port);
  //Send packet 
  void AddCastTable( sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash> *table,Ipv4Address i4a,uint8_t* buf);
  /**
   * \brief Handle frontend issue.
   *
   * This function is called as a callback function when a packet is received.
   *
   * \param socket the socket the packet was received to.
   */
  void FrontEnd (Ptr<Socket> socket);

  /**
   * \brief Handle afterend issue.
   *
   * This function is called  periodicity to check .
   *
   * \param socket the socket the packet was received to.
   */
  void AferEnd ();
  
  typedef sgi::hash_map<SeanetEID, uint8_t, SeanetEIDHash> EID_table; //the eid this switch stores.

  Ptr<Queue<Packet> > packetin;//connet frontend and afterend.
  Ptr<Queue<SeanetAddress>> addressin;
  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket
  uint64_t m_received; //!< Number of received packets
  uint16_t m_cache_size; // total cache size;
  uint16_t m_temp_cache_size;
  bool is_entry_switch;// is entry
  PacketLossCounter m_loss_counter; //!< Lost packet counter
  EventId m_AfterEndEvent,m_RandomCacheEvent;
  EID_table m_eid_table;
  std::string tree_type;
  sgi::hash_map<Ipv4Address, Time, Ipv4AddressHash> delay_table;
  Address resolution_addr,local_address;
  sgi::hash_map<Ipv4Address, std::list<uint8_t *>*, Ipv4AddressHash> *unicast_table, *multicast_table;
  sgi::hash_map<SeanetEID, std::list<Ipv4Address*>*, SeanetEIDHash> *resolution_table;
  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > rm_rx_trace;
  bool have_detected;
  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rx_trace_with_addresses;

};

} // namespace ns3

#endif /* UDP_SERVER_H */
