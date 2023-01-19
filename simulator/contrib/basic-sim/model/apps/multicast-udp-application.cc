/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */


#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"

#include "multicast-udp-application.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("MulticastUdpApplication");

    NS_OBJECT_ENSURE_REGISTERED (MulticastUdpApplication);

    TypeId
    MulticastUdpApplication::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::MulticastUdpApplication")
                .SetParent<Application>()
                .SetGroupName("Applications")
                .AddConstructor<MulticastUdpApplication>()
                .AddAttribute("Port", "Port on which we listen for incoming packets.",
                              UintegerValue(9),
                              MakeUintegerAccessor(&MulticastUdpApplication::m_port),
                              MakeUintegerChecker<uint16_t>())
                .AddAttribute ("BaseLogsDir",
                               "Base logging directory (logging will be placed here, i.e. logs_dir/udp_burst_[UDP burst id]_{incoming, outgoing}.csv",
                               StringValue (""),
                               MakeStringAccessor (&MulticastUdpApplication::m_baseLogsDir),
                               MakeStringChecker ())
                .AddAttribute("MaxUdpPayloadSizeByte", "Total UDP payload size (byte) before it gets fragmented.",
                              UintegerValue(1472), // 1500 (point-to-point default) - 20 (IP) - 8 (UDP) = 1472
                              MakeUintegerAccessor(&MulticastUdpApplication::m_max_udp_payload_size_byte),
                              MakeUintegerChecker<uint32_t>());
        return tid;
    }

    MulticastUdpApplication::MulticastUdpApplication() {
        NS_LOG_FUNCTION(this);
        m_next_internal_burst_idx = 0;
    }

    MulticastUdpApplication::~MulticastUdpApplication() {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    uint32_t MulticastUdpApplication::GetMaxUdpPayloadSizeByte() {
        return m_max_udp_payload_size_byte;
    }

    void
    MulticastUdpApplication::RegisterOutgoingBurst(MulticastUdpInfo burstInfo, InetSocketAddress targetAddress, bool enable_precise_logging) {
        //check src
        NS_ABORT_MSG_IF(burstInfo.GetFromNodeId() != this->GetNode()->GetId(), "Source node identifier is not that of this node.");
        //check burstinfo start time order
        if (m_outgoing_bursts.size() >= 1 && burstInfo.GetStartTimeNs() < std::get<0>(m_outgoing_bursts[m_outgoing_bursts.size() - 1]).GetStartTimeNs()) {
            throw std::runtime_error("Multicast Bursts must be added weakly ascending on start time");
        }
        //record burst info
        m_outgoing_bursts.push_back(std::make_tuple(burstInfo, targetAddress));
        m_outgoing_bursts_packets_sent_counter.push_back(0);
        m_outgoing_bursts_event_id.push_back(EventId());
        m_outgoing_bursts_enable_precise_logging.push_back(enable_precise_logging);
        //create log file
        if (enable_precise_logging) {
            std::ofstream ofs;
            ofs.open(m_baseLogsDir + "/" + format_string("multicast_udp_%" PRIu64 "_outgoing.csv", burstInfo.GetUdpBurstId()));
            ofs.close();
        }
    }

    void
    MulticastUdpApplication::RegisterIncomingBurst(MulticastUdpInfo burstInfo, bool enable_precise_logging) {
        //check dst
        // NS_ABORT_MSG_IF(burstInfo.GetToNodeId() != this->GetNode()->GetId(), "Destination node identifier is not that of this node.");
        std::set<int64_t> to_node_ids = burstInfo.GetToNodeIds();
        int64_t cur_node_id = this->GetNode()->GetId();
        NS_ABORT_MSG_IF(to_node_ids.find(cur_node_id) == to_node_ids.end(), "Destination node identifier is not that of this node."); //find() returns set.end() means not found
        m_incoming_bursts.push_back(burstInfo);
        m_incoming_bursts_received_counter[burstInfo.GetUdpBurstId()] = 0;
        m_incoming_bursts_enable_precise_logging[burstInfo.GetUdpBurstId()] = enable_precise_logging;
        if (enable_precise_logging) {
            std::ofstream ofs;
            ofs.open(m_baseLogsDir + "/" + format_string("multicast_udp_%" PRIu64 "_incoming.csv", burstInfo.GetUdpBurstId()));
            ofs.close();
        }
    }

    void
    MulticastUdpApplication::DoDispose(void) {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    MulticastUdpApplication::StartApplication(void) {
        NS_LOG_FUNCTION(this);

        // Bind a socket to the UDP port
        if (m_socket == 0) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            // std::cout << " -------------bind port " << m_port << std::endl;
            if (m_socket->Bind(local) == -1) {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            std::cout << "    > Multicast app start at Node " << GetNode()->GetId() << std::endl;
        }

        // Receive of packets
        m_socket->SetRecvCallback(MakeCallback(&MulticastUdpApplication::HandleRead, this));

        // First process call is for the start of the first burst
        if (m_outgoing_bursts.size() > 0) {
            m_startNextBurstEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[0]).GetStartTimeNs()), &MulticastUdpApplication::StartNextBurst, this);
        }

    }

    void
    MulticastUdpApplication::StartNextBurst()
    {
        int64_t now_ns = Simulator::Now().GetNanoSeconds();

        // If this function is called, there must be a next burst
        if (m_next_internal_burst_idx >= m_outgoing_bursts.size() || std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() != now_ns) {
            throw std::runtime_error("No next burst available; this function should not have been called.");
        }

        // Start the self-calling (and self-ending) process of sending out packets of the burst
        BurstSendOut(m_next_internal_burst_idx);

        // Schedule the start of the next burst if there are more
        m_next_internal_burst_idx += 1;
        if (m_next_internal_burst_idx < m_outgoing_bursts.size()) {
            m_startNextBurstEvent = Simulator::Schedule(NanoSeconds(std::get<0>(m_outgoing_bursts[m_next_internal_burst_idx]).GetStartTimeNs() - now_ns), &MulticastUdpApplication::StartNextBurst, this);
        }

    }

    void
    MulticastUdpApplication::BurstSendOut(size_t internal_burst_idx)
    {

        // Send out the packet as desired
        TransmitFullPacket(internal_burst_idx);

        // Schedule the next if the packet gap would not exceed the rate
        uint64_t now_ns = Simulator::Now().GetNanoSeconds();
        MulticastUdpInfo info = std::get<0>(m_outgoing_bursts[internal_burst_idx]);
        uint64_t packet_gap_nanoseconds = std::ceil(1500.0 / (info.GetTargetRateMegabitPerSec() / 8000.0)); //pkt trans dura
        //if a single pkt is not enough, schedule next send
        if (now_ns + packet_gap_nanoseconds < (uint64_t) (info.GetStartTimeNs() + info.GetDurationNs())) {
            m_outgoing_bursts_event_id.at(internal_burst_idx) = Simulator::Schedule(NanoSeconds(packet_gap_nanoseconds), &MulticastUdpApplication::BurstSendOut, this, internal_burst_idx);
        }

    }

    void
    MulticastUdpApplication::TransmitFullPacket(size_t internal_burst_idx) {

        // Header with (udp_burst_id, seq_no)
        IdSeqHeader idSeq;
        idSeq.SetId(std::get<0>(m_outgoing_bursts[internal_burst_idx]).GetUdpBurstId());
        idSeq.SetSeq(m_outgoing_bursts_packets_sent_counter[internal_burst_idx]);

        // One more packet will be sent out
        m_outgoing_bursts_packets_sent_counter[internal_burst_idx] += 1;

        // Log precise timestamp sent away of the sequence packet if needed
        if (m_outgoing_bursts_enable_precise_logging[internal_burst_idx]) {
            std::ofstream ofs;
            ofs.open(m_baseLogsDir + "/" + format_string("multicast_udp_%" PRIu64 "_outgoing.csv", idSeq.GetId()), std::ofstream::out | std::ofstream::app);
            ofs << idSeq.GetId() << "," << idSeq.GetSeq() << "," << Simulator::Now().GetNanoSeconds() << std::endl;
            ofs.close();
        }

        // A full payload packet
        Ptr<Packet> p = Create<Packet>(m_max_udp_payload_size_byte - idSeq.GetSerializedSize());
        p->AddHeader(idSeq);

        // Send out the packet to the target address
        m_socket->SendTo(p, 0, std::get<1>(m_outgoing_bursts[internal_burst_idx]));

    }

    void
    MulticastUdpApplication::StopApplication() {
        NS_LOG_FUNCTION(this);
        if (m_socket != 0) {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback < void, Ptr < Socket > > ());
            Simulator::Cancel(m_startNextBurstEvent);
            for (EventId& eventId : m_outgoing_bursts_event_id) {
                Simulator::Cancel(eventId);
            }
        }
    }

    void
    MulticastUdpApplication::HandleRead(Ptr <Socket> socket) {
        NS_LOG_FUNCTION(this << socket);
        Ptr <Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from))) {
            std::cout << "[App-Recv]Multicast App socket recv a pkt at node " << m_node->GetId() << " from " << from << std::endl;
            // Extract burst identifier and packet sequence number
            IdSeqHeader incomingIdSeq;
            packet->RemoveHeader (incomingIdSeq);

            // Count packets from incoming bursts
            m_incoming_bursts_received_counter.at(incomingIdSeq.GetId()) += 1;

            // Log precise timestamp received of the sequence packet if needed
            if (m_incoming_bursts_enable_precise_logging[incomingIdSeq.GetId()]) {
                std::ofstream ofs;
                ofs.open(m_baseLogsDir + "/" + format_string("multicast_udp_%" PRIu64 "_incoming.csv", incomingIdSeq.GetId()), std::ofstream::out | std::ofstream::app);
                ofs << incomingIdSeq.GetId() << "," << incomingIdSeq.GetSeq() << "," << Simulator::Now().GetNanoSeconds() << std::endl;
                ofs.close();
            }

        }
    }

    std::vector<std::tuple<MulticastUdpInfo, uint64_t>>
    MulticastUdpApplication::GetOutgoingBurstsInformation() {
        std::vector<std::tuple<MulticastUdpInfo, uint64_t>> result;
        for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
            result.push_back(std::make_tuple(std::get<0>(m_outgoing_bursts[i]), m_outgoing_bursts_packets_sent_counter[i]));
        }
        return result;
    }

    std::vector<std::tuple<MulticastUdpInfo, uint64_t>>
    MulticastUdpApplication::GetIncomingBurstsInformation() {
        std::vector<std::tuple<MulticastUdpInfo, uint64_t>> result;
        for (size_t i = 0; i < m_incoming_bursts.size(); i++) {
            result.push_back(std::make_tuple(m_incoming_bursts[i], m_incoming_bursts_received_counter.at(m_incoming_bursts[i].GetUdpBurstId())));
        }
        return result;
    }

    uint64_t
    MulticastUdpApplication::GetSentCounterOf(int64_t udp_burst_id) {
        std::vector<std::tuple<MulticastUdpInfo, uint64_t>> result;
        for (size_t i = 0; i < m_outgoing_bursts.size(); i++) {
            if (std::get<0>(m_outgoing_bursts[i]).GetUdpBurstId() == udp_burst_id) {
                return m_outgoing_bursts_packets_sent_counter[i];
            }
        }
        throw std::runtime_error("Sent counter for unknown UDP burst ID was requested");
    }

    uint64_t
    MulticastUdpApplication::GetReceivedCounterOf(int64_t udp_burst_id) {
        return m_incoming_bursts_received_counter.at(udp_burst_id);
    }

} // namespace ns3

