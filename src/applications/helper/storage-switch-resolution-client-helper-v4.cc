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
#include "storage-switch-resolution-client-helper-v4.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

SwitchApplicationHelperv4::SwitchApplicationHelperv4 (Address resolution_addr)
{
  m_factory.SetTypeId (SwitchApplicationv4::GetTypeId ());
  SetAttribute ("Resolutionaddr", AddressValue (resolution_addr));
}

SwitchApplicationHelperv4::SwitchApplicationHelperv4 (Address resolution_addr, uint16_t port)
{
  m_factory.SetTypeId (SwitchApplicationv4::GetTypeId ());
  SetAttribute ("Resolutionaddr", AddressValue (resolution_addr));
  SetAttribute ("Port", UintegerValue (port));
}

//  SwitchApplicationHelperv4::SwitchApplicationHelperv4 (Address resolution_addr,uint16_t port,
//   sgi::hash_map<Ipv4Address, float, Ipv4AddressHash> UnicastTable,
//   sgi::hash_map<Ipv4Address, float, Ipv4AddressHash> MultiCastTable){
//   m_factory.SetTypeId (SwitchApplicationv4::GetTypeId ());
  
//   SetAttribute ("Resolutionaddr", AddressValue (resolution_addr));
//   SetAttribute ("Port", UintegerValue (port));
//   SetAttribute ("UnicastTable", UintegerValue (port));
// }

void
SwitchApplicationHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SwitchApplicationHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<SwitchApplicationv4> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}
ApplicationContainer SwitchApplicationHelperv4::Install (NodeContainer c,sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* sct,
  sgi::hash_map<Ipv4Address, float, Ipv4AddressHash>* mct)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<SwitchApplicationv4> ();
      m_server->SetNeighInfoTable(sct,mct);
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<SwitchApplicationv4>
SwitchApplicationHelperv4::GetServer (void)
{
  return m_server;
}


ResolutionApplicationHelperv4::ResolutionApplicationHelperv4 ()
{
  m_factory.SetTypeId (ResolutionApplicationv4::GetTypeId ());
}

ResolutionApplicationHelperv4::ResolutionApplicationHelperv4 (uint16_t port)
{
  m_factory.SetTypeId (ResolutionApplicationv4::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
ResolutionApplicationHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
ResolutionApplicationHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<ResolutionApplicationv4> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<ResolutionApplicationv4>
ResolutionApplicationHelperv4::GetServer (void)
{
  return m_server;
}
StorageClientApplicationHelperv4::StorageClientApplicationHelperv4 ()
{
  m_factory.SetTypeId (StorageClientApplicationv4::GetTypeId ());
}

StorageClientApplicationHelperv4::StorageClientApplicationHelperv4 (Address switch_address, uint16_t switch_port,
                                                                Address res_address, uint16_t res_port)
{
  m_factory.SetTypeId (StorageClientApplicationv4::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("SwitchPort", UintegerValue (switch_port));
  SetAttribute ("ResAddress", AddressValue (res_address));
  SetAttribute ("ResPort", UintegerValue (res_port));
}

StorageClientApplicationHelperv4::StorageClientApplicationHelperv4 (Address switch_address,Address res_address)
{
  m_factory.SetTypeId (StorageClientApplicationv4::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("ResAddress", AddressValue (res_address));
}

void
StorageClientApplicationHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
StorageClientApplicationHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<StorageClientApplicationv4> client = m_factory.Create<StorageClientApplicationv4> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

StorageClientApplicationTraceHelperv4::StorageClientApplicationTraceHelperv4 ()
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
}

StorageClientApplicationTraceHelperv4::StorageClientApplicationTraceHelperv4 (Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

StorageClientApplicationTraceHelperv4::StorageClientApplicationTraceHelperv4 (Address address, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("TraceFilename", StringValue (filename));
}

void
StorageClientApplicationTraceHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
StorageClientApplicationTraceHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<UdpTraceClient> client = m_factory.Create<UdpTraceClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}
MulticastClientApplicationHelperv4::MulticastClientApplicationHelperv4 ()
{
  m_factory.SetTypeId (MulticastClientApplicationv4::GetTypeId ());
}

MulticastClientApplicationHelperv4::MulticastClientApplicationHelperv4 (Address switch_address, uint16_t switch_port,
                                                                Address res_address, uint16_t res_port)
{
  m_factory.SetTypeId (MulticastClientApplicationv4::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("SwitchPort", UintegerValue (switch_port));
  SetAttribute ("ResAddress", AddressValue (res_address));
  SetAttribute ("ResPort", UintegerValue (res_port));
}

MulticastClientApplicationHelperv4::MulticastClientApplicationHelperv4 (Address switch_address,Address res_address)
{
  m_factory.SetTypeId (MulticastClientApplicationv4::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("ResAddress", AddressValue (res_address));
}

void
MulticastClientApplicationHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
MulticastClientApplicationHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<MulticastClientApplicationv4> client = m_factory.Create<MulticastClientApplicationv4> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}


MulticastClientApplicationTraceHelperv4::MulticastClientApplicationTraceHelperv4 ()
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
}

MulticastClientApplicationTraceHelperv4::MulticastClientApplicationTraceHelperv4 (Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

MulticastClientApplicationTraceHelperv4::MulticastClientApplicationTraceHelperv4 (Address address, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("TraceFilename", StringValue (filename));
}

void
MulticastClientApplicationTraceHelperv4::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
MulticastClientApplicationTraceHelperv4::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<UdpTraceClient> client = m_factory.Create<UdpTraceClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}
} // namespace ns3
