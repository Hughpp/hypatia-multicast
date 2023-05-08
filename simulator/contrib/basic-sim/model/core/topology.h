#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/id-seq-bier-header.h"

namespace ns3 {

class Topology : public Object
{
public:
    static TypeId GetTypeId (void);
    virtual const NodeContainer& GetNodes() = 0;
    virtual int64_t GetNumNodes() = 0;
    virtual bool IsValidEndpoint(int64_t node_id) = 0;
    virtual const std::set<int64_t>& GetEndpoints() = 0;
    virtual int NodeidToBP(uint32_t node_id);
    Ipv4Address GetMulticastGroupBase();
    // Ipv4Address GetMulticastBIERBase();
    // void SetMulticastGroupBase(Ipv4Address multicast_group_base);
private:
    // Ipv4Address m_multicast_group_base = Ipv4Address("225.1.2.4");
    // Ipv4Address m_multicast_bier_base = Ipv4Address("233.0.0.0");
};
}

#endif //TOPOLOGY_H
