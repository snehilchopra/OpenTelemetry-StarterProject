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
  std::map<std::string, std::vector<std::string>> suppliers = {{"Amazon", {"onion"}}};

};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FoodSupplier service;

  grpc::ServerBuilder builder;
  
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}