#ifndef ARBITER_MULTICAST_H
#define ARBITER_MULTICAST_H

#include <map>
#include "ns3/arbiter-ptop.h"
#include "ns3/topology-ptop.h"
#include "ns3/arbiter-ecmp.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/id-seq-bier-header.h"
// #include "ns3/ipv4-routing-protocol.h"

namespace ns3 {
class Ipv4MulticastRoutingTableEntry;
class BIERTableEntry;

class ArbiterMulticast : public ArbiterEcmp
{
public:
    static TypeId GetTypeId (void);

    // Constructor for Multicast forwarding state
    ArbiterMulticast(
            Ptr<Node> this_node,
            NodeContainer nodes,
            Ptr<TopologyPtop> topology,
            std::vector<std::vector<uint32_t>> candidate_list
    );

    //inputInterface is useless now
    void AddMulticastRoute(Ipv4Address origin, Ipv4Address group, uint32_t inputInterface, std::vector<uint32_t> outputInterfaces);
    void SetMulticastRoutes(std::list<Ipv4MulticastRoutingTableEntry *> multicast_routes);
    ArbiterResult DecideMulticast(int32_t source_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader);

    //used by bier
    uint32_t GetBpFromNodeID(uint32_t node_id);

    // get routing table
    // std::string StringReprOfForwardingState();

private:
    std::list<Ipv4MulticastRoutingTableEntry *> m_multicast_routes;
    std::list<BIERTableEntry *> m_bier_table;
    std::map<uint32_t, uint32_t> m_node_id_to_bp;

};

}

#endif //ARBITER_MULTICAST_H
