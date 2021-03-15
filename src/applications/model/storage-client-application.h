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

#ifndef STORAGE_CLIENT_APP_H
#define STORAGE_CLIENT_APP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/seanet-protocol.h"
#include "ns3/seanet-header.h"
namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup udpclientserver
 *
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class StorageClientApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StorageClientApplication ();

  virtual ~StorageClientApplication ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address switch_ip,Address res_ip,uint16_t res_port, uint16_t switch_port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address switch_ip,Address res_ip);

protected:
  virtual void DoDispose (void);

private:
  void ReceiveCallback (Ptr<Socket> socket);
  void ResolutionProtocolHandle(uint8_t* buffer, uint8_t buffer_len,
                               uint8_t protocoal_type, Address from);
  void SwitchProtocolHandle(uint8_t* buffer, uint8_t buffer_len, 
                            uint8_t protocoal_type, Address from);
  
  void ResolutionReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from);

  void SwitchReplyHandle(uint8_t* buffer, uint8_t buffer_len, Address from);

  void SendPacket(uint8_t* buffer, uint8_t buffer_len,SeanetHeader ssenh,Address to, uint16_t port);
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Write (void);
  void Read (void); 
  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)

  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_switch_socket,m_resolution_socket; //!< Socket
  Address m_switch_address, m_resolution_address, m_local_address; //!< Remote peer address
  uint16_t m_switch_port,m_resolution_port; //!< Remote peer port
  EventId m_sendEvent, m_readEvent; //!< Event to send the next packet

};

} // namespace ns3

#endif /* UDP_CLIENT_H */
