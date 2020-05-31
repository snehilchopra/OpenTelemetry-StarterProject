#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "foodsystem.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using foodsupplier::List;
using foodsupplier::Ingredient;
using foodsupplier::FoodSupplier;

class FoodFinder {
public:
    FoodFinder(std::shared_ptr<Channel> channel): stub_(FoodSupplier::NewStub(channel)) {}
    std::string GetVendors(const std::string & ingredient) {
        // Data we are sending to the server.
        Ingredient request;
        request.set_name(ingredient);

        // Container for the data we expect from the server.
        List reply;

        // Context for the client. 
        // It could be used to convey extra information to the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->GetVendors(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            return reply.items();
        } 
        else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return "gRPC failed";
        }
    }

private:
    std::unique_ptr<FoodSupplier::Stub> stub_;
};

void InterativeGRPC() {
    // Instantiate the client. It requires a channel, out of which the actual RPCs are created. 
    // This channel models a connection to an endpoint (in this case, localhost at port 50051). 
    // We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
    FoodFinder foodfinder(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    while (true) {
        std::string ingredient;
        std::cout << "Please enter your ingredient:" << std::endl;
        // std::cin >> user;
        std::getline(std::cin, ingredient);
        std::string reply = foodfinder.GetVendors(ingredient);
        if (reply == "gRPC failed") {
            std::cout << "gRPC failed" << std::endl;
        }
        std::cout << "gRPC returned: " << std::endl;
        std::cout << reply << std::endl;
    }
}


int main() {

    InterativeGRPC();

    return 0;
}
