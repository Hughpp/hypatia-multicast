#include "arbiter-sat-multicast-helper.h"

#define ROUTING_HELPER_PRINT true

namespace ns3 {

    ArbiterSatMulticastHelper::ArbiterSatMulticastHelper(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology, const std::vector<MulticastUdpInfo> &multicast_reqs) {
        std::cout << "SETUP SAT UNICAST AND MULTICAST ROUTING" << std::endl;
        m_basicSimulation = basicSimulation;
        m_nodes = topology->GetNodes();
        m_multicast_reqs = multicast_reqs;
        m_topology = topology;

        // Read in initial forwarding state
        std::cout << "  > Create initial routing state" << std::endl;
        InitialGlobalRoutingState();
        basicSimulation->RegisterTimestamp("Create initial routing state");

        // Set the routing arbiters
        std::cout << "  > Setting the sat-multicast-arbiter on each node" << std::endl;
        for (size_t i = 0; i < m_nodes.GetN(); i++) {
            Ptr<ArbiterSatMulticast> arbiter = CreateObject<ArbiterSatMulticast>(m_nodes.Get(i), m_nodes, m_globalForwardingState[i]);
            m_arbiters.push_back(arbiter);
            m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbiterRouting>()->SetArbiter(arbiter);
        }
        basicSimulation->RegisterTimestamp("Setup sat-multicast-arbiter on each node");

        // Load first forwarding state
        m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
        std::cout << "  > Forward state update interval: " << m_dynamicStateUpdateIntervalNs << "ns" << std::endl;
        std::cout << "  > Perform first routing state load for t=0" << std::endl;

        // UpdateForwardingState(0);
        UpdateGlobalRoutingState(0);
        basicSimulation->RegisterTimestamp("Create initial single forwarding state");

        std::cout << std::endl;

    }

    void ArbiterSatMulticastHelper::InitialEmptyMulticastState() {
        //multicast table (list) is empty by default ( empty list )
    }

    void ArbiterSatMulticastHelper::UpdateMulticastState(int64_t t) {
        //update multicast state according to unicast state saved at m_globalForwardingState

        //first, clear all the old multicast table
        std::set<Mac48Address> multicastMacsToDel; //here just record old mac, del them after new mac updated
        for (auto oldMac: m_lastMulticastMacs) { //mac
            multicastMacsToDel.insert(oldMac);
        }
        m_lastMulticastMacs.clear();
        for (size_t i = 0; i < m_nodes.GetN(); i++) { //ip
            m_arbiters[i]->ClearMulticastRoutes();
        }
        
        //second, add new state
        std::cout << "  > Calculating multicast route from m_globalForwardingState" << std::endl;
        for (MulticastUdpInfo req : m_multicast_reqs) {
            //check time
            if (req.GetStartTimeNs() > t+m_dynamicStateUpdateIntervalNs || req.GetStartTimeNs()+req.GetDurationNs() < t) {
                continue;
            }
            //cal oifs for every node separately
            std::set<uint32_t> nodes_ids_to_install; //the node to add multicast routes
            std::map<uint32_t, std::set<uint32_t>> node_id_to_oifs; //node id to out ifs(route info)
            std::map<uint32_t, uint32_t> node_id_to_iif; //node id to in ifs(route info)
            std::set<std::pair<Mac48Address, Ptr<GSLNetDevice>>> mac_toset; //record logic mac to set
            int64_t src_id = req.GetFromNodeId();
            std::set<int64_t> dst_ids = req.GetToNodeIds();
            uint32_t cur_node_id, nxt_node_id, cur_oif_id=0, nxt_iif_id=0;
            int64_t nxthop_of_src = -1;
            Ipv4Address origin = m_nodes.Get(src_id)->GetObject<Ipv4>()->GetAddress(*node_id_to_oifs[src_id].begin(), 0).GetLocal();
            Ipv4Address group = Ipv4Address(m_topology->GetMulticastGroupBase().Get() + req.GetUdpBurstId());
            for (auto dst_id : dst_ids) {
                cur_node_id = src_id;
                while (cur_node_id != (uint32_t)dst_id) {
                    std::tuple<int32_t, int32_t, int32_t> nexthop_myif_nxtif = m_globalForwardingState[cur_node_id][dst_id];// get unicast entry
                    nxt_node_id = std::get<0>(nexthop_myif_nxtif); 
                    cur_oif_id = std::get<1>(nexthop_myif_nxtif);
                    nxt_iif_id = std::get<2>(nexthop_myif_nxtif);
                    nodes_ids_to_install.insert(cur_node_id);
                    //src single nexthop check
                    if (cur_node_id == src_id){
                        if (nxthop_of_src != -1 && nxthop_of_src != nxt_node_id) throw std::runtime_error("MulticastRoutingHelper: Src oif num != 1(not supported by ns3 routing protocol)");
                        nxthop_of_src = (int64_t)nxt_node_id;
                    }
                    // std::cout << "  ******* src_id=" << src_id << " dst_id=" << dst_id << " cur_node=" << cur_node_id << "-oif=" << cur_oif_id << " nxthop_id=" << nxt_node_id << "-iif=" << nxt_iif_id << std::endl;
                    if (node_id_to_oifs.find(cur_node_id) != node_id_to_oifs.end()) { //cur_node_id has been recorded
                        node_id_to_oifs[cur_node_id].insert(cur_oif_id); // add oif id to set
                    }
                    else {
                        node_id_to_oifs.insert({cur_node_id, {cur_oif_id}}); // init a set
                    }
                    node_id_to_iif[nxt_node_id] = nxt_iif_id;
                    //handel mac
                    if ((m_topology->IsSatelliteId(cur_node_id) && m_topology->IsGroundStationId(nxt_node_id)) || 
                    (m_topology->IsGroundStationId(cur_node_id) && m_topology->IsSatelliteId(nxt_node_id))) { //sat<->gs 
                        Ptr<GSLNetDevice> tar_dev = DynamicCast<GSLNetDevice>(m_nodes.Get(nxt_node_id)->GetDevice(nxt_iif_id));
                        Ptr<GSLNetDevice> src_dev = DynamicCast<GSLNetDevice>(m_nodes.Get(cur_node_id)->GetDevice(cur_oif_id));
                        mac_toset.insert(std::make_pair(Mac48Address::ConvertFrom(src_dev->GetMulticast(group)), tar_dev));
                    }
                    cur_node_id = nxt_node_id; //hop to next
                }
            }
            if (node_id_to_oifs[src_id].size() != 1) {
                throw std::runtime_error("MulticastRoutingHelper: Src oif num != 1(not supported by ns3 routing protocol)");
            }
            
            std::cout << "  > Set Multicast mac and ip table for req=" << req.GetUdpBurstId() << "  addr:" << group << std::endl;
            //set multicast logic mac
            for (auto p: mac_toset) {
                Mac48Address local_mac = p.first;
                Ptr<GSLNetDevice> tar_dev = p.second;
                std::cout << "    > MulticastSetLogicMac local mac=" << local_mac << "  toNode=" << tar_dev->GetNode()->GetId() << std::endl;
                m_topology->m_gslChannel->SetLogicLink(local_mac, tar_dev); 
                m_lastMulticastMacs.insert(local_mac);
                //update macToDel
                if(multicastMacsToDel.find(local_mac) != multicastMacsToDel.end()) { 
                    multicastMacsToDel.erase(local_mac);
                }
            }
            //add multicast route for this req
            uint32_t iif_id;
            for (auto node_id_to_install : nodes_ids_to_install) {
                if (node_id_to_install == src_id) {
                    iif_id = 0;
                }
                else {
                    iif_id = node_id_to_iif[node_id_to_install]; // use 0 for simplicity
                }
                std::vector<uint32_t> out_if_ids(node_id_to_oifs[node_id_to_install].begin(), node_id_to_oifs[node_id_to_install].end()); //trans set to vector
                m_arbiters[node_id_to_install]->AddMulticastRoute(origin, group, iif_id, out_if_ids);
            }
        }

        //acturally del the out-dated mac
        for (auto oldMac: multicastMacsToDel) { //mac
            //del after 10ms
            Simulator::Schedule(NanoSeconds(t+10000000), &GSLChannel::DelLogicLink, m_topology->m_gslChannel, oldMac); //delayed del mac
            // m_topology->m_gslChannel->DelLogicLink(oldMac);
        }
    }

    void ArbiterSatMulticastHelper::InitialGlobalRoutingState() {
        //unicast
        InitialEmptyForwardingState();

        //multicast
        InitialEmptyMulticastState();
    }

    void ArbiterSatMulticastHelper::UpdateGlobalRoutingState(int64_t t) {
        //unicast update
        UpdateForwardingState(t);
        std::cout << "    > Forwarding state update completed for t=" << t << std::endl;

        //multicast update
        UpdateMulticastState(t);
        std::cout << "    > Multicast state update completed for t=" << t << std::endl;

        //timer
        if (!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("satellite_network_force_static", "false"))) {
            // Plan the next update
            int64_t next_update_ns = t + m_dynamicStateUpdateIntervalNs;
            if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
                Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs), &ArbiterSatMulticastHelper::UpdateGlobalRoutingState, this, next_update_ns);
            }
        }
    }

    void ArbiterSatMulticastHelper::InitialEmptyForwardingState() {
        for (size_t i = 0; i < m_nodes.GetN(); i++) {
            std::vector <std::tuple<int32_t, int32_t, int32_t>> next_hop_list;
            for (size_t j = 0; j < m_nodes.GetN(); j++) {
                next_hop_list.push_back(std::make_tuple(-2, -2, -2)); // -2 indicates an invalid entry
            }
            m_globalForwardingState.push_back(next_hop_list);
        }
    }

    void ArbiterSatMulticastHelper::UpdateForwardingState(int64_t t) {
        // Filename
        std::ostringstream res;
        res << m_basicSimulation->GetRunDir() << "/";
        res << m_basicSimulation->GetConfigParamOrFail("satellite_network_routes_dir") << "/fstate_" << t << ".txt";
        std::string filename = res.str();

        // Check that the file exists
        if (!file_exists(filename)) {
            throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
        }

        // Open file
        std::string line;
        std::ifstream fstate_file(filename);
        if (fstate_file) {

            // Go over each line
            size_t line_counter = 0;
            while (getline(fstate_file, line)) {

                // Split on ,
                std::vector<std::string> comma_split = split_string(line, ",", 5);

                // Retrieve identifiers
                int64_t current_node_id = parse_positive_int64(comma_split[0]);
                int64_t target_node_id = parse_positive_int64(comma_split[1]);
                int64_t next_hop_node_id = parse_int64(comma_split[2]);
                int64_t my_if_id = parse_int64(comma_split[3]);
                int64_t next_if_id = parse_int64(comma_split[4]);

                // Check the node identifiers
                NS_ABORT_MSG_IF(current_node_id < 0 || current_node_id >= m_nodes.GetN(), "Invalid current node id.");
                NS_ABORT_MSG_IF(target_node_id < 0 || target_node_id >= m_nodes.GetN(), "Invalid target node id.");
                NS_ABORT_MSG_IF(next_hop_node_id < -1 || next_hop_node_id >= m_nodes.GetN(), "Invalid next hop node id.");

                // Drops are only valid if all three values are -1
                NS_ABORT_MSG_IF(
                        !(next_hop_node_id == -1 && my_if_id == -1 && next_if_id == -1)
                        &&
                        !(next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1),
                        "All three must be -1 for it to signify a drop."
                );

                // Check the interfaces exist
                NS_ABORT_MSG_UNLESS(my_if_id == -1 || (my_if_id >= 0 && my_if_id + 1 < m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNInterfaces()), "Invalid current interface");
                NS_ABORT_MSG_UNLESS(next_if_id == -1 || (next_if_id >= 0 && next_if_id + 1 < m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNInterfaces()), "Invalid next hop interface");

                // Node id and interface id checks are only necessary for non-drops
                if (next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1) {

                    // It must be either GSL or ISL
                    bool source_is_gsl = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<GSLNetDevice>() != 0;
                    bool source_is_isl = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>() != 0;
                    NS_ABORT_MSG_IF((!source_is_gsl) && (!source_is_isl), "Only GSL and ISL network devices are supported");

                    // If current is a GSL interface, the destination must also be a GSL interface
                    NS_ABORT_MSG_IF(
                        source_is_gsl &&
                        m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + next_if_id)->GetObject<GSLNetDevice>() == 0,
                        "Destination interface must be attached to a GSL network device"
                    );

                    // If current is a p2p laser interface, the destination must match exactly its counter-part
                    NS_ABORT_MSG_IF(
                        source_is_isl &&
                        m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + next_if_id)->GetObject<PointToPointLaserNetDevice>() == 0,
                        "Destination interface must be an ISL network device"
                    );
                    if (source_is_isl) {
                        Ptr<NetDevice> device0 = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>()->GetChannel()->GetDevice(0);
                        Ptr<NetDevice> device1 = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>()->GetChannel()->GetDevice(1);
                        Ptr<NetDevice> other_device = device0->GetNode()->GetId() == current_node_id ? device1 : device0;
                        NS_ABORT_MSG_IF(other_device->GetNode()->GetId() != next_hop_node_id, "Next hop node id across does not match");
                        NS_ABORT_MSG_IF(other_device->GetIfIndex() != 1 + next_if_id, "Next hop interface id across does not match");
                    }

                }

                // Add to forwarding state
                m_globalForwardingState[current_node_id][target_node_id] = std::make_tuple(next_hop_node_id, 1 + my_if_id, 1 + next_if_id);
                m_arbiters.at(current_node_id)->SetSingleForwardState(
                        target_node_id,
                        next_hop_node_id,
                        1 + my_if_id,   // Skip the loop-back interface
                        1 + next_if_id  // Skip the loop-back interface
                );

                // Next line
                line_counter++;

            }

            // Close file
            fstate_file.close();

        } else {
            throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
        }

    }

} // namespace ns3

