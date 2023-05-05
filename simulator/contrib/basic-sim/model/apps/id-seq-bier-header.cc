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
 * Author: Simon
 * Based on SeqTsHeader by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "id-seq-bier-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IdSeqBIERHeader");

NS_OBJECT_ENSURE_REGISTERED (IdSeqBIERHeader);

TypeId
IdSeqBIERHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::IdSeqHeader")
            .SetParent<Header> ()
            .SetGroupName("BasicSim")
            .AddConstructor<IdSeqHeader> ()
    ;
    return tid;
}

IdSeqBIERHeader::IdSeqBIERHeader ()
  : m_id (0),
    m_seq (0)
{
  for(int i = 0; i < BS_LEN_32; i++) m_bier_bs[i] = 0; //init bs
  NS_LOG_FUNCTION (this);
}

void 
IdSeqBIERHeader::SetBP(uint32_t bitposi, uint8_t content) //set to 0 or 1
{
  NS_ASSERT(0 <= bitposi && bitposi < 32*BS_LEN_32);
  NS_ASSERT(content == 0 || content == 1);
  int uintposi = bitposi/32; //array index
  int bitwithshift = bitposi%32;
  if (content == 0) {
    m_bier_bs[uintposi] &= ~(1 << bitwithshift); //set to 0
  }
  else {
    m_bier_bs[uintposi] |= 1 << bitwithshift; //set to 1
  }
} 

bool 
IdSeqBIERHeader::TestBP(uint32_t bitposi) //if it is 1
{
  NS_ASSERT(0 <= bitposi && bitposi < 32*BS_LEN_32);
  int uintposi = bitposi/32; //array index
  int bitwithshift = bitposi%32;
  uint32_t mask = 1 << bitwithshift;
  return (m_bier_bs[uintposi] & mask) == mask; //true if it is 1 else false
}

uint32_t* 
IdSeqBIERHeader::GetBS (void) const //get bs pointer
{
  return (uint32_t*)m_bier_bs;
}

void
IdSeqBIERHeader::SetId (uint64_t id)
{
    NS_LOG_FUNCTION (this << id);
    m_id = id;
}

uint64_t
IdSeqBIERHeader::GetId (void) const
{
    NS_LOG_FUNCTION (this);
    return m_id;
}

void
IdSeqBIERHeader::SetSeq (uint64_t seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_seq = seq;
}

uint64_t
IdSeqBIERHeader::GetSeq (void) const
{
  NS_LOG_FUNCTION (this);
  return m_seq;
}

TypeId
IdSeqBIERHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
IdSeqBIERHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(id=" << m_id << ", seq=" << m_seq << ")";
}

uint32_t
IdSeqBIERHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  // return 8+8;
  return 8+8+BS_LEN_32*4;
}

void
IdSeqBIERHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_id);
  i.WriteHtonU64 (m_seq);
  i.Write((uint8_t*)m_bier_bs, BS_LEN_32*4);
}

uint32_t
IdSeqBIERHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_id = i.ReadNtohU64 ();
  m_seq = i.ReadNtohU64 ();
  i.Read((uint8_t*)m_bier_bs, BS_LEN_32*4);
  return GetSerializedSize ();
}

} // namespace ns3
