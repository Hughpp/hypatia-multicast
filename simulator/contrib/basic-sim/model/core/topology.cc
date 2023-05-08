#include "topology.h"

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (Topology);
TypeId Topology::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Topology")
            .SetParent<Object> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

int Topology::NodeidToBP(uint32_t node_id) {
    throw std::runtime_error("no implement");
}

// void Topology::SetMulticastGroupBase(Ipv4Address multicast_group_base){
//     m_multicast_group_base = multicast_group_base;
// }

Ipv4Address Topology::GetMulticastGroupBase(){
    if (USE_BIER) {
        return Ipv4Address("233.0.0.0");
    }
    else {
        return Ipv4Address("225.1.2.4");
    }
    // return m_multicast_group_base;
}

// Ipv4Address Topology::GetMulticastBIERBase() {
//     return m_multicast_bier_base;
// }

}

