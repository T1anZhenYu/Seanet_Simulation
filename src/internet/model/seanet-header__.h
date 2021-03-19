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

#ifndef SEANET_HEADER_H__
#define SEANET_HEADER_H__

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
/**
 * \ingroup applications
 *
 * \brief Packet header to carry sequence number and timestamp
 *
 * The header is used as a payload in applications (typically UDP) to convey
 * a 32 bit sequence number followed by a 64 bit timestamp (12 bytes total).
 *
 * The timestamp is not set explicitly but automatically set to the
 * simulation time upon creation.
 *
 * If you need space for an application data unit size field (e.g. for 
 * stream-based protocols like TCP), use ns3::SeqTsSizeHeader.
 *
 * \sa ns3::SeqTsSizeHeader
 */
class SeanetHeader__ : public Header
{
public:
  SeanetHeader__();
  SeanetHeader__(uint8_t at, uint8_t pt);
  SeanetHeader__(uint8_t at, uint8_t pt, uint8_t dst);

  /**
   * \param num the eid list len
   */
  void SetApplicationType (uint8_t at);
  /**
   * \return the eid list len
   */
  uint8_t GetApplicationType (void) const;
  /**
   * \param num the eid flag
   */
  void SetProtocolType (uint8_t pt);
  /**
   * \return the eid flag
   */
  uint8_t GetProtocolType (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  void Setdst (uint8_t at);
  uint8_t Getdst(void) const;
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint8_t application_type; //!< eid num
  uint8_t protocol_type; //!< eid flag
  uint8_t is_dst;
};

} // namespace ns3

#endif /* SEQ_TS_HEADER_H */
