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

#ifndef SEANETADDRESS_H
#define SEANETADDRESS_H

#include <stdint.h>
#include <ostream>
#include "ns3/object.h"
#include "ns3/address.h"
namespace ns3 {

/**
 * \ingroup network
 * \defgroup address SeanetAddress 
 *
 * Network SeanetAddress  abstractions, including MAC, IPv4 and IPv6.
 */
/**
 * \ingroup address
 * \brief a polymophic address class
 *
 * This class is very similar in design and spirit to the BSD sockaddr
 * structure: they are both used to hold multiple types of addresses
 * together with the type of the address.
 *
 * A new address class defined by a user needs to:
 *   - allocate a type id with SeanetAddress ::Register
 *   - provide a method to convert his new address to an SeanetAddress  
 *     instance. This method is typically a member method named ConvertTo:
 *     SeanetAddress  MyAddress::ConvertTo (void) const;
 *   - provide a method to convert an SeanetAddress  instance back to
 *     an instance of his new address type. This method is typically
 *     a static member method of his address class named ConvertFrom:
 *     static MyAddress MyAddress::ConvertFrom (const SeanetAddress  &address);
 *   - the ConvertFrom method is expected to check that the type of the
 *     input SeanetAddress  instance is compatible with its own type.
 *
 * Typical code to create a new class type looks like:
 * \code
 * // this class represents addresses which are 2 bytes long.
 * class MyAddress
 * {
 * public:
 *   SeanetAddress  ConvertTo (void) const;
 *   static MyAddress ConvertFrom (void);
 * private:
 *   static uint8_t GetType (void);
 * };
 *
 * SeanetAddress  MyAddress::ConvertTo (void) const
 * {
 *   return SeanetAddress  (GetType (), m_buffer, 2);
 * }
 * MyAddress MyAddress::ConvertFrom (const SeanetAddress  &address)
 * {
 *   MyAddress ad;
 *   NS_ASSERT (address.CheckCompatible (GetType (), 2));
 *   address.CopyTo (ad.m_buffer, 2);
 *   return ad;
 * }
 * uint8_t MyAddress::GetType (void)
 * {
 *   static uint8_t type = SeanetAddress ::Register ();
 *   return type;
 * }
 * \endcode
 *
 * \see attribute_Address
 */
class SeanetAddress : public Object
{
public:
  enum MaxSize_e {
    MAX_SIZE = 20
  };
  SeanetAddress();
  SeanetAddress (uint8_t type, const uint8_t *buffer, uint8_t len);
  SeanetAddress (const SeanetAddress & address);
  uint8_t m_type; //!< Type of the address
  uint8_t m_len;  //!< Length of the address
  uint8_t m_data[MAX_SIZE]; //!< The address value
  void fromAddress(const Address & address);
  void toAddress(Address & address);
  inline uint32_t GetSize (void) const;
};

ATTRIBUTE_HELPER_HEADER (SeanetAddress );
} // namespace ns3

/****************************************************
 *  Implementation of inline methods for performance
 ****************************************************/
namespace ns3 {

uint32_t SeanetAddress::GetSize(void) const{
  return (2 + MAX_SIZE);
}

} // namespace ns3
#endif /* SEANETADDRESS_H */
