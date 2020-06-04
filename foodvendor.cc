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
* the 'GetInfoFromVendor' method of the FoodSystem Service. This will be invoked
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
  grpc::Status GetInfoFromVendor(grpc::ServerContext* context,
                        const foodsystem::PriceRequest* request,
                        foodsystem::PriceInfo* reply) override {

    // opencensus::trace::Span span = grpc::GetSpanFromServerContext(context);
    // span.AddAttribute("my_attribute", "red");
    // span.AddAnnotation("Fetching inventory info from " + request.vendor());

    // Fetch the price of the ingredient from the vendor
    reply->set_price(inventory[request->vendor()][request->ingredient()]);

    return grpc::Status::OK;
  }

private:
    /* Statically stored database of suppliers and their respective inventory and price */
    std::unordered_map<std::string, std::map<std::string, double>> inventory = {{"Amazon", {{"onion", 2.39}, {"tomato", 1.99}}},
                                                                                {"Walmart", {{"onion", 2.99}, {"eggs", 1.39}, {"milk", 11}}},
                                                                                {"Costco", {{"eggs", 0.99}, {"potato", 4.99}}}
                                                                                };

};

/*
* Runs the gRPC Server
*/
void RunServer() {
  // Register the OpenCensus gRPC plugin to enable stats and tracing in gRPC.
  grpc::RegisterOpenCensusPlugin();

  RegisterExporters();

  // The server address of the form "address:port"
  std::string server_address("127.0.0.1:9002");
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