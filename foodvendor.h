/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#ifndef FOOD_VENDOR_H
#define FOOD_VENDOR_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "foodsystem.grpc.pb.h"

#include <grpcpp/opencensus.h>

#include "foodsystem.grpc.pb.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"


using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using foodsystem::FoodSystem;
using foodsystem::PriceInfo;
using foodsystem::PriceRequest;

class ServerImpl final {
 public:
  /*
  * Destructor
  */
  ~ServerImpl();

  /*
  * Runs the gRPC asynchronous server
  */
  void Run();

 private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData {
    public:
      /* 
      * Take in the "service" instance (in this case representing an asynchronous
      * server) and the completion queue "cq" used for asynchronous communication
      * with the gRPC runtime.
      * 
      * @param service : The means of communication with the gRPC at runtime for an asnychronous server
      * @param cq : The produce-consumer queue for asnychronous notifications
      */ 
      CallData(FoodSystem::AsyncService* service, ServerCompletionQueue* cq);

      /*
      * Handles all server logic. Tracks and acts upon the current state of an instance.
      */ 
      void Proceed();

    private:
      // Statically stored database of suppliers and their respective inventory and price 
      std::unordered_map<std::string, std::map<std::string, double>> inventory = {
                                                                                  {"Amazon", {{"onion", 2.39}, {"tomato", 1.99},
                                                                                              {"cheese", 0.89}, {"eggs", 1.5}, {"mango", 4.5}}},
                                                                                  {"Walmart", {{"onion", 2.99}, {"eggs", 1.39},
                                                                                               {"milk", 11}, {"orange", 2.8}}},
                                                                                  {"Costco", {{"eggs", 0.99}, {"potato", 4.99}, {"cheese", 1.1},
                                                                                              {"tomato", 2.3}, {"avocado", 3.4}}},
                                                                                  {"Bazaar", {{"onion", 2.4}, {"milk", 9}, {"potato", 4.2},
                                                                                                {"orange", 1.99}}},
                                                                                  {"Safeway", {{"orange", 1.5}, {"cheese", 0.5}, 
                                                                                               {"avocado", 4.1}}}
                                                                                  };

      // The means of communication with the gRPC runtime for an asynchronous
      // server.
      FoodSystem::AsyncService* service_;

      // The producer-consumer queue where for asynchronous server notifications.
      ServerCompletionQueue* cq_;

      // Context for the rpc, allowing to tweak aspects of it such as the use
      // of compression, authentication, as well as to send metadata back to the
      // client.
      ServerContext ctx_;

      // What we get from the client.
      PriceRequest request_;

      // What we send back to the client.
      PriceInfo reply_;

      // The means to get back to the client.
      ServerAsyncResponseWriter<PriceInfo> responder_;

      // State machine with the following states.
      // Used for tracking the progress of different instances
      enum CallStatus { CREATE, PROCESS, FINISH };

      // The current serving state.
      CallStatus status_;  
  };

  /*
  * Handles all the incoming RPCs.
  */
  void HandleRpcs();
 
  std::unique_ptr<ServerCompletionQueue> cq_;
  FoodSystem::AsyncService service_;
  std::unique_ptr<Server> server_;
};

    

#endif