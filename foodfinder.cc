// Copyright 2018, OpenCensus Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpcpp/opencensus.h>

#include "foodsystem.grpc.pb.h"

#include "exporters.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/stats/aggregation.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/view_descriptor.h"
#include "opencensus/tags/context_util.h"
#include "opencensus/tags/tag_key.h"
#include "opencensus/tags/tag_map.h"
#include "opencensus/stats/stats.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using foodsystem::SupplierList;
using foodsystem::Ingredient;
using foodsystem::FoodSystem;
using foodsystem::IngredientInfo;
using foodsystem::InventoryInfo;
using foodsystem::ItemInfo;


/*
* Adds synthetic delays to better simulate processing time
* and view traces on GCP.
*
* @param parent_span - The span of which we make a child and add a small delay for descriptive results
* @param sampler - An Always sampler which exports/process every span
*/
void AddDelay(opencensus::trace::Span* parent_span, opencensus::trace::AlwaysSampler* sampler){
    auto child_span = opencensus::trace::Span::StartSpan("Delay Span", parent_span, {sampler});
    child_span.AddAnnotation("delay");
    absl::SleepFor(absl::Milliseconds(20));  // Working hard here.
    child_span.End();
}

/*
* Fetches list of suppliers who have a user-specified ingredient.
*
* @param ingredient - The user specified ingredient 
* @return suppliers - The list of suppliers who have the user specified ingredient
*/
std::vector<std::string> GetSuppliers(std::string& ingredient,
                                      std::unique_ptr<FoodSystem::Stub>& stub){
    // Set up the request to send to FoodSupplier service
    Ingredient request_fs;
    request_fs.set_name(ingredient);

    // This will fetch results from the FoodSupplier service
    SupplierList reply_fs;
    
    ClientContext context_fs;

    // Send the RPC
    Status status_fs = stub->GetSuppliers(&context_fs, request_fs, &reply_fs);

    std::vector<std::string> suppliers;

    std::cout << "SEARCH RESULTS FOR " << ingredient << "\n\n";

    // Check if we got any suppliers and accordingly append to the 'suppliers' vector
    if(!(int)reply_fs.items_size()) {
        std::cout << "No suppliers have " << ingredient << std::endl << std::endl;
        return {};
    } else {
        for(int i = 0; i < (int)reply_fs.items_size(); i++){
            suppliers.push_back(reply_fs.items(i));
        } 
    }

    return suppliers;
}

/*
* Fetches price of the ingredient from each vendor which has the user
* specified ingredient.
*
*/
void GetInfoFromVendors(std::string& ingredient,
                        std::vector<std::string>& vendors,
                        std::unique_ptr<FoodSystem::Stub>& stub){
    // Set up the request to send to the FoodVendor service
    IngredientInfo request_fv;
    request_fv.set_name(ingredient);
    for(auto& vendor:vendors){
        request_fv.add_vendors(vendor);
    }

    // This fetches the results from the FoodVendor service
    InventoryInfo reply_fv;
    
    ClientContext context_fv;

    // Send the RPC
    Status status_fv = stub->GetInfoFromVendors(&context_fv, request_fv, &reply_fv);
    
    // Print results
    std::cout << "----------------------------\n";
    std::cout << "Vendor\t|\tPrice\n";
    std::cout << "----------------------------\n";
    int size = reply_fv.iteminfo_size();
    for(int i = 0; i < size; i++){
        std::cout << reply_fv.iteminfo(i).vendor() << "\t|\t$" << reply_fv.iteminfo(i).price() << std::endl;
    }
    std::cout << std::endl;
}


/*
* Runs the main gRPC procedure
*/
void RungRPC() {
   
    // Register the OpenCensus gRPC plugin to enable stats and tracing in gRPC.
	grpc::RegisterOpenCensusPlugin();

	RegisterExporters();

    // Create a channel to the FoodSupplier service to send RPCs over.
	std::shared_ptr<grpc::Channel> foodsupplier_channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());	
	std::unique_ptr<FoodSystem::Stub> foodsupplier_stub = FoodSystem::NewStub(foodsupplier_channel);

    // Create a channel to the FoodVendor service to send RPCs over.
    std::shared_ptr<grpc::Channel> foodvendor_channel = grpc::CreateChannel("localhost:9008", grpc::InsecureChannelCredentials());	
    std::unique_ptr<FoodSystem::Stub> foodvendor_stub = FoodSystem::NewStub(foodvendor_channel);

    // Setup Always sampler so that every span is processed and exported
    static opencensus::trace::AlwaysSampler sampler;	

    while(true){
        // Get user specified ingredient
        std::string ingredient;
        std::cout << "Please enter your ingredient (press x to quit):" << std::endl;
        std::getline(std::cin, ingredient);
        if(ingredient == "x") break;

        // This is a parent span which spans both RPCs sent to FoodSupplier and FoodVendor services 
        auto system_span = opencensus::trace::Span::StartSpan("System span", nullptr, {&sampler});
        system_span.AddAnnotation("Start RPC service");

        // Create a span for tracing the RPC from Foodfinder to FoodSupplier service
        auto fs_span = opencensus::trace::Span::StartSpan("foodsupplier", &system_span, {&sampler});
        fs_span.AddAnnotation("Sending request for fetching suppliers.");

        // Add synthetic delay
        AddDelay(&fs_span, &sampler);

        // Get list of potential suppliers
        std::vector<std::string> suppliers = GetSuppliers(ingredient, foodsupplier_stub);
        
        // End the current span
        fs_span.End();

        
        // Create a span for tracing the RPC from Foodfinder to FoodVendor service
        auto fv_span = opencensus::trace::Span::StartSpan("foodvendor", &system_span, {&sampler});
        fv_span.AddAnnotation("Fetching inventory info from vendors.");
        
        // Add synthetic delay
        AddDelay(&fv_span, &sampler);

        // Fetch inventory info from vendors
        GetInfoFromVendors(ingredient, suppliers, foodvendor_stub);

        // End the current span
        fv_span.End();

        // End this rpc
        system_span.End();
    }

    // Disable tracing any further RPCs (which will be sent by exporters).
    opencensus::trace::TraceConfig::SetCurrentTraceParams(
        {128, 128, 128, 128, opencensus::trace::ProbabilitySampler(0.0)});

    // Sleep while exporters run in the background.
    std::cout << "Client sleeping, ^C to exit.\n";
    while (true) {
        absl::SleepFor(absl::Seconds(10));
    }
    
}


int main() {
    RungRPC();
    return 0;
}
