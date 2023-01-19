#ifndef MULTICAST_UDP_SCHEDULE_READER_H
#define MULTICAST_UDP_SCHEDULE_READER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>

#include "ns3/exp-util.h"
#include "ns3/topology.h"

namespace ns3 {

    class MulticastUdpInfo
    {
    public:
        MulticastUdpInfo(
                int64_t udp_burst_id,
                int64_t from_node_id,
                // int64_t to_node_id,
                std::set<int64_t> to_node_ids,
                double target_rate_megabit_per_s,
                int64_t start_time_ns,
                int64_t duration_ns,
                std::string additional_parameters,
                std::string metadata
        );
        int64_t GetUdpBurstId() const;
        int64_t GetFromNodeId();
        std::set<int64_t>& GetToNodeIds();
        // int64_t GetToNodeId();
        double GetTargetRateMegabitPerSec();
        int64_t GetStartTimeNs();
        int64_t GetDurationNs();
        std::string GetAdditionalParameters();
        std::string GetMetadata();
        Ipv4Address GetMulticastGroup();
        void SetMulticastGroup(Ipv4Address multicast_group);
    private:
        int64_t m_udp_burst_id;
        int64_t m_from_node_id;
        std::set<int64_t> m_to_node_ids;
        // int64_t m_to_node_id;
        double m_target_rate_megabit_per_s;
        int64_t m_start_time_ns;
        int64_t m_duration_ns;
        std::string m_additional_parameters;
        std::string m_metadata;
        Ipv4Address m_multicast_group;
    };

    std::vector<MulticastUdpInfo> read_multicast_udp_schedule(
        const std::string& filename,
        Ptr<Topology> topology,
        const int64_t simulation_end_time_ns
    );
}

#endif /* MULTICAST_UDP_SCHEDULE_READER_H */
