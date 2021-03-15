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
#include "storage-switch-resolution-client-helper.h"
#include "ns3/storage-client-application.h"
#include "ns3/storage-resolution-application.h"
#include "ns3/switch-application.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

SwitchApplicationHelper::SwitchApplicationHelper (Address resolution_addr)
{
  m_factory.SetTypeId (SwitchApplication::GetTypeId ());
  SetAttribute ("Resolutionaddr", AddressValue (resolution_addr));
}

SwitchApplicationHelper::SwitchApplicationHelper (Address resolution_addr, uint16_t port)
{
  m_factory.SetTypeId (SwitchApplication::GetTypeId ());
  SetAttribute ("Resolutionaddr", AddressValue (resolution_addr));
  SetAttribute ("Port", UintegerValue (port));
}

void
SwitchApplicationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SwitchApplicationHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<SwitchApplication> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<SwitchApplication>
SwitchApplicationHelper::GetServer (void)
{
  return m_server;
}


ResolutionApplicationHelper::ResolutionApplicationHelper ()
{
  m_factory.SetTypeId (ResolutionApplication::GetTypeId ());
}

ResolutionApplicationHelper::ResolutionApplicationHelper (uint16_t port)
{
  m_factory.SetTypeId (ResolutionApplication::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
ResolutionApplicationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
ResolutionApplicationHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<ResolutionApplication> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<ResolutionApplication>
ResolutionApplicationHelper::GetServer (void)
{
  return m_server;
}
StorageClientApplicationHelper::StorageClientApplicationHelper ()
{
  m_factory.SetTypeId (StorageClientApplication::GetTypeId ());
}

StorageClientApplicationHelper::StorageClientApplicationHelper (Address switch_address, uint16_t switch_port,
                                                                Address res_address, uint16_t res_port)
{
  m_factory.SetTypeId (StorageClientApplication::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("SwitchPort", UintegerValue (switch_port));
  SetAttribute ("ResAddress", AddressValue (res_address));
  SetAttribute ("ResPort", UintegerValue (res_port));
}

StorageClientApplicationHelper::StorageClientApplicationHelper (Address switch_address,Address res_address)
{
  m_factory.SetTypeId (StorageClientApplication::GetTypeId ());
  SetAttribute ("SwitchAddress", AddressValue (switch_address));
  SetAttribute ("ResAddress", AddressValue (res_address));
}

void
StorageClientApplicationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
StorageClientApplicationHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<StorageClientApplication> client = m_factory.Create<StorageClientApplication> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

StorageClientApplicationTraceHelper::StorageClientApplicationTraceHelper ()
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
}

StorageClientApplicationTraceHelper::StorageClientApplicationTraceHelper (Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

StorageClientApplicationTraceHelper::StorageClientApplicationTraceHelper (Address address, std::string filename)
{
  m_factory.SetTypeId (UdpTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("TraceFilename", StringValue (filename));
}

void
StorageClientApplicationTraceHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
StorageClientApplicationTraceHelper::Install (NodeContainer c)
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
