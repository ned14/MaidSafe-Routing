/*******************************************************************************
 *  Copyright 2012 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ******************************************************************************/

#include "maidsafe/routing/group_matrix.h"

#include <algorithm>
#include <bitset>
#include <cstdint>

#include "maidsafe/common/log.h"

#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/node_info.h"
#include "maidsafe/routing/return_codes.h"

namespace maidsafe {

namespace routing {

GroupMatrix::GroupMatrix(const NodeId& this_node_id, bool client_mode)
    : kNodeId_(this_node_id),
      unique_nodes_(),
      client_mode_(client_mode),
      matrix_() {}

void GroupMatrix::AddConnectedPeer(const NodeInfo& node_info) {
  LOG(kVerbose) << "AddConnectedPeer : " << DebugId(node_info.node_id);
  for (const auto& nodes : matrix_) {  // NOLINT (Alison)
    if (nodes.at(0).node_id == node_info.node_id) {
      LOG(kWarning) << "Already Added in matrix";
      return;
    }
  }
  if (matrix_.size() >= Parameters::closest_nodes_size) {
    assert(false && "Matrix should not have more than Parameters::closest_nodes_size rows.");
    return;
  }
  matrix_.push_back(std::vector<NodeInfo>(1, node_info));
  UpdateUniqueNodeList();
}

void GroupMatrix::RemoveConnectedPeer(const NodeInfo& node_info) {
  matrix_.erase(std::remove_if(matrix_.begin(),
                               matrix_.end(),
                               [node_info](const std::vector<NodeInfo> nodes) {
                                   return (node_info.node_id == nodes.at(0).node_id);
                               }), matrix_.end());
  UpdateUniqueNodeList();
}

std::vector<NodeInfo> GroupMatrix::GetConnectedPeers() {
  std::vector<NodeInfo> connected_peers;
  for (const auto& nodes : matrix_)  // NOLINT (Alison)
    connected_peers.push_back(nodes.at(0));
  PartialSortFromTarget(kNodeId_, Parameters::closest_nodes_size, connected_peers);
  return connected_peers;
}

NodeInfo GroupMatrix::GetConnectedPeerFor(const NodeId& target_node_id) {
/*
  for (const auto& nodes : matrix_) {
    if (nodes.at(0).node_id == target_node_id) {
      assert(false && "Shouldn't request connected peer for node in first column of group matrix.");
      return NodeInfo();
    }
  }*/
  for (const auto& nodes : matrix_) {  // NOLINT (Alison)
    if (std::find_if(nodes.begin(), nodes.end(),
                     [target_node_id](const NodeInfo& node_info) {
                       return (node_info.node_id == target_node_id);
                     }) != nodes.end()) {
      return nodes.at(0);
    }
  }
  return NodeInfo();
}

void GroupMatrix::GetBetterNodeForSendingMessage(const NodeId& target_node_id,
                                                 const std::vector<std::string>& exclude,
                                                 bool ignore_exact_match,
                                                 NodeInfo& current_closest_peer) {
  NodeId closest_id(current_closest_peer.node_id);

  for (const auto& row : matrix_) {  // NOLINT (Alison)
    if (ignore_exact_match && row.at(0).node_id == target_node_id)
      continue;
    if (std::find(exclude.begin(), exclude.end(), row.at(0).node_id.string()) != exclude.end())
      continue;

    for (const auto& node : row) {  // NOLINT (Alison)
      if (node.node_id == kNodeId_)
        continue;
      if (ignore_exact_match && node.node_id == target_node_id)
        continue;
      if (std::find(exclude.begin(), exclude.end(), node.node_id.string()) != exclude.end())
        continue;
      if (NodeId::CloserToTarget(node.node_id, closest_id, target_node_id)) {
        closest_id = node.node_id;
        current_closest_peer = row.at(0);
      }
    }
  }
  LOG(kVerbose) << "[" << DebugId(kNodeId_)
                << "]\ttarget: " << DebugId(target_node_id)
                << "\tfound node in matrix: " << DebugId(closest_id)
                << "\treccommend sending to: " << DebugId(current_closest_peer.node_id);
}

void GroupMatrix::GetBetterNodeForSendingMessage(const NodeId& target_node_id,
                                                 bool ignore_exact_match,
                                                 NodeId& current_closest_peer_id) {
  NodeId closest_id(current_closest_peer_id);

  for (const auto& row : matrix_) {  // NOLINT (Alison)
    if (ignore_exact_match && row.at(0).node_id == target_node_id)
      continue;

    for (const auto& node : row) {  // NOLINT (Alison)
      if (ignore_exact_match && node.node_id == target_node_id)
        continue;
      if (NodeId::CloserToTarget(node.node_id, closest_id, target_node_id)) {
        closest_id = node.node_id;
        current_closest_peer_id = row.at(0).node_id;
      }
    }
  }
  LOG(kVerbose) << "[" << DebugId(kNodeId_)
                << "]\ttarget: " << DebugId(target_node_id)
                << "\tfound node in matrix: " << DebugId(closest_id)
                << "\treccommend sending to: " << DebugId(current_closest_peer_id);
}

std::vector<NodeInfo> GroupMatrix::GetAllConnectedPeersFor(const NodeId& target_id) {
  std::vector<NodeInfo> connected_nodes;
  for (const auto& row : matrix_) {  // NOLINT (Alison)
    if (std::find_if(row.begin(),
                     row.end(),
                     [&target_id] (const NodeInfo& node_info) {
                       return target_id == node_info.node_id;
                     }) != row.end()) {
      connected_nodes.push_back(row.at(0));
    }
  }
  return connected_nodes;
}

bool GroupMatrix::IsThisNodeGroupLeader(const NodeId& target_id, NodeId& connected_peer) {
  assert(!client_mode_ && "Client should not call IsThisNodeGroupLeader.");
  if (client_mode_)
    return false;

  LOG(kVerbose) << " Destination " << DebugId(target_id) << " kNodeId " << DebugId(kNodeId_);
  bool is_group_leader = true;
  if (unique_nodes_.empty()) {
    is_group_leader = true;
    return true;
  }

  std::string log("unique_nodes_ for " + DebugId(kNodeId_) + " are ");
  for (const auto& node : unique_nodes_) {  // NOLINT (Alison)
    log += DebugId(node.node_id) + ", ";
  }
  LOG(kVerbose) << log;

  for (const auto& node : unique_nodes_) {  // NOLINT (Alison)
    if (node.node_id == target_id)
      continue;
    if (NodeId::CloserToTarget(node.node_id, kNodeId_, target_id)) {
      LOG(kVerbose) << DebugId(node.node_id) << " could be leader";
      is_group_leader = false;
      break;
    }
  }
  if (!is_group_leader) {
    NodeId better_id(kNodeId_);
    GetBetterNodeForSendingMessage(target_id, true, better_id);
    connected_peer = better_id;
    assert(connected_peer != target_id);
  }
  return is_group_leader;
}

bool GroupMatrix::ClosestToId(const NodeId& node_id) {
  if (unique_nodes_.size() == 0)
    return true;

  PartialSortFromTarget(node_id, 2, unique_nodes_);
  if (unique_nodes_.at(0).node_id == kNodeId_)
    return true;

  if (unique_nodes_.at(0).node_id == node_id) {
    if (unique_nodes_.at(1).node_id == kNodeId_)
      return true;
    else
      return NodeId::CloserToTarget(kNodeId_, unique_nodes_.at(1).node_id, node_id);
  }

  return NodeId::CloserToTarget(kNodeId_, unique_nodes_.at(0).node_id, node_id);
}

bool GroupMatrix::IsNodeIdInGroupRange(const NodeId& target_id) {
  if (unique_nodes_.size() < Parameters::node_group_size) {
    return true;
  }

  PartialSortFromTarget(kNodeId_, Parameters::node_group_size, unique_nodes_);

  NodeInfo furthest_group_node(unique_nodes_.at(Parameters::node_group_size - 1));
  return !NodeId::CloserToTarget(furthest_group_node.node_id, target_id, kNodeId_);
}

void GroupMatrix::UpdateFromConnectedPeer(const NodeId& peer,
                                          const std::vector<NodeInfo>& nodes) {
  if (peer.IsZero()) {
    assert(false && "Invalid peer node id.");
    return;
  }
  if (nodes.size() > Parameters::closest_nodes_size) {
    assert(false && "Vector of nodes should have length less than Parameters::closest_nodes_size");
    return;
  }
  // If peer is in my group
  auto group_itr(matrix_.begin());
  for (group_itr = matrix_.begin(); group_itr != matrix_.end(); ++group_itr) {
    if ((*group_itr).at(0).node_id == peer)
      break;
  }

  if (group_itr == matrix_.end()) {
    LOG(kWarning) << "Peer Node : " << DebugId(peer)
                  << " is not in closest group of this node.";
    return;
  }

  // Update peer's row
  if (group_itr->size() > 1) {
    group_itr->erase(group_itr->begin() + 1, group_itr->end());
  }
  for (const auto& i : nodes)  // NOLINT (Alison)
    group_itr->push_back(i);

  // Update unique node vector
  UpdateUniqueNodeList();
}

bool GroupMatrix::GetRow(const NodeId& row_id, std::vector<NodeInfo>& row_entries) {
  if (row_id.IsZero()) {
    assert(false && "Invalid node id.");
    return false;
  }
  auto group_itr(matrix_.begin());
  for (group_itr = matrix_.begin(); group_itr != matrix_.end(); ++group_itr) {
    if ((*group_itr).at(0).node_id == row_id)
      break;
  }

  if (group_itr == matrix_.end())
    return false;

  row_entries.clear();
  for (uint32_t i(0); i < (*group_itr).size(); ++i) {
    if (i != 0) {
      row_entries.push_back((*group_itr).at(i));
    }
  }
  return true;
}

std::vector<NodeInfo> GroupMatrix::GetUniqueNodes() {
  return unique_nodes_;
}

bool GroupMatrix::IsRowEmpty(const NodeInfo& node_info) {
  auto group_itr(matrix_.begin());
  for (group_itr = matrix_.begin(); group_itr != matrix_.end(); ++group_itr) {
    if ((*group_itr).at(0).node_id == node_info.node_id)
      break;
  }
  assert(group_itr != matrix_.end());
  if (group_itr == matrix_.end())
    return false;

  return (group_itr->size() < 2);
}

std::vector<NodeInfo> GroupMatrix::GetClosestNodes(const uint16_t& size) {
  uint16_t size_to_sort(std::min(size, static_cast<uint16_t>(unique_nodes_.size())));
  PartialSortFromTarget(kNodeId_, size_to_sort, unique_nodes_);
  return std::vector<NodeInfo>(unique_nodes_.begin(), unique_nodes_.begin() + size_to_sort);
}

bool GroupMatrix::Contains(const NodeId& node_id) {
  return std::find_if(unique_nodes_.begin(), unique_nodes_.end(),
                      [&node_id] (const NodeInfo& node_info) {
                        return node_info.node_id == node_id;
                      }) != unique_nodes_.end();
}

void GroupMatrix::Clear() {
  for (auto& row : matrix_)
    row.clear();
  matrix_.clear();
  UpdateUniqueNodeList();
}

bool GroupMatrix::Unsubscribe(const NodeId& node_id) {
  auto nodes_info(std::find_if(matrix_.begin(),
                               matrix_.end(),
                               [node_id] (const std::vector<NodeInfo>& nodes) {
                                 return nodes[0].node_id == node_id;
                               }));
  if (nodes_info == matrix_.end())
    return true;
  matrix_.erase(nodes_info);
  return true;
}

void GroupMatrix::UpdateUniqueNodeList() {
  unique_nodes_.clear();
  // Update unique node vector
  if (!client_mode_) {
    NodeInfo node_info;
    node_info.node_id = kNodeId_;
    unique_nodes_.push_back(node_info);
  }
  for (auto itr = matrix_.begin(); itr != matrix_.end(); ++itr)
    for (size_t i(0); i !=  itr->size(); ++i)
      unique_nodes_.push_back((*itr).at(i));

  // Removing duplicates
  std::sort(unique_nodes_.begin(),
            unique_nodes_.end(),
            [&](const NodeInfo& lhs, const NodeInfo& rhs)->bool {
              return (lhs.node_id ^ kNodeId_) < (rhs.node_id ^ kNodeId_);
            });

  auto itr = std::unique(unique_nodes_.begin(),
                         unique_nodes_.end(),
                         [](const NodeInfo& lhs, const NodeInfo& rhs) {
                           return lhs.node_id == rhs.node_id;
                         });
  unique_nodes_.resize(itr - unique_nodes_.begin());
}

void GroupMatrix::PartialSortFromTarget(const NodeId& target,
                                        const uint16_t& number,
                                        std::vector<NodeInfo>& nodes) {
  uint16_t count = std::min(number, static_cast<uint16_t>(nodes.size()));
  std::partial_sort(nodes.begin(),
                    nodes.begin() + count,
                    nodes.end(),
                    [target](const NodeInfo& lhs, const NodeInfo& rhs) {
                      return NodeId::CloserToTarget(lhs.node_id, rhs.node_id, target);
                    });
}

void GroupMatrix::PrintGroupMatrix() {
  auto group_itr(matrix_.begin());
  std::string tab("\t");
  std::string output("Group matrix of node with NodeID: " + DebugId(kNodeId_));
  for (group_itr = matrix_.begin(); group_itr != matrix_.end(); ++group_itr) {
    output.append("\nGroup matrix row:");
    for (uint32_t i(0); i < (*group_itr).size(); ++i) {
      output.append(tab);
      output.append(DebugId((*group_itr).at(i).node_id));
    }
  }
  LOG(kVerbose) << output;
}

}  // namespace routing

}  // namespace maidsafe
