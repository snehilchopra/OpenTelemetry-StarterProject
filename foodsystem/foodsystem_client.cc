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
        Ingredient request;
        request.set_name(ingredient);

        List reply;

       
        ClientContext context;

        Status status = stub_->GetVendors(&context, request, &reply);

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

void grpc() {
    
    FoodFinder foodfinder(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    while (true) {
        std::string ingredient;
        std::cout << "Please enter your ingredient:" << std::endl;
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

    grpc();

    return 0;
}
