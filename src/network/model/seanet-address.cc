/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
#include "seanet-address.h"
#include <cstring>
#include <iostream>
#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeanetAddress");

SeanetAddress::SeanetAddress ()
  : m_type (0),
    m_len (0)
{
  // Buffer left uninitialized
  NS_LOG_FUNCTION (this);

}

SeanetAddress::SeanetAddress (uint8_t type, const uint8_t *buffer, uint8_t len)
  : m_type (type),
    m_len (len)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (type) << &buffer << static_cast<uint32_t> (len));
  NS_ASSERT (m_len <= MAX_SIZE);
  std::memcpy (m_data, buffer, m_len);
}
SeanetAddress::SeanetAddress (const SeanetAddress & address)
  : m_type (address.m_type),
    m_len (address.m_len)
{
  NS_ASSERT (m_len <= MAX_SIZE);
  std::memcpy (m_data, address.m_data, m_len);
}
void SeanetAddress::fromAddress(const Address & address){
  uint8_t buffer[MAX_SIZE+2];
  uint32_t len = address.CopyAllTo(buffer,MAX_SIZE+2);
  m_type = buffer[0];
  m_len = buffer[1];
  std::memcpy(m_data,buffer+2,len);
  NS_LOG_ERROR("m_type: "<< m_type <<"m_len :"<<m_len<<" mdata:"<<m_data);
}
void SeanetAddress::toAddress(Address & address){
  uint8_t buffer[MAX_SIZE+2];
  buffer[0]=m_type;
  buffer[1]=m_len;
  memcpy(buffer+2,m_data,m_len);
  address.CopyAllFrom(buffer,m_len+2);
}

} // namespace ns3
