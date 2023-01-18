#ifndef ARBITER_MULTICAST_HELPER_H
#define ARBITER_MULTICAST_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-ptop.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/arbiter-multicast.h"
#include "ns3/arbiter-ecmp-helper.h"

namespace ns3 {
    class Ipv4MulticastRoutingTableEntry;
    class ArbiterEcmpHelper;

    class ArbiterMulticastHelper : public ArbiterEcmpHelper
    {
    public:
        static void InstallArbiters (Ptr<BasicSimulation> basicSimulation, Ptr<TopologyPtop> topology);
    private:
        // static std::vector<std::vector<std::vector<uint32_t>>> CalculateGlobalState(Ptr<TopologyPtop> topology);
        static void testMulticast(Ptr<TopologyPtop> topology, Ptr<ArbiterMulticast> arbiterMulticast, uint32_t node_id);
        static std::vector<std::list<Ipv4MulticastRoutingTableEntry*>> ReadGlobalMulticastState(Ptr<TopologyPtop> topology);
        static std::vector<std::list<Ipv4MulticastRoutingTableEntry*>> CalGlobalMulticastState(Ptr<TopologyPtop> topology);
    };

} // namespace ns3

#endif /* ARBITER_MULTICAST_HELPER_H */
