#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "foodsystem.grpc.pb.h"

#include <grpcpp/opencensus.h>

#include "exporters.h"
#include "foodsystem.grpc.pb.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"

/*
* This class implements the FoodVendor service. We only implement the
* the 'GetInfoFromVendors' method of the FoodSystem Service. This will be invoked
* by the running gRPC server.
*/
class FoodVendor final : public foodsystem::FoodSystem::Service {
public:

  /*
  * Fetches the inventory info, i.e., the name of the vendor and the price of the 
  * user-specified ingredient.
  * 
  * @param context - Context object which carries scoped values and propagates states between the client and server
  * @param request - Contains the list of suppliers who have the user-specified ingredient as
  *                  well as the name of the ingredients
  * @param reply - Server generated reply which contains the list of potential suppliers
  * @return grpc::Status::OK - A field which tells us that the operation completed successfully
  */
  grpc::Status GetInfoFromVendors(grpc::ServerContext* context,
                        const foodsystem::IngredientInfo* request,
                        foodsystem::InventoryInfo* reply) override {
    // opencensus::trace::Span span = grpc::GetSpanFromServerContext(context);
    // span.AddAttribute("my_attribute", "red");
    // span.AddAnnotation("Fetching inventory info from vendors");

    // Fetch the price of them ingredient from each vendor
    int size = request->vendors_size();
    for(int i = 0 ; i < size; i++){
        foodsystem::ItemInfo* item = reply->add_iteminfo();
        item->set_price(inventory[request->vendors(i)][request->name()]);
        item->set_vendor(request->vendors(i));
    }

    return grpc::Status::OK;
  }

private:
    // Statically stored database of suppliers and their respective inventory and price
    std::unordered_map<std::string, std::map<std::string, double>> inventory = {{"Amazon", {{"onion", 10}, {"tomato", 8}}},
                                                                                {"Walmart", {{"onion", 5}, {"eggs", 3}, {"milk", 12}}},
                                                                                {"Costco", {{"eggs", 2}, {"potato", 11}}}
                                                                                };

};

void RunServer() {
  // Register the OpenCensus gRPC plugin to enable stats and tracing in gRPC.
  grpc::RegisterOpenCensusPlugin();

  RegisterExporters();

  // The server address of the form "address:port"
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