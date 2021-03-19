/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#ifndef STORAGE_SWITCH_RESOLUTION_CLIENT_HELPER_V4_H
#define STORAGE_SWITCH_RESOLUTION_CLIENT_HELPER_V4_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/storage-client-application-v4.h"
#include "ns3/switch-application-v4.h"
#include "ns3/resolution-application-v4.h"
#include "ns3/seanet-header.h"
#include "ns3/multicast-client-application-v4.h"

namespace ns3 {

/**
 * \ingroup udpclientserver
 * \brief Create a server application which waits for input UDP packets
 *        and uses the information carried into their payload to compute
 *        delay and to determine if some packets are lost.
 */
class SwitchApplicationHelperv4
{
public:
  /**
   * Create SwitchApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   */
  SwitchApplicationHelperv4 (Address resolution_addr);

  /**
   * Create SwitchApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   * \param port The port the server will wait on for incoming packets
   */
  SwitchApplicationHelperv4 (Address resolution_addr,uint16_t port);

  // SwitchApplicationHelperv4 (Address resolution_addr,uint16_t port,
  // sgi::hash_map<Ipv4Address, float, Ipv4AddressHash> UnicastTable,
  // sgi::hash_map<Ipv4Address, float, Ipv4AddressHash> MultiCastTable);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

  ApplicationContainer Install (NodeContainer c,sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* sct,
  sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* mct);

  /**
   * \brief Return the last created server.
   *
   * This function is mainly used for testing.
   *
   * \returns a Ptr to the last created server application
   */
  Ptr<SwitchApplicationv4> GetServer (void);
private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<SwitchApplicationv4> m_server; //!< The last created server application
};



class ResolutionApplicationHelperv4
{
public:
  /**
   * Create ResolutionApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   */
  ResolutionApplicationHelperv4 ();

  /**
   * Create ResolutionApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   * \param port The port the server will wait on for incoming packets
   */
  ResolutionApplicationHelperv4 (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * \brief Return the last created server.
   *
   * This function is mainly used for testing.
   *
   * \returns a Ptr to the last created server application
   */
  Ptr<ResolutionApplicationv4> GetServer (void);
private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<ResolutionApplicationv4> m_server; //!< The last created server application
};
/**
 * \ingroup udpclientserver
 * \brief Create a client application which sends UDP packets carrying
 *  a 32bit sequence number and a 64 bit time stamp.
 *
 */
class StorageClientApplicationHelperv4
{

public:
  /**
   * Create StorageClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server.
   *
   */
  StorageClientApplicationHelperv4 ();

  /**
   *  Create StorageClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do not include a port value (e.g., Ipv4Address and
   * Ipv6Address).
   *
   * \param ip The IP address of the remote UDP server
   * \param port The port number of the remote UDP server
   */

  StorageClientApplicationHelperv4 (Address switch_ip, uint16_t switch_port,Address res_ip,uint16_t res_port);
  /**
   *  Create StorageClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do include a port value (e.g., InetSocketAddress and
   * Inet6SocketAddress).
   *
   * \param addr The address of the remote UDP server
   */

  StorageClientApplicationHelperv4 (Address switch_ip,Address res_ip);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
     * \param c the nodes
     *
     * Create one UDP client application on each of the input nodes
     *
     * \returns the applications created, one application per input node.
     */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
};
/**
 * \ingroup udpclientserver
 * Create UdpTraceClient application which sends UDP packets based on a trace
 * file of an MPEG4 stream. Trace files could be downloaded form :
 * http://www2.tkn.tu-berlin.de/research/trace/ltvt.html (the 2 first lines of
 * the file should be removed)
 * A valid trace file is a file with 4 columns:
 * \li -1- the first one represents the frame index
 * \li -2- the second one indicates the type of the frame: I, P or B
 * \li -3- the third one indicates the time on which the frame was generated by the encoder
 * \li -4- the fourth one indicates the frame size in byte
*/
class StorageClientApplicationTraceHelperv4
{
public:
  /**
   * Create StorageClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server.
   *
   */
  StorageClientApplicationTraceHelperv4 ();

  /**
   * Create StorageClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do not include a port value (e.g., Ipv4Address and
   * Ipv6Address).
   *
   * \param ip The IP address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  StorageClientApplicationTraceHelperv4 (Address ip, uint16_t port, std::string filename);
  /**
   * Create StorageClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do include a port value (e.g., InetSocketAddress and
   * Inet6SocketAddress).
   *
   * \param addr The address of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  StorageClientApplicationTraceHelperv4 (Address addr, std::string filename);

  /**
    * Record an attribute to be set in each Application after it is is created.
    *
    * \param name the name of the attribute to set
    * \param value the value of the attribute to set
    */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
    * \param c the nodes
    *
    * Create one UDP trace client application on each of the input nodes
    *
    * \returns the applications created, one application per input node.
    */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
};

/**
 * \ingroup udpclientserver
 * \brief Create a client application which sends UDP packets carrying
 *  a 32bit sequence number and a 64 bit time stamp.
 *
 */
class MulticastClientApplicationHelperv4
{

public:
  /**
   * Create MulticastClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server.
   *
   */
  MulticastClientApplicationHelperv4 ();

  /**
   *  Create MulticastClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do not include a port value (e.g., Ipv4Address and
   * Ipv6Address).
   *
   * \param ip The IP address of the remote UDP server
   * \param port The port number of the remote UDP server
   */

  MulticastClientApplicationHelperv4 (Address switch_ip, uint16_t switch_port,Address res_ip,uint16_t res_port);
  /**
   *  Create MulticastClientApplicationHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do include a port value (e.g., InetSocketAddress and
   * Inet6SocketAddress).
   *
   * \param addr The address of the remote UDP server
   */

  MulticastClientApplicationHelperv4 (Address switch_ip,Address res_ip);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
     * \param c the nodes
     *
     * Create one UDP client application on each of the input nodes
     *
     * \returns the applications created, one application per input node.
     */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
};


/**
 * \ingroup udpclientserver
 * Create UdpTraceClient application which sends UDP packets based on a trace
 * file of an MPEG4 stream. Trace files could be downloaded form :
 * http://www2.tkn.tu-berlin.de/research/trace/ltvt.html (the 2 first lines of
 * the file should be removed)
 * A valid trace file is a file with 4 columns:
 * \li -1- the first one represents the frame index
 * \li -2- the second one indicates the type of the frame: I, P or B
 * \li -3- the third one indicates the time on which the frame was generated by the encoder
 * \li -4- the fourth one indicates the frame size in byte
*/
class MulticastClientApplicationTraceHelperv4
{
public:
  /**
   * Create MulticastClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server.
   *
   */
  MulticastClientApplicationTraceHelperv4 ();

  /**
   * Create MulticastClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do not include a port value (e.g., Ipv4Address and
   * Ipv6Address).
   *
   * \param ip The IP address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  MulticastClientApplicationTraceHelperv4 (Address ip, uint16_t port, std::string filename);
  /**
   * Create MulticastClientApplicationTraceHelperv4 which will make life easier for people trying
   * to set up simulations with udp-client-server. Use this variant with
   * addresses that do include a port value (e.g., InetSocketAddress and
   * Inet6SocketAddress).
   *
   * \param addr The address of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  MulticastClientApplicationTraceHelperv4 (Address addr, std::string filename);

  /**
    * Record an attribute to be set in each Application after it is is created.
    *
    * \param name the name of the attribute to set
    * \param value the value of the attribute to set
    */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
    * \param c the nodes
    *
    * Create one UDP trace client application on each of the input nodes
    *
    * \returns the applications created, one application per input node.
    */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
};
} // namespace ns3

#endif /* MY_UDP_CLIENT_SERVER_H */
