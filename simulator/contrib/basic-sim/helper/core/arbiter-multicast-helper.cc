#include "arbiter-multicast-helper.h"

namespace ns3 {

ArbiterMulticastHelper::ArbiterMulticastHelper(Ptr<BasicSimulation> basicSimulation, Ptr<TopologyPtop> topology, const std::vector<MulticastUdpInfo> &multicast_reqs) {
    m_basicSimulation = basicSimulation;
    m_topology = topology;
    m_multicast_reqs = multicast_reqs;
    m_nodes = topology->GetNodes();

    std::cout << "SETUP MULTICAST ROUTING" << std::endl;

    // Calculate and instantiate the routing
    std::cout << "  > ArbiterMulticastHelper:Calculating ECMP routing for unicast" << std::endl;
    std::vector<std::vector<std::vector<uint32_t>>> global_ecmp_state = CalculateGlobalState(topology);
    basicSimulation->RegisterTimestamp("ArbiterMulticastHelper Calculate ECMP routing state");

    std::cout << "  > Setting the routing arbiter on each node" << std::endl;
    for (int i = 0; i < topology->GetNumNodes(); i++) {
        Ptr<ArbiterMulticast> arbiterMulticast = CreateObject<ArbiterMulticast>(m_nodes.Get(i), m_nodes, topology, global_ecmp_state[i]);
        m_arbiters.push_back(arbiterMulticast);
        m_node_to_nbr_if_idx.push_back(arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx());
        // testMulticast(topology, arbiterMulticast, i);
        m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbiterRouting>()->SetArbiter(arbiterMulticast);
    }
    basicSimulation->RegisterTimestamp("Setup routing arbiter on each node");

    basicSimulation->RegisterTimestamp("Setup multicast routing state");
    // ReadGlobalMulticastState();
    CalGlobalMulticastState(global_ecmp_state);

    std::cout << std::endl;

}

void ArbiterMulticastHelper::testMulticast(Ptr<TopologyPtop> topology, Ptr<ArbiterMulticast> arbiterMulticast, uint32_t node_id) {
    throw std::runtime_error("Testing Method is Disabled!!!");
    //testing: static routing
    NodeContainer nodes = topology->GetNodes();
    std::cout << "  > Setting the multicast routing state on node " << node_id << std::endl;
    //init
    Ipv4Address origin = nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); //get local addr
    Ipv4Address group = topology->GetMulticastGroupBase(); //req id 0
    std::cout << "  > origin " << origin << std::endl;
    std::cout << "  > group " << group << std::endl;
    if(node_id == 0){
        // node 0: outbound
        std::vector<uint32_t> out_if_ids = {arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[1]};
        arbiterMulticast->AddMulticastRoute(origin, group, 0, out_if_ids);
    }
    if(node_id == 1){
        // node 1: forward
        std::vector<uint32_t> out_if_ids = {arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[2], arbiterMulticast->ArbiterPtop::GetNbrIdToIfIdx()[3]};
        arbiterMulticast->AddMulticastRoute(origin, group, 0, out_if_ids);
    }
    
}

void ArbiterMulticastHelper::ReadGlobalMulticastState() {
    std::cout << "  > Reading multicast route file and add route" << std::endl;
    // Filename
    std::string filename = m_basicSimulation->GetRunDir() + "/" + m_basicSimulation->GetConfigParamOrFail("multicast_route_filename");
    //check exist
    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }
    // Open file
    std::string line;
    std::ifstream route_file(filename);
    if (route_file) {
        // Go over each line
        size_t line_counter = 0;
        while (getline(route_file, line)) {
            // Split on ,
            std::vector<std::string> comma_split = split_string(line, ",", 7);
            // read entry
            int64_t multicast_burst_id = parse_positive_int64(comma_split[0]);
            int64_t node_id_to_install = parse_positive_int64(comma_split[1]);
            int64_t src_node_id = parse_positive_int64(comma_split[2]);
            int64_t src_nexthop_id = parse_positive_int64(comma_split[3]);
            int64_t input_nbr_id = parse_int64(comma_split[4]);
            int64_t output_nbr_num = parse_positive_int64(comma_split[5]);
            // read out nbr ids
            std::string output_nbr_ids_str = comma_split[6];
            std::vector<int64_t> output_nbr_ids;
            for (std::string space_split: split_string(output_nbr_ids_str, " ", output_nbr_num)) {
                output_nbr_ids.push_back(parse_positive_int64(space_split));
            }
            // check correct
            if (multicast_burst_id < 0 || multicast_burst_id >= (int64_t)m_multicast_reqs.size()) {// Must be a valid burst_id identifier
                throw std::runtime_error(format_string("The multicast_burst_id %d is out of multicast_burst id range of [0, %"  PRId64 ").", multicast_burst_id, m_multicast_reqs.size()));
            }
            if (!m_topology->IsValidEndpoint(src_node_id)) {// Check endpoint validity
                throw std::invalid_argument(format_string("Invalid from-endpoint for a schedule entry based on topology: %d", src_node_id));
            }  
            if (node_id_to_install < 0 || node_id_to_install >= m_topology->GetNumNodes()) {// Must be a valid node identifier
                throw std::runtime_error(format_string("The node_id_to_install %d is out of node id range of [0, %"  PRId64 ").", node_id_to_install, m_topology->GetNumNodes()));
            }
            if (src_node_id < 0 || src_node_id >= m_topology->GetNumNodes()) {// Must be a valid node identifier
                throw std::runtime_error(format_string("The src_node_id %d is out of node id range of [0, %"  PRId64 ").", src_node_id, m_topology->GetNumNodes()));
            }
            if (src_nexthop_id < 0 || src_nexthop_id >= m_topology->GetNumNodes()) {// Must be a valid node identifier
                throw std::runtime_error(format_string("The src_nexthop_id %d is out of node id range of [0, %"  PRId64 ").", src_nexthop_id, m_topology->GetNumNodes()));
            }
            std::vector<uint32_t> out_if_ids;
            for (auto out_nbr_id : output_nbr_ids) {
                if (out_nbr_id < 0 || out_nbr_id >= m_topology->GetNumNodes()) {// Must be a valid node identifier
                    throw std::runtime_error(format_string("The out_nbr_id %d is out of node id range of [0, %"  PRId64 ").", out_nbr_id, m_topology->GetNumNodes()));
                }
                uint32_t out_if_id = m_node_to_nbr_if_idx[node_id_to_install][out_nbr_id];
                if (out_if_id == 0) {
                    throw std::runtime_error(format_string("The out_if_id %d is not a neighbor of node %d.", out_if_id, node_id_to_install));
                }
                out_if_ids.push_back(m_node_to_nbr_if_idx[node_id_to_install][out_nbr_id]);
            }
            uint32_t src_if_id = m_node_to_nbr_if_idx[src_node_id][src_nexthop_id];
            uint32_t iif_id = 0; //default src route
            if (input_nbr_id != -1) { //src route
                if (input_nbr_id < 0 || input_nbr_id >= m_topology->GetNumNodes()) {// Must be a valid node identifier
                    throw std::runtime_error(format_string("The input_nbr_id %d is out of node id range of [0, %"  PRId64 ").", input_nbr_id, m_topology->GetNumNodes()));
                }
                iif_id = m_node_to_nbr_if_idx[node_id_to_install][input_nbr_id];
                if (iif_id == 0) {
                    throw std::runtime_error(format_string("The iif_id %d is not a neighbor of node %d.", iif_id, node_id_to_install));
                }
            }
            if (src_if_id == 0) {
                throw std::runtime_error(format_string("The src_if_id %d is not a neighbor of node %d.", src_if_id, src_node_id));
            }
            // add route
            Ipv4Address origin = m_nodes.Get(src_node_id)->GetObject<Ipv4>()->GetAddress(src_if_id, 0).GetLocal();
            Ipv4Address group = Ipv4Address(m_topology->GetMulticastGroupBase().Get() + multicast_burst_id);
            m_arbiters[node_id_to_install]->AddMulticastRoute(origin, group, iif_id, out_if_ids);
            // Next line
            line_counter++;
        }
        // Close file
        route_file.close();
    } else {
        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }    
}

void ArbiterMulticastHelper::CalGlobalMulticastState(std::vector<std::vector<std::vector<uint32_t>>> &global_ecmp_state) {
    m_basicSimulation->GetConfigParamOrFail("multicast_route_filename"); //lazily activate Config key 'multicast_route_filename'
    std::cout << "  > Calculating multicast route from ecmp states" << std::endl;

    for (MulticastUdpInfo req : m_multicast_reqs) {
        //cal oifs for every req separately
        std::set<uint32_t> nodes_ids_to_install;
        std::map<uint32_t, std::set<uint32_t>> node_id_to_oifs;
        std::map<uint32_t, uint32_t> node_id_to_iif;
        int64_t src_id = req.GetFromNodeId();
        std::set<int64_t> dst_ids = req.GetToNodeIds();
        uint32_t cur_node_id, nxt_node_id, cur_to_nxt_oif_id=0;
        for (auto dst_id : dst_ids) {
            cur_node_id = src_id;
            while (cur_node_id != (uint32_t)dst_id) {
                nxt_node_id = global_ecmp_state[cur_node_id][dst_id][0]; //select the first ecmp candidate by default
                cur_to_nxt_oif_id = m_node_to_nbr_if_idx[cur_node_id][nxt_node_id];
                nodes_ids_to_install.insert(cur_node_id); // auto filter duplicate element
                if (node_id_to_oifs.find(cur_node_id) != node_id_to_oifs.end()) { //cur_node_id has been recorded
                    node_id_to_oifs[cur_node_id].insert(cur_to_nxt_oif_id); // add oif id to set
                }
                else {
                    node_id_to_oifs.insert({cur_node_id, {cur_to_nxt_oif_id}}); // init a set
                }
                node_id_to_iif[nxt_node_id] = m_node_to_nbr_if_idx[nxt_node_id][cur_node_id];
                cur_node_id = nxt_node_id; //hop to next
            }
        }
        //add multicast route for this req
        if (node_id_to_oifs[src_id].size() != 1) {
            throw std::runtime_error("CalGlobalMulticastState: Src oif num != 1(not supported by ns3 routing protocol)");
        }
        Ipv4Address origin = m_nodes.Get(src_id)->GetObject<Ipv4>()->GetAddress(*node_id_to_oifs[src_id].begin(), 0).GetLocal();
        Ipv4Address group = Ipv4Address(m_topology->GetMulticastGroupBase().Get() + req.GetUdpBurstId());
        uint32_t iif_id;
        for (auto node_id_to_install : nodes_ids_to_install) {
            if (node_id_to_install == src_id) {
                iif_id = 0;
            }
            else {
                iif_id = node_id_to_iif[node_id_to_install]; // use 0 for simplicity
            }
            std::vector<uint32_t> out_if_ids(node_id_to_oifs[node_id_to_install].begin(), node_id_to_oifs[node_id_to_install].end());
            m_arbiters[node_id_to_install]->AddMulticastRoute(origin, group, iif_id, out_if_ids);
        }
    }
}

} // namespace ns3

