#include "arbiter.h"

namespace ns3 {

// Arbiter result

ArbiterResult::ArbiterResult(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address) {
    m_failed = failed;
    m_out_if_idx = out_if_idx;
    m_gateway_ip_address = gateway_ip_address;
}

bool ArbiterResult::Failed() {
    return m_failed;
}

uint32_t ArbiterResult::GetOutIfIdx() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve out interface index if the arbiter did not succeed in finding a next hop");
    }
    return m_out_if_idx;
}

bool ArbiterResult::IsMulticast() {
    return m_is_multicast;
}

bool ArbiterResult::IsMulticastOutbound() {
    return m_is_multicast_outbound;
}

std::vector<uint32_t> ArbiterResult::GetOutIfIdxMulticast() {
    if (m_is_multicast) {
        return m_out_if_idxs;
    }
    else {
        throw std::runtime_error("Cannot retrieve out interface indexs if the arbiter did not succeed in finding a multicast route");
    }
}

void ArbiterResult::SetIsMulticast(bool is_multicast) {
    m_is_multicast = is_multicast;
}

void ArbiterResult::SetIsMulticastOutbound(bool is_multicast_outbound) {
    m_is_multicast_outbound = is_multicast_outbound;
}

void ArbiterResult::SetOutIfIdxs(std::vector<uint32_t> out_if_idxs) {
    m_out_if_idxs = out_if_idxs;
}

void ArbiterResult::SetOutIfIdx(uint32_t out_if_idx) {
    m_out_if_idx = out_if_idx;
}

uint32_t ArbiterResult::GetGatewayIpAddress() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve gateway IP address if the arbiter did not succeed in finding a next hop");
    }
    return m_gateway_ip_address;
}

// Arbiter

NS_OBJECT_ENSURE_REGISTERED (Arbiter);
TypeId Arbiter::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Arbiter")
            .SetParent<Object> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes) {
    m_node_id = this_node->GetId();
    m_nodes = nodes;

    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        }
    }

}

uint32_t Arbiter::ResolveNodeIdFromIp(uint32_t ip) {
    m_ip_to_node_id_it = m_ip_to_node_id.find(ip);
    if (m_ip_to_node_id_it != m_ip_to_node_id.end()) {
        return m_ip_to_node_id_it->second;
    } else {
        std::ostringstream res;
        res << "IP address " << Ipv4Address(ip)  << " (" << ip << ") is not mapped to a node id";
        throw std::invalid_argument(res.str());
    }
}

ArbiterResult Arbiter::DecideMulticast(int32_t source_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    throw std::runtime_error("Multicast not supported in this arbiter");
}

uint32_t Arbiter::GetBpFromNodeID(uint32_t node_id) {
    throw std::runtime_error("BIER not supported in this arbiter");
}

BIERTableEntry& Arbiter::LookupBIERTable(int bp) {
    throw std::runtime_error("BIER not supported in this arbiter");
}

int Arbiter::GetBP() {
    throw std::runtime_error("BIER not supported in this arbiter");
}

ArbiterResult Arbiter::BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    //handle src
    // Retrieve the source node id
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t source_node_id;

    // Ipv4Address default constructor has IP 0x66666666 = 102.102.102.102 = 1717986918,
    // which is set by TcpSocketBase::SetupEndpoint to discover its actually source IP.
    bool is_socket_request_for_source_ip = source_ip == 1717986918;

    // If it is a request for source IP, the source node id is just the current node.
    if (is_socket_request_for_source_ip) {
        source_node_id = m_node_id;
    } else {
        source_node_id = ResolveNodeIdFromIp(source_ip);
    }

    //handle dst
    Ipv4Address dest_addr = ipHeader.GetDestination();
    if(dest_addr.IsMulticast()) {
        //multicast routing: outbound and forward
        return DecideMulticast(source_node_id, pkt, ipHeader);
    }
    uint32_t target_node_id = ResolveNodeIdFromIp(dest_addr.Get());

    // Decide the next node
    return Decide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_socket_request_for_source_ip
    );

}

}
