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

ArbiterSatMulticast::ArbiterSatMulticast(
        Ptr<Node> this_node,
        NodeContainer nodes,
        std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list
) : ArbiterSingleForward(this_node, nodes, next_hop_list) {}

void ArbiterSatMulticast::AddMulticastRoute(Ipv4Address origin, Ipv4Address group, uint32_t inputInterface, std::vector<uint32_t> outputInterfaces) {
    //print mutlicast route
    std::cout << " > Adding a multicast route for node " << ArbiterSatMulticast::m_node_id;
    std::cout << "  >>> " << origin << " " << group << " " << inputInterface << " outputIf:";
    for(int32_t oif: outputInterfaces){
        std::cout << " " << oif;
    }
    std::cout << std::endl;

    Ipv4MulticastRoutingTableEntry *route = new Ipv4MulticastRoutingTableEntry ();
    *route = Ipv4MulticastRoutingTableEntry::CreateMulticastRoute (origin, group, inputInterface, outputInterfaces);
    m_multicast_routes.push_back(route);
}

void ArbiterSatMulticast::SetMulticastRoutes(std::list<Ipv4MulticastRoutingTableEntry *> multicast_routes) {
    m_multicast_routes = multicast_routes;
}

void ArbiterSatMulticast::ClearMulticastRoutes() {
    m_multicast_routes.clear();
}

ArbiterResult ArbiterSatMulticast::DecideMulticast(int32_t source_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    //look up without input interface, in other words, interface is IF_ANY
    Ipv4Address group = ipHeader.GetDestination();
    Ipv4Address origin = ipHeader.GetSource();
    int32_t origin_id = source_node_id;
    uint32_t interface = Ipv4::IF_ANY;

    NS_LOG_FUNCTION (this << origin << " " << group << " " << interface);

    Ptr<Ipv4MulticastRoute> mrtentry = 0;
    // bool foundRoute = false;
    for (auto i = m_multicast_routes.begin (); 
        i != m_multicast_routes.end (); 
        i++) {
        Ipv4MulticastRoutingTableEntry *route = *i;
        //
        // We've been passed an origin address, a multicast group address and an 
        // interface index.  We have to decide if the current route in the list is
        // a match.
        //
        // The first case is the restrictive case where the origin, group and index
        // matches.
        //
        if (origin == route->GetOrigin () && group == route->GetGroup ()) {
            // Skipping this case (SSM) for now
            NS_LOG_LOGIC ("Found multicast source specific route" << *i);
        }
        if (group == route->GetGroup ()) {
            if (interface == Ipv4::IF_ANY || interface == route->GetInputInterface ()) {
                NS_LOG_LOGIC ("Found multicast route" << *i);
                ArbiterResult result = ArbiterResult(false, 0, 0);
                result.SetIsMulticast(true);
                if(origin_id == m_node_id) { //outbound packet, turns to unicast
                    result.SetIsMulticastOutbound(true);
                    if (route->GetNOutputInterfaces() != 1){
                        throw std::runtime_error("outbound multicast route num of out if != 1");
                    }
                    uint32_t out_if = route->GetOutputInterface(0);
                    result.SetOutIfIdx(out_if);
                }
                else{ //multicast forwarding
                    std::vector<uint32_t> out_if_idxs;
                    for (uint32_t j = 0; j < route->GetNOutputInterfaces (); j++) {
                        if (route->GetOutputInterface (j)) {
                            NS_LOG_LOGIC ("Found output interface index " << route->GetOutputInterface (j));
                            out_if_idxs.push_back(route->GetOutputInterface(j));
                        }
                    }
                    result.SetOutIfIdxs(out_if_idxs);
                }
                return result;
            }
        }
    }
    std::cout << "    ArbiterSatMulticast at node " << m_node_id << ": not found multicast route for forwarding" << std::endl;
    return ArbiterResult(true, 0, 0);
}

// uint32_t ArbiterSatMulticast::GetBpFromNodeID(uint32_t node_id) {
//     return 1000;
// }

BIERTableEntry& ArbiterSatMulticast::LookupBIERTable(int bp) {
    for (auto i = m_bier_table.begin (); i != m_bier_table.end (); i++) {
        BIERTableEntry *route = *i;
        if(route->GetBFERBP() == bp) { //find the same bp
            return *route;
        }
    }
    std::cout << "The bp not found: " << bp << std::endl;
    throw std::runtime_error("ArbiterSatMulticast::BIER table lookup failed");
}

void ArbiterSatMulticast::ClearBIERRoutes() {
    m_bier_table.clear();
}

void ArbiterSatMulticast::AddBIERRoute(int bfer_bp, uint32_t fbm[BS_LEN_32], int nexthop_bp, uint32_t bfer_id, uint32_t oifidx, uint32_t bfer_addr, uint32_t nxthop_addr) {
    // //print mutlicast route
    // std::cout << " > Adding a multicast route for node " << ArbiterSatMulticast::m_node_id;
    // std::cout << "  >>> " << origin << " " << group << " " << inputInterface << " outputIf:";
    // for(int32_t oif: outputInterfaces){
    //     std::cout << " " << oif;
    // }
    // std::cout << std::endl;
    BIERTableEntry *route = new BIERTableEntry(bfer_bp, fbm, nexthop_bp, bfer_id, oifidx, bfer_addr, nxthop_addr);
    m_bier_table.push_back(route);
}

}

