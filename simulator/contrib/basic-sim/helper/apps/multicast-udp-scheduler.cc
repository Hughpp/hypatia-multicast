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

            std::cout << "  > ByLul multicast app setup!!!" << std::endl;
            
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

            std::cout << "  > ByLul multicast log write!!!" << std::endl;

            // Register completion
            std::cout << "  > Multicast UDP log files have been written" << std::endl;
            m_basicSimulation->RegisterTimestamp("Write Multicast UDP log files");

        }

        std::cout << std::endl;
    }

}
