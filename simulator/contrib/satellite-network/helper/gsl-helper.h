/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * (based on point-to-point helper)
 * Author: Jens Eirik Saethre  June 2019
 *         Andre Aguas         March 2020
 *         Simon               2020
 */


#ifndef GSL_HELPER_H
#define GSL_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/gsl-net-device.h"
#include "ns3/gsl-channel.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "ns3/gsl-helper.h"

namespace ns3 {

class NetDevice;
class Node;

class GSLHelper
{
public:
  GSLHelper ();
  virtual ~GSLHelper () {}

  // Set device and channel attributes
  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());
  void SetDeviceAttribute (std::string name, const AttributeValue &value);
  void SetChannelAttribute (std::string name, const AttributeValue &value);

  // Installers
  NetDeviceContainer Install (NodeContainer satellites, NodeContainer gss, std::vector<std::tuple<int32_t, double>>& node_gsl_if_info);
  // std::pair<NetDeviceContainer, Ptr<GSLChannel>> Install (NodeContainer satellites, NodeContainer gss, std::vector<std::tuple<int32_t, double>>& node_gsl_if_info);
  Ptr<GSLNetDevice> Install (Ptr<Node> node, Ptr<GSLChannel> channel);
  Ptr<GSLChannel> m_gslChannel; //access by topology

private:
  ObjectFactory m_queueFactory;         //!< Queue Factory
  ObjectFactory m_channelFactory;       //!< Channel Factory
  ObjectFactory m_deviceFactory;        //!< Device Factory
};

} // namespace ns3

#endif /* GSL_HELPER_H */
