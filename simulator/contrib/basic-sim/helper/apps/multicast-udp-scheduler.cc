/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 ETH Zurich
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Simon
 * Originally based on the TcpFlowScheduler (which is authored by Simon, Hussain)
 */

#include "multicast-udp-scheduler.h"

namespace ns3 {
    std::vector<MulticastUdpInfo> MulticastUdpScheduler::GetMulticastReqs() {
        return m_schedule;
    }

    MulticastUdpScheduler::MulticastUdpScheduler(Ptr<BasicSimulation> basicSimulation, Ptr<Topology> topology) {
        printf("MULTICAST UDP SCHEDULER\n");

        m_basicSimulation = basicSimulation;
        m_topology = topology;

        // Check if it is enabled explicitly
        m_enabled = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_multicast_udp_scheduler", "false"));
        if (!m_enabled) {
            std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

        } else {
            std::cout << "  > Multicast UDP scheduler is enabled" << std::endl;

            // Properties we will use often
            m_nodes = m_topology->GetNodes();
            m_simulation_end_time_ns = m_basicSimulation->GetSimulationEndTimeNs();
            m_enable_logging_for_multicast_udp_ids = parse_set_positive_int64(m_basicSimulation->GetConfigParamOrDefault("multicast_udp_enable_logging_for_multicast_udp_ids", "set()"));

            // Distributed run information
            m_system_id = m_basicSimulation->GetSystemId();
            m_enable_distributed = m_basicSimulation->IsDistributedEnabled();
            m_distributed_node_system_id_assignment = m_basicSimulation->GetDistributedNodeSystemIdAssignment();

            // Read schedule
            m_schedule = read_multicast_udp_schedule(
                    m_basicSimulation->GetRunDir() + "/" + m_basicSimulation->GetConfigParamOrFail("multicast_udp_schedule_filename"),
                    m_topology,
                    m_simulation_end_time_ns
            );

            // Check that the UDP burst IDs exist in the logging
            for (int64_t udp_burst_id : m_enable_logging_for_multicast_udp_ids) {
                if ((size_t) udp_burst_id >= m_schedule.size()) {
                    throw std::invalid_argument("Invalid UDP burst ID in udp_burst_enable_logging_for_udp_burst_ids: " + std::to_string(udp_burst_id));
                }
            }

            // Schedule read
            printf("  > Read schedule (total Multicast UDP bursts: %lu)\n", m_schedule.size());
            m_basicSimulation->RegisterTimestamp("Read Multicast UDP schedule");

            // Determine filenames
            if (m_enable_distributed) {
                m_multicast_udp_outgoing_csv_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_multicast_udp_outgoing.csv";
                m_multicast_udp_outgoing_txt_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_multicast_udp_outgoing.txt";
                m_multicast_udp_incoming_csv_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_multicast_udp_incoming.csv";
                m_multicast_udp_incoming_txt_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_multicast_udp_incoming.txt";
            } else {
                m_multicast_udp_outgoing_csv_filename = m_basicSimulation->GetLogsDir() + "/multicast_udp_outgoing.csv";
                m_multicast_udp_outgoing_txt_filename = m_basicSimulation->GetLogsDir() + "/multicast_udp_outgoing.txt";
                m_multicast_udp_incoming_csv_filename = m_basicSimulation->GetLogsDir() + "/multicast_udp_incoming.csv";
                m_multicast_udp_incoming_txt_filename = m_basicSimulation->GetLogsDir() + "/multicast_udp_incoming.txt";
            }
            // Remove files if they are there
            remove_file_if_exists(m_multicast_udp_outgoing_csv_filename);
            remove_file_if_exists(m_multicast_udp_outgoing_txt_filename);
            remove_file_if_exists(m_multicast_udp_incoming_csv_filename);
            remove_file_if_exists(m_multicast_udp_incoming_txt_filename);
            printf("  > Removed previous Multicast UDP log files if present\n");
            m_basicSimulation->RegisterTimestamp("Remove previous Multicast UDP log files");

            // Install sink on each endpoint node
            std::cout << "  > Setting up Multicast UDP applications on all endpoint nodes" << std::endl;
            for (int64_t endpoint : m_topology->GetEndpoints()) {
                if (!m_enable_distributed || m_distributed_node_system_id_assignment[endpoint] == m_system_id) {

                    // Setup the application
                    MulticastUdpHelper multicastUdpHelper(3026, m_basicSimulation->GetLogsDir());
                    ApplicationContainer app = multicastUdpHelper.Install(m_nodes.Get(endpoint));
                    app.Start(Seconds(0.0));
                    m_apps.push_back(app);

                    // Register all bursts being sent from there and being received
                    Ptr<MulticastUdpApplication> multicastUdpApp = app.Get(0)->GetObject<MulticastUdpApplication>();
                    for (MulticastUdpInfo entry : m_schedule) {
                        std::set<int64_t> to_node_ids = entry.GetToNodeIds();
                        if (entry.GetFromNodeId() == endpoint) {
                            std::cout << "    >>> Register multicast outgoing at " << endpoint << std::endl;
                            multicastUdpApp->RegisterOutgoingBurst(
                                    entry,
                                    // InetSocketAddress(m_nodes.Get(entry.GetToNodeId())->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 1026),
                                    InetSocketAddress(entry.GetMulticastGroup(), 3026),
                                    m_enable_logging_for_multicast_udp_ids.find(entry.GetUdpBurstId()) != m_enable_logging_for_multicast_udp_ids.end()
                            );
                            m_responsible_for_outgoing_multicasts.push_back(std::make_pair(entry, multicastUdpApp));
                        }
                        if (to_node_ids.find(endpoint) != to_node_ids.end()) {
                            std::cout << "    >>> Register multicast incoming at " << endpoint << std::endl;
                            multicastUdpApp->RegisterIncomingBurst(
                                    entry,
                                    m_enable_logging_for_multicast_udp_ids.find(entry.GetUdpBurstId()) != m_enable_logging_for_multicast_udp_ids.end()
                            );
                            m_responsible_for_incoming_multicasts.push_back(std::make_pair(entry, multicastUdpApp));
                        }
                    }

                }
            }

            std::cout << "  > ByLul multicast app setup DONE!!!" << std::endl;
            
            m_basicSimulation->RegisterTimestamp("Setup Multicast UDP applications");

        }

        std::cout << std::endl;
    }

    void MulticastUdpScheduler::WriteResults() {
        std::cout << "STORE MULTICAST UDP RESULTS" << std::endl;

        // Check if it is enabled explicitly
        if (!m_enabled) {
            std::cout << "  > Not enabled, so no Multicast UDP results are written" << std::endl;

        } else {
            // Open files
            std::cout << "  > Opening UDP burst log files:" << std::endl;
            FILE* file_outgoing_csv = fopen(m_multicast_udp_outgoing_csv_filename.c_str(), "w+");
            std::cout << "    >> Opened: " << m_multicast_udp_outgoing_csv_filename << std::endl;
            FILE* file_outgoing_txt = fopen(m_multicast_udp_outgoing_txt_filename.c_str(), "w+");
            std::cout << "    >> Opened: " << m_multicast_udp_outgoing_txt_filename << std::endl;
            FILE* file_incoming_csv = fopen(m_multicast_udp_incoming_csv_filename.c_str(), "w+");
            std::cout << "    >> Opened: " << m_multicast_udp_incoming_csv_filename << std::endl;
            FILE* file_incoming_txt = fopen(m_multicast_udp_incoming_txt_filename.c_str(), "w+");
            std::cout << "    >> Opened: " << m_multicast_udp_incoming_txt_filename << std::endl;

            // Header
            std::cout << "  > Writing multicast_udp_{incoming, outgoing}.txt headers" << std::endl;
            fprintf(
                    file_outgoing_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-16s%-28s%-28s%s\n",
                    "UDP burst ID", "From", "MemNum", "Target rate", "Start time", "Duration",
                    "Outgoing rate (w/ headers)", "Outgoing rate (payload)", "Packets sent",
                    "Data sent (w/headers)", "Data sent (payload)", "Metadata"
            );
            fprintf(
                    file_incoming_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-19s%-28s%-28s%s\n",
                    "UDP burst ID", "From", "To", "Target rate", "Start time", "Duration",
                    "Incoming rate (w/ headers)", "Incoming rate (payload)", "Packets received",
                    "Data received (w/headers)", "Data received (payload)", "Metadata"
            );

            // Sort ascending to preserve UDP burst schedule order
            struct ascending_paired_udp_burst_id_key
            {
                inline bool operator() (const std::pair<MulticastUdpInfo, Ptr<MulticastUdpApplication>>& a, const std::pair<MulticastUdpInfo, Ptr<MulticastUdpApplication>>& b)
                {
                    return (a.first.GetUdpBurstId() < b.first.GetUdpBurstId());
                }
            };
            std::sort(m_responsible_for_outgoing_multicasts.begin(), m_responsible_for_outgoing_multicasts.end(), ascending_paired_udp_burst_id_key());
            std::sort(m_responsible_for_incoming_multicasts.begin(), m_responsible_for_incoming_multicasts.end(), ascending_paired_udp_burst_id_key());

            // Outgoing bursts
            std::cout << "  > Writing Multicast outgoing log files" << std::endl;
            for (std::pair<MulticastUdpInfo, Ptr<MulticastUdpApplication>> p : m_responsible_for_outgoing_multicasts) {
                MulticastUdpInfo info = p.first;
                Ptr<MulticastUdpApplication> udpBurstAppOutgoing = p.second;
                
                // Fetch data from the application
                uint32_t complete_packet_size = 1500;
                uint32_t max_udp_payload_size_byte = udpBurstAppOutgoing->GetMaxUdpPayloadSizeByte();
                uint64_t sent_counter = udpBurstAppOutgoing->GetSentCounterOf(info.GetUdpBurstId());

                // Calculate outgoing rate
                int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
                double rate_incl_headers_megabit_per_s = byte_to_megabit(sent_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
                double rate_payload_only_megabit_per_s = byte_to_megabit(sent_counter * max_udp_payload_size_byte) / nanosec_to_sec(effective_duration_ns);

                // Write plain to the CSV
                fprintf(
                        file_outgoing_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
                        info.GetUdpBurstId(), info.GetFromNodeId(), info.GetToNodeIds().size(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
                        info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, sent_counter,
                        sent_counter * complete_packet_size, sent_counter * max_udp_payload_size_byte, info.GetMetadata().c_str()
                );

                // Write nicely formatted to the text
                char str_target_rate[100];
                sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
                char str_start_time[100];
                sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
                char str_duration_ms[100];
                sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
                char str_eff_rate_incl_headers[100];
                sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
                char str_eff_rate_payload_only[100];
                sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
                char str_sent_incl_headers[100];
                sprintf(str_sent_incl_headers, "%.2f Mbit", byte_to_megabit(sent_counter * complete_packet_size));
                char str_sent_payload_only[100];
                sprintf(str_sent_payload_only, "%.2f Mbit", byte_to_megabit(sent_counter * max_udp_payload_size_byte));
                fprintf(
                        file_outgoing_txt,
                        "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-16" PRIu64 "%-28s%-28s%s\n",
                        info.GetUdpBurstId(),
                        info.GetFromNodeId(),
                        info.GetToNodeIds().size(),
                        str_target_rate,
                        str_start_time,
                        str_duration_ms,
                        str_eff_rate_incl_headers,
                        str_eff_rate_payload_only,
                        sent_counter,
                        str_sent_incl_headers,
                        str_sent_payload_only,
                        info.GetMetadata().c_str()
                );
            }

            // Incoming bursts
            std::cout << "  > Writing Multicast incoming log files" << std::endl;
            for (std::pair<MulticastUdpInfo, Ptr<MulticastUdpApplication>> p : m_responsible_for_incoming_multicasts) {
                MulticastUdpInfo info = p.first;
                Ptr<MulticastUdpApplication> udpBurstAppIncoming = p.second;
                int64_t dst_node_id = udpBurstAppIncoming->GetNode()->GetId();

                // Fetch data from the application
                uint32_t complete_packet_size = 1500;
                uint32_t max_udp_payload_size_byte = udpBurstAppIncoming->GetMaxUdpPayloadSizeByte();
                uint64_t received_counter = udpBurstAppIncoming->GetReceivedCounterOf(info.GetUdpBurstId());

                // Calculate incoming rate
                int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
                double rate_incl_headers_megabit_per_s = byte_to_megabit(received_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
                double rate_payload_only_megabit_per_s = byte_to_megabit(received_counter * max_udp_payload_size_byte) / nanosec_to_sec(effective_duration_ns);

                // Write plain to the CSV
                fprintf(
                        file_incoming_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
                        info.GetUdpBurstId(), info.GetFromNodeId(), dst_node_id, info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
                        info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, received_counter,
                        received_counter * complete_packet_size, received_counter * max_udp_payload_size_byte, info.GetMetadata().c_str()
                );

                // Write nicely formatted to the text
                char str_target_rate[100];
                sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
                char str_start_time[100];
                sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
                char str_duration_ms[100];
                sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
                char str_eff_rate_incl_headers[100];
                sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
                char str_eff_rate_payload_only[100];
                sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
                char str_received_incl_headers[100];
                sprintf(str_received_incl_headers, "%.2f Mbit", byte_to_megabit(received_counter * complete_packet_size));
                char str_received_payload_only[100];
                sprintf(str_received_payload_only, "%.2f Mbit", byte_to_megabit(received_counter * max_udp_payload_size_byte));
                fprintf(
                        file_incoming_txt,
                        "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-19" PRIu64 "%-28s%-28s%s\n",
                        info.GetUdpBurstId(),
                        info.GetFromNodeId(),
                        dst_node_id,
                        str_target_rate,
                        str_start_time,
                        str_duration_ms,
                        str_eff_rate_incl_headers,
                        str_eff_rate_payload_only,
                        received_counter,
                        str_received_incl_headers,
                        str_received_payload_only,
                        info.GetMetadata().c_str()
                );

            }

            // Close files
            std::cout << "  > Closing UDP burst log files:" << std::endl;
            fclose(file_outgoing_csv);
            std::cout << "    >> Closed: " << m_multicast_udp_outgoing_csv_filename << std::endl;
            fclose(file_outgoing_txt);
            std::cout << "    >> Closed: " << m_multicast_udp_outgoing_txt_filename << std::endl;
            fclose(file_incoming_csv);
            std::cout << "    >> Closed: " << m_multicast_udp_incoming_csv_filename << std::endl;
            fclose(file_incoming_txt);
            std::cout << "    >> Closed: " << m_multicast_udp_incoming_txt_filename << std::endl;

            std::cout << "  > ByLul multicast log write!!!" << std::endl;
            // Register completion
            std::cout << "  > Multicast UDP log files have been written" << std::endl;
            m_basicSimulation->RegisterTimestamp("Write Multicast UDP log files");

        }

        std::cout << std::endl;
    }

}