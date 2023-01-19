#include "arbiter-sat-multicast.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ArbiterSatMulticast);
TypeId ArbiterSatMulticast::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ArbiterSatMulticast")
            .SetParent<ArbiterSingleForward> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

ArbiterResult ArbiterSatMulticast::DecideMulticast(int32_t source_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    throw std::runtime_error("not implement");
}

}

