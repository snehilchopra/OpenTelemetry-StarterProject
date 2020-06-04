#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpcpp/opencensus.h>

#include "exporters.h"
#include "foodsystem.grpc.pb.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"

/* 
*  This class implements the FoodSupplier service. We only implement the
*  the 'GetSuppliers' method of the FoodSystem Service. This will be invoked
*  by the running gRPC server.
*/
class FoodSupplier final : public foodsystem::FoodSystem::Service {
public:

  /*
  * Fetches a list of potential suppliers who have a certain user specified ingredient
  * 
  * @param context - Context object which carries scoped values and propagates states between the client and server
  * @param request - Contains the user's request, i.e., the ingredient to look for
  * @param reply - Server generated reply which contains the list of potential suppliers
  * @return grpc::Status::OK - A field which tells us that the operation completed successfully
  */
  grpc::Status GetSuppliers(grpc::ServerContext* context,
                        const foodsystem::Ingredient* request,
                        foodsystem::SupplierList* reply) override {
    
    // opencensus::trace::Span span = grpc::GetSpanFromServerContext(context);
    // span.AddAttribute("my_attribute", "blue");
    // span.AddAnnotation("Fetching list of potential suppliers");

    // Fetch the suppliers which have the user-specified ingredient
    auto it = suppliers.begin();
    for(;it!=suppliers.end();it++){
      std::vector<std::string>& ingredients = it->second;
      for(auto& ingredient: ingredients){
        if(ingredient == request->name()){
          reply->add_items(it->first);
          break;
        }
      }
    }
        
    return grpc::Status::OK;
  }

private:
  // Statically stored database of suppliers and their respective inventory 
  std::map<std::string, std::vector<std::string>> suppliers = {{"Amazon", {"onion", "tomato"}},
                                                               {"Walmart", {"onion", "eggs", "milk"}}, 
                                                               {"Costco", {"eggs", "potato"}}};

};

void RunServer() {
  // The server address of the form "address:port"
  std::string server_address("0.0.0.0:50051");
  FoodSupplier service;

  // Register the OpenCensus gRPC plugin to enable stats and tracing in gRPC.
  grpc::RegisterOpenCensusPlugin();

  RegisterExporters();

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