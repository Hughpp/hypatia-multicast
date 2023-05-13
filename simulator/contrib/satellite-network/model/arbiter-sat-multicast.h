/*
 * Copyright (c) 2020 ETH Zurich
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
 * Author: Simon               2020
 */

#ifndef ARBITER_SAT_MULTICAST_H
#define ARBITER_SAT_MULTICAST_H

#include <tuple>
#include "ns3/arbiter-satnet.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/hash.h"
#include "ns3/abort.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/id-seq-bier-header.h"

namespace ns3 {

class ArbiterSatMulticast : public ArbiterSingleForward
{
public:
    static TypeId GetTypeId (void);

    // Constructor for single forward next-hop forwarding state
    ArbiterSatMulticast(
            Ptr<Node> this_node,
            NodeContainer nodes,
            std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list
    );

    //same as arbiter-multicast
    void AddMulticastRoute(Ipv4Address origin, Ipv4Address group, uint32_t inputInterface, std::vector<uint32_t> outputInterfaces);
    void SetMulticastRoutes(std::list<Ipv4MulticastRoutingTableEntry *> multicast_routes);
    void ClearMulticastRoutes();
    // std::vector<std::tuple<int32_t, int32_t, int32_t>> GetNextHopList();
    ArbiterResult DecideMulticast(int32_t source_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader);
    
    // Static routing table
    // std::string StringReprOfForwardingState();

    //bier
    // uint32_t GetBpFromNodeID(uint32_t node_id);
    BIERTableEntry& LookupBIERTable(int bp);
    void ClearBIERRoutes();
    void AddBIERRoute(int bfer_bp, uint32_t fbm[BS_LEN_32], int nexthop_bp, uint32_t bfer_id, uint32_t oifidx, uint32_t bfer_addr, uint32_t nxthop_addr);

private:
    std::list<Ipv4MulticastRoutingTableEntry *> m_multicast_routes;
    std::list<BIERTableEntry *> m_bier_table;
    // std::map<uint32_t, uint32_t> m_node_id_to_bp;
};

}

#endif //ARBITER_SAT_MULTICAST_H
