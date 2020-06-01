#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "foodsystem.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using foodsystem::SupplierList;
using foodsystem::Ingredient;
using foodsystem::FoodSystem;
using foodsystem::IngredientInfo;
using foodsystem::InventoryInfo;
using foodsystem::ItemInfo;

class FoodFinder {
public:
    FoodFinder(std::shared_ptr<Channel> channel): stub_(FoodSystem::NewStub(channel)) {}
    std::vector<std::string> GetSuppliers(const std::string & ingredient) {
        Ingredient request;
        request.set_name(ingredient);

        SupplierList reply;
       
        ClientContext context;

        Status status = stub_->GetSuppliers(&context, request, &reply);

        if (status.ok()) {

            if(!((int)reply.items_size())){
                return {};

            } else {
                std::vector<std::string> vendors;
                for(int i = 0; i < (int)reply.items_size(); i++){
                    vendors.push_back(reply.items(i));
                }
                return vendors;

            }
        } 
        else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            std::cout << "gRPC failed" << std::endl;
            return {};
        }
    }

    void GetInfoFromVendors(std::string ingredient, std::vector<std::string> vendors) {
        IngredientInfo request;
        request.set_name(ingredient);
        for(auto& vendor:vendors){
            request.add_vendors(vendor);
        }

        InventoryInfo reply;
       
        ClientContext context;

        Status status = stub_->GetInfoFromVendors(&context, request, &reply);

        if (status.ok()) {
            std::cout << "----------------------------\n";
            std::cout << "Vendor\t|\tPrice\n";
            std::cout << "----------------------------\n";
            int size = reply.iteminfo_size();
            for(int i = 0; i < size; i++){
                std::cout << reply.iteminfo(i).vendor() << "\t|\t$" << reply.iteminfo(i).price() << std::endl;
            }
        
            std::cout << std::endl;
        } 
        else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            std::cout << "gRPC failed" << std::endl;
        }
    }

private:
    std::unique_ptr<FoodSystem::Stub> stub_;
};

void rungrpc() {
    
    FoodFinder foodfinder(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    FoodFinder foodfinder2(grpc::CreateChannel("localhost:9008", grpc::InsecureChannelCredentials()));
    while (true) {
        std::string ingredient;
        std::cout << "Please enter your ingredient:" << std::endl;
        std::getline(std::cin, ingredient);
        auto vendors = foodfinder.GetSuppliers(ingredient);
        std::cout << "SEARCH RESULTS FOR " << ingredient << ":\n\n";
        if(vendors.size())
            foodfinder2.GetInfoFromVendors(ingredient, vendors);
        else 
            std::cout << "No vendors have " << ingredient << std::endl << std::endl;
    }
}


int main() {
    rungrpc();
    return 0;
}
