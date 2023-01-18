#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"

namespace ns3 {

class Topology : public Object
{
public:
    static TypeId GetTypeId (void);
    virtual const NodeContainer& GetNodes() = 0;
    virtual int64_t GetNumNodes() = 0;
    virtual bool IsValidEndpoint(int64_t node_id) = 0;
    virtual const std::set<int64_t>& GetEndpoints() = 0;
    Ipv4Address GetMulticastGroupBase();
    void SetMulticastGroupBase(Ipv4Address multicast_group_base);
private:
    Ipv4Address m_multicast_group_base = Ipv4Address("225.1.2.4");
};
}

#endif //TOPOLOGY_H
