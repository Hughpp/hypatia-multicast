#include "multicast-udp-schedule-reader.h"

namespace ns3 {

    MulticastUdpInfo::MulticastUdpInfo(
            int64_t udp_burst_id,
            int64_t from_node_id,
            std::set<int64_t> to_node_ids,
            double target_rate_megabit_per_s,
            int64_t start_time_ns,
            int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    ) {
        m_udp_burst_id = udp_burst_id;
        m_from_node_id = from_node_id;
        m_to_node_ids = to_node_ids;
        m_target_rate_megabit_per_s = target_rate_megabit_per_s;
        m_start_time_ns = start_time_ns;
        m_duration_ns = duration_ns;
        m_additional_parameters = additional_parameters;
        m_metadata = metadata;
    }

    int64_t MulticastUdpInfo::GetUdpBurstId() const {
        return m_udp_burst_id;
    }

    int64_t MulticastUdpInfo::GetFromNodeId() {
        return m_from_node_id;
    }

    std::set<int64_t>& MulticastUdpInfo::GetToNodeIds() {
        return m_to_node_ids;
    }

    double MulticastUdpInfo::GetTargetRateMegabitPerSec() {
        return m_target_rate_megabit_per_s;
    }

    int64_t MulticastUdpInfo::GetStartTimeNs() {
        return m_start_time_ns;
    }

    int64_t MulticastUdpInfo::GetDurationNs() {
        return m_duration_ns;
    }

    std::string MulticastUdpInfo::GetAdditionalParameters() {
        return m_additional_parameters;
    }

    std::string MulticastUdpInfo::GetMetadata() {
        return m_metadata;
    }

    Ipv4Address MulticastUdpInfo::GetMulticastGroup() {
        return m_multicast_group;
    }

    void MulticastUdpInfo::SetMulticastGroup(Ipv4Address multicast_group) {
        m_multicast_group = multicast_group;
    }

    /**
     * Read in the UDP burst schedule.
     *
     * @param filename                  File name of the udp_burst_schedule.csv
     * @param topology                  Topology
     * @param simulation_end_time_ns    Simulation end time (ns) : all UDP bursts must start less than this value
    */
    std::vector<MulticastUdpInfo> read_multicast_udp_schedule(const std::string& filename, Ptr<Topology> topology, const int64_t simulation_end_time_ns) {

        // Schedule to put in the data
        std::vector<MulticastUdpInfo> schedule;
        Ipv4Address multicast_group_base = topology->GetMulticastGroupBase();

        // Check that the file exists
        if (!file_exists(filename)) {
            throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
        }

        // Open file
        std::string line;
        std::ifstream schedule_file(filename);
        if (schedule_file) {

            // Go over each line
            size_t line_counter = 0;
            int64_t prev_start_time_ns = 0;
            while (getline(schedule_file, line)) {

                // Split on ,
                std::vector<std::string> comma_split = split_string(line, ",", 9);

                // Fill entry
                int64_t udp_burst_id = parse_positive_int64(comma_split[0]);
                if (udp_burst_id != (int64_t) line_counter) {
                    throw std::invalid_argument(format_string("UDP burst ID is not ascending by one each line (violation: %" PRId64 ")\n", udp_burst_id));
                }
                int64_t from_node_id = parse_positive_int64(comma_split[1]);
                //multicast change
                int64_t num_mem = parse_positive_int64(comma_split[2]);
                std::string to_node_ids_str = comma_split[3];
                std::set<int64_t> to_node_ids;
                std::vector<std::string> space_split = split_string(to_node_ids_str, " ", num_mem);
                for(int i = 0; i < num_mem; ++i){ //update set
                    int64_t cur_node_id = parse_positive_double(space_split[i]);
                    to_node_ids.insert(cur_node_id);
                }
                Ipv4Address multicast_group = Ipv4Address(multicast_group_base.Get()+udp_burst_id);
                //multicast change end
                double target_rate_megabit_per_s = parse_positive_double(comma_split[4]);
                int64_t start_time_ns = parse_positive_int64(comma_split[5]);
                int64_t duration_ns = parse_positive_int64(comma_split[6]);
                std::string additional_parameters = comma_split[7];
                std::string metadata = comma_split[8];

                // Zero target rate
                if (target_rate_megabit_per_s == 0.0) {
                    throw std::invalid_argument("Multicast UDP target rate is zero.");
                }

                // Must be weakly ascending start time
                if (prev_start_time_ns > start_time_ns) {
                    throw std::invalid_argument(format_string("Start time is not weakly ascending (on line with multicast UDP ID: %" PRId64 ", violation: %" PRId64 ")\n", udp_burst_id, start_time_ns));
                }
                prev_start_time_ns = start_time_ns;

                // Check node IDs
                if (to_node_ids.find(from_node_id) != to_node_ids.end()) {
                    throw std::invalid_argument(format_string("Multicast UDP (ID: %" PRId64 ") to itself (src as a member) at node ID: %" PRId64 ".", udp_burst_id, from_node_id));
                }

                // Check endpoint validity
                if (!topology->IsValidEndpoint(from_node_id)) {
                    throw std::invalid_argument(format_string("Invalid from-endpoint for a schedule entry based on topology: %d", from_node_id));
                }
                for (int64_t cur_node_id :to_node_ids){
                    if (!topology->IsValidEndpoint(cur_node_id)) {
                        throw std::invalid_argument(format_string("Invalid to-endpoint for a schedule entry based on topology: %d", cur_node_id));
                    }
                }

                // Check start time
                if (start_time_ns >= simulation_end_time_ns) {
                    throw std::invalid_argument(format_string(
                            "UDP burst %" PRId64 " has invalid start time %" PRId64 " >= %" PRId64 ".",
                            udp_burst_id, start_time_ns, simulation_end_time_ns
                    ));
                }

                // Put into schedule
                MulticastUdpInfo entry = MulticastUdpInfo(udp_burst_id, from_node_id, to_node_ids, target_rate_megabit_per_s, start_time_ns, duration_ns, additional_parameters, metadata);
                entry.SetMulticastGroup(multicast_group);
                schedule.push_back(entry);
                //check if correct
                std::cout << "   Add a multicast udp flow: id " << entry.GetUdpBurstId() << " multicast_group " << entry.GetMulticastGroup() << " src " << entry.GetFromNodeId() << " dsts ";
                for (int64_t dst :to_node_ids){
                    std::cout << dst << ",";
                }
                std::cout << std::endl;

                // Next line
                line_counter++;

            }

            // Close file
            schedule_file.close();

        } else {
            throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
        }

        return schedule;

    }

}