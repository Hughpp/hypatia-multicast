#include "arbiter-multicast-helper.h"

namespace ns3 {

void ArbiterMulticastHelper::InstallArbiters (Ptr<BasicSimulation> basicSimulation, Ptr<TopologyPtop> topology) {
    std::cout << "SETUP MULTICAST ROUTING" << std::endl;

    NodeContainer nodes = topology->GetNodes();

    // Calculate and instantiate the routing
    std::cout << "  > ArbiterMulticastHelper:Calculating ECMP routing for unicast" << std::endl;
    std::vector<std::vector<std::vector<uint32_t>>> global_ecmp_state = CalculateGlobalState(topology);
    basicSimulation->RegisterTimestamp("ArbiterMulticastHelper Calculate ECMP routing state");

    std::cout << "  > Setting the routing arbiter on each node" << std::endl;
    for (int i = 0; i < topology->GetNumNodes(); i++) {
        Ptr<ArbiterMulticast> arbiterMulticast = CreateObject<ArbiterMulticast>(nodes.Get(i), nodes, topology, global_ecmp_state[i]);
        testMulticast(topology, arbiterMulticast, i);
        nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbiterRouting>()->SetArbiter(arbiterMulticast);
    }
    basicSimulation->RegisterTimestamp("Setup routing arbiter on each node");

    std::cout << std::endl;
}

void ArbiterMulticastHelper::testMulticast(Ptr<TopologyPtop> topology, Ptr<ArbiterMulticast> arbiterMulticast, uint32_t node_id) {
    //testing: static routing
    NodeContainer nodes = topology->GetNodes();
    std::cout << "  > Setting the multicast routing state on node " << node_id << std::endl;
    //init
    Ipv4Address origin = nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); //get local addr
    Ipv4Address group = topology->GetMulticastGroupBase(); //req id 0
    std::cout << "  > origin " << origin << std::endl;
    std::cout << "  > group " << group << std::endl;
    if(node_id == 0){
        // node 0: outbound
        std::vector<uint32_t> out_if_ids = {arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[1]};
        arbiterMulticast->AddMulticastRoute(origin, group, 0, out_if_ids);
    }
    if(node_id == 1){
        // node 1: forward
        std::vector<uint32_t> out_if_ids = {arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[2], arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[3]};
        arbiterMulticast->AddMulticastRoute(origin, group, 0, out_if_ids);
    }
    
}

std::vector<std::list<Ipv4MulticastRoutingTableEntry*>> ArbiterMulticastHelper::ReadGlobalMulticastState(Ptr<TopologyPtop> topology) {
    throw std::runtime_error("not implement");
}

std::vector<std::list<Ipv4MulticastRoutingTableEntry*>> ArbiterMulticastHelper::CalGlobalMulticastState(Ptr<TopologyPtop> topology) {
    throw std::runtime_error("not implement");
}

} // namespace ns3

