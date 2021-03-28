/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "seanet-header_.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeanetHeader_");

NS_OBJECT_ENSURE_REGISTERED (SeanetHeader_);
SeanetHeader_::SeanetHeader_ ()
  : application_type(0x01),
    protocol_type (0x01),
    is_dst(0x00)
{
  NS_LOG_FUNCTION (this);
}
SeanetHeader_::SeanetHeader_ (uint8_t at, uint8_t pt)
  : application_type(at),
    protocol_type (pt),
    is_dst(0x00)
{
  NS_LOG_FUNCTION (this);
}
SeanetHeader_::SeanetHeader_ (uint8_t at, uint8_t pt, uint8_t dst)
  : application_type(at),
    protocol_type (pt),
    is_dst(dst)
{
  NS_LOG_FUNCTION (this);
}

void
SeanetHeader_::SetApplicationType (uint8_t at)
{
  NS_LOG_FUNCTION (this << application_type);
  application_type = at;
}
uint8_t
SeanetHeader_::GetApplicationType(void) const
{
  NS_LOG_FUNCTION (this);
  return application_type;
}
void SeanetHeader_::Setdst (uint8_t at){
  is_dst = at;
}

uint8_t SeanetHeader_::Getdst(void) const
{
  NS_LOG_FUNCTION (this);
  return is_dst;
}
void SeanetHeader_::SetInterface(uint8_t num){
  interface_num = num;
}
uint8_t SeanetHeader_::GetInterface(){
  return interface_num;
}
void
SeanetHeader_::SetProtocolType (uint8_t pt)
{
  NS_LOG_FUNCTION (this);
  protocol_type = pt;
}
uint8_t
SeanetHeader_::GetProtocolType (void) const
{
  NS_LOG_FUNCTION (this);
  return protocol_type;
}

TypeId
SeanetHeader_::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeanetHeader_")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<SeanetHeader_> ()
  ;
  return tid;
}
TypeId
SeanetHeader_::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
SeanetHeader_::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(application_type=" << application_type
   << " protocol_type=" << protocol_type 
   << " is_dst= "<<is_dst<<" intefacenum "<< interface_num<< ")";
}
uint32_t
SeanetHeader_::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}

void
SeanetHeader_::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteU8 (application_type, 1);
  i.WriteU8 (protocol_type, 1);
  i.WriteU8 (is_dst, 1);
  i.WriteU8(interface_num,1);
}
uint32_t
SeanetHeader_::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  application_type = i.ReadU8();
  protocol_type = i.ReadU8 ();
  is_dst = i.ReadU8 ();
  interface_num = i.ReadU8();
  return GetSerializedSize ();
}

} // namespace ns3
