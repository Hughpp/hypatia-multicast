#include "topology.h"

namespace ns3 {
void Topology::SetMulticastGroupBase(Ipv4Address multicast_group_base){
    m_multicast_group_base = multicast_group_base;
}

Ipv4Address Topology::GetMulticastGroupBase(){
    return m_multicast_group_base;
}
}

