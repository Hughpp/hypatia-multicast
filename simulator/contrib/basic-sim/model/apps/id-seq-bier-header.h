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

//all bier related struct
#ifndef ID_SEQ_BIER_HEADER_H
#define ID_SEQ_BIER_HEADER_H
#define BS_LEN_32 4 
#define USE_BIER true
// #define USE_BIER false
//32 bit as a unit

#include "ns3/header.h"
#include "ns3/id-seq-header.h"
// #include <bitset>

namespace ns3 {
    
class IdSeqBIERHeader : public Header
{
public:
  static TypeId GetTypeId (void);
    
  IdSeqBIERHeader ();
  void SetId (uint64_t id);
  void SetSeq (uint64_t seq);
  void SetBP(uint32_t bitposi, uint8_t content); //set to 0(false) or 1(true)
  void SetBS(uint32_t* bs);
  bool TestBP(uint32_t bitposi); //if it is 1
  uint32_t* GetBS (void) const; //get whole bs
  uint64_t GetId (void) const;
  uint64_t GetSeq (void) const;

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint64_t m_id;  //!< Identifier of what these sequences belong to
  uint64_t m_seq; //!< Sequence number
  uint32_t m_bier_bs[BS_LEN_32]; //
  // std::bitset<128> m_bier_bs;// = {0}; // used by BIER, init as 0
};

class BIERTableEntry
{
  // BP: int
  // node_id: uint32_t
  // BS part: uint32_t
public:
    BIERTableEntry(int bfer_bp, uint32_t fbm[BS_LEN_32], int nexthop_bp, uint32_t bfer_id, uint32_t oifidx, uint32_t bfer_addr, uint32_t nxthop_addr);
    int GetBFERBP();
    uint32_t GetBFERId();
    uint32_t* GetFbm();
    int GetNexthopBP();
    uint32_t GetOifIdx();
    uint32_t GetNexthopAddr();
    void BitwiseAndWith(uint32_t tarbs[BS_LEN_32]);

private:
    int m_bfer_bp;
    uint32_t m_fbm[BS_LEN_32];
    int m_nexthop_bp;
    // extra info
    uint32_t m_bfer_id;
    uint32_t m_oifidx;
    uint32_t m_bfer_addr; //useless
    uint32_t m_nxthop_addr;
};

} // namespace ns3

namespace bsOpera {

void BScopy(uint32_t tar[BS_LEN_32], uint32_t src[BS_LEN_32]);
void BSset(uint32_t tar[BS_LEN_32], int posi, bool active);
void BSreset(uint32_t tar[BS_LEN_32]);
void BSAnd(uint32_t res[BS_LEN_32], uint32_t a[BS_LEN_32], uint32_t b[BS_LEN_32]);
void BSXor(uint32_t res[BS_LEN_32], uint32_t a[BS_LEN_32], uint32_t b[BS_LEN_32]);
bool BSEqualZero(uint32_t bs[BS_LEN_32]);
int BSFindLeastActive(uint32_t bs[BS_LEN_32]);
void BSPrint(uint32_t tar[BS_LEN_32]);

} // namespace bsOpera

#endif /* ID_SEQ_BIER_HEADER_H */
