#include "foodsupplier.h"

grpc::Status FoodSupplier::GetSuppliers(grpc::ServerContext* context,
                        const foodsystem::Ingredient* request,
                        foodsystem::SupplierList* reply) {

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
        
    // Randomize rpc errors
    if(rand() % 10 + 1 >= 8){
      return grpc::Status::CANCELLED;
    } else {
      return grpc::Status::OK;
    }
}


void RunServer() {
  // The server address of the form "address:port"
  std::string server_address("0.0.0.0:9001");
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
