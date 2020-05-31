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
    void GetVendors(const std::string & ingredient) {
        Ingredient request;
        request.set_name(ingredient);

        List reply;

       
        ClientContext context;

        Status status = stub_->GetVendors(&context, request, &reply);

        if (status.ok()) {
            std::cout << "gRPC returned:\n";

            if(!((int)reply.items_size())){
                std::cout << "No vendors have " << ingredient << "." << std::endl;
            } else {
                for(int i = 0; i < (int)reply.items_size(); i++){
                    std::cout << reply.items(i) << std::endl;
                }
            }
            std::cout << std::endl;
        } 
        else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            std::cout << "gRPC failed" << std::endl;
        }
    }

private:
    std::unique_ptr<FoodSupplier::Stub> stub_;
};

void rungrpc() {
    
    FoodFinder foodfinder(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    while (true) {
        std::string ingredient;
        std::cout << "Please enter your ingredient:" << std::endl;
        std::getline(std::cin, ingredient);
        foodfinder.GetVendors(ingredient);
        // if (reply == "gRPC failed") {
        //     std::cout << "gRPC failed" << std::endl;
        // }
        // std::cout << "gRPC returned: " << std::endl;
        // std::cout << reply << std::endl;
    }
}


int main() {

    rungrpc();

    return 0;
}
