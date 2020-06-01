#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "foodsystem.grpc.pb.h"


class FoodVendor final : public foodsystem::FoodSystem::Service {
public:
  grpc::Status GetInfoFromVendors(grpc::ServerContext* context,
                        const foodsystem::IngredientInfo* request,
                        foodsystem::InventoryInfo* reply) override {
    int size = request->vendors_size();

    for(int i = 0 ; i < size; i++){
        foodsystem::ItemInfo* item = reply->add_iteminfo();
        item->set_price(inventory[request->vendors(i)][request->name()]);
        item->set_vendor(request->vendors(i));
    }

    return grpc::Status::OK;
  }

private:

    std::unordered_map<std::string, std::map<std::string, double>> inventory = {{"Amazon", {{"onion", 10}, {"tomato", 8}}},
                                                                                {"Walmart", {{"onion", 5}, {"eggs", 3}, {"milk", 12}}},
                                                                                {"Costco", {{"eggs", 2}, {"potato", 11}}}
                                                                                };

};

void RunServer() {
  std::string server_address("0.0.0.0:9008");
  FoodVendor service;

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