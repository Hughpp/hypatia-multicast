#ifndef ARBITER_MULTICAST_HELPER_H
#define ARBITER_MULTICAST_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-ptop.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/arbiter-multicast.h"
#include "ns3/arbiter-ecmp-helper.h"
#include "ns3/multicast-udp-schedule-reader.h"

namespace ns3 {
    class Ipv4MulticastRoutingTableEntry;
    class ArbiterEcmpHelper;

    class ArbiterMulticastHelper : public ArbiterEcmpHelper
    {
    public:
        ArbiterMulticastHelper(Ptr<BasicSimulation> basicSimulation, Ptr<TopologyPtop> topology, const std::vector<MulticastUdpInfo> &multicast_reqs);
    private:
        // Parameters
        Ptr<BasicSimulation> m_basicSimulation;
        Ptr<TopologyPtop> m_topology;
        std::vector<MulticastUdpInfo> m_multicast_reqs;
        NodeContainer m_nodes;
        int64_t m_dynamicStateUpdateIntervalNs;
        std::vector<Ptr<ArbiterMulticast>> m_arbiters;
        std::vector<std::vector<uint32_t>> m_node_to_nbr_if_idx;

        // static std::vector<std::vector<std::vector<uint32_t>>> CalculateGlobalState(Ptr<TopologyPtop> topology);
        void testMulticast(Ptr<TopologyPtop> topology, Ptr<ArbiterMulticast> arbiterMulticast, uint32_t node_id);
        void ReadGlobalMulticastState();
        std::vector<std::list<Ipv4MulticastRoutingTableEntry*>> CalGlobalMulticastState(Ptr<TopologyPtop> topology);
    };

} // namespace ns3

#endif /* ARBITER_MULTICAST_HELPER_H */
