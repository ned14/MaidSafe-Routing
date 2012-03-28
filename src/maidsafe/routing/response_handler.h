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

#ifndef MAIDSAFE_ROUTING_RESPONSE_HANDLER_H_
#define MAIDSAFE_ROUTING_RESPONSE_HANDLER_H_

#include "boost/thread/shared_mutex.hpp"
#include "boost/thread/mutex.hpp"
#include "maidsafe/common/rsa.h"
#include "maidsafe/routing/rpcs.h"
#include "maidsafe/transport/managed_connections.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/node_id.h"
#include "maidsafe/routing/log.h"


namespace maidsafe {

namespace routing {

class RoutingTable;

class ResponseHandler {
 public:
  ResponseHandler(const NodeValidationFunctor &node_Validation_functor,
                 RoutingTable &routing_table,
                 transport::ManagedConnections &transport,
                 Rpcs &rpcs);
  ~ResponseHandler();
  void ProcessPingResponse(protobuf::Message &message);
  void ProcessConnectResponse(protobuf::Message &message);
  void ProcessFindNodeResponse(protobuf::Message &message);
 private:
  ResponseHandler(const ResponseHandler&);  // no copy
  ResponseHandler& operator=(const ResponseHandler&);  // no assign
  NodeValidationFunctor node_validation_functor_;
  RoutingTable &routing_table_;
  transport::ManagedConnections &transport_;
  Rpcs &rpcs_;
};

}  // namespace routing

}  // namespace maidsafe

#endif  // MAIDSAFE_ROUTING_RESPONSE_HANDLER_H_