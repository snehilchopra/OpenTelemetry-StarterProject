#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "foodsystem.grpc.pb.h"


class FoodSupplier final : public foodsupplier::FoodSupplier::Service {
public:
  grpc::Status GetVendors(grpc::ServerContext* context,
                        const foodsupplier::Ingredient* request,
                        foodsupplier::List* reply) override {
    auto j = request->name();
    auto it = suppliers.find(request->name());
    if (it == suppliers.end()) {
      const std::string& h = "";
      reply->set_items(h);
    } else {
      const std::string& h = it->second[0];
      reply->set_items(h);
    }
    return grpc::Status::OK;
  }

private:
  // The actual database.
  std::map<std::string, std::vector<std::string>> suppliers = {{"Amazon", {"onion"}}};

};

void RunServer() {
  // This parts is taken from the "hello world" gRPC sample.
  std::string server_address("0.0.0.0:50051");
  FoodSupplier service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}