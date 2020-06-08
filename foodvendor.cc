#include "foodvendor.h"


grpc::Status FoodVendor::GetInfoFromVendor(grpc::ServerContext* context,
                        const foodsystem::PriceRequest* request,
                        foodsystem::PriceInfo* reply) {
                          
    // Fetch the price of the ingredient from the vendor
    reply->set_price(inventory[request->vendor()][request->ingredient()]);

    return grpc::Status::OK;
}


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