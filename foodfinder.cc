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
#include <stdlib.h>

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
using foodsystem::PriceInfo;
using foodsystem::PriceRequest;


/*
* Adds synthetic delays to better simulate processing time
* and view more descriptive traces on GCP.
*
* @param parent_span - The span of which we make a child and add a small delay for descriptive results
* @param sampler - An Always sampler which exports/process every span
* @param delay - Amount of delay ( Range is [0,20] )
*/
void AddDelay(opencensus::trace::Span* parent_span, opencensus::trace::AlwaysSampler* sampler, int delay){
    auto child_span = opencensus::trace::Span::StartSpan("Delay Span", parent_span, {sampler});
    child_span.AddAnnotation("delay");
    absl::SleepFor(absl::Milliseconds(delay));  // Working hard here.
    child_span.End();
}

/*
* Fetches list of suppliers who have a user-specified ingredient.
*
* @param ingredient - The user specified ingredient 
* @param stub - The FoodSystem stub used to send RPCs
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
* @param ingredient - The user specified ingredient
* @param vendors - List of vendors who have the user specified ingredient
* @param parent_span - The span of which we create child spans for each RPC
* @param stub - FoodSystem stub used to send RPCs to FoodVendor service
* @return prices - the map which will hold the {key, value} pairs of the form {vendor, price of the ingredeint}
*/
std::unordered_map<std::string, double> GetInfoFromVendor(std::string& ingredient,
                        std::vector<std::string>& vendors,
                        opencensus::trace::Span& parent_span,
                        opencensus::trace::AlwaysSampler& sampler,
                        std::unique_ptr<FoodSystem::Stub>& stub){

    // Declare the map which will hold the {key, value} pairs
    // of the form {vendor, price of the ingredeint}
    std::unordered_map<std::string, double> prices;

    // Declare request and reply variables to use for all RPCs 
    PriceRequest price_request;
    PriceInfo price_info;

    // Get price info from each vendor
    for(std::string& vendor: vendors){
        // Set up request
        price_request.set_vendor(vendor);
        price_request.set_ingredient(ingredient);

        ClientContext context;

        // Add a span to monitor the RPC
        opencensus::trace::Span span = opencensus::trace::Span::StartSpan("Fetching price info from " + vendor, &parent_span, {&sampler});
        span.AddAnnotation("Fetching price info from " + vendor);

        // Add random, synthetic delay  
        AddDelay(&span, &sampler, rand() % 20 + 1);

        // Send the rpc
        Status status = stub->GetInfoFromVendor(&context, price_request, &price_info);

        // End the span
        span.End();

        // Add to map
        prices[vendor] = price_info.price();
    }
    
    // Print results
    std::cout << "----------------------------\n";
    std::cout << "Vendor\t|\tPrice\n";
    std::cout << "----------------------------\n";
    auto it = prices.begin();
    for(;it != prices.end(); it++){
        std::cout << it->first << "\t|\t$" << it->second << std::endl;
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
	std::shared_ptr<grpc::Channel> foodsupplier_channel = grpc::CreateChannel("127.0.0.1:9001", grpc::InsecureChannelCredentials());	
	std::unique_ptr<FoodSystem::Stub> foodsupplier_stub = FoodSystem::NewStub(foodsupplier_channel);

    // Create a channel to the FoodVendor service to send RPCs over.
    std::shared_ptr<grpc::Channel> foodvendor_channel = grpc::CreateChannel("127.0.0.1:9002", grpc::InsecureChannelCredentials());	
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
        auto fs_span = opencensus::trace::Span::StartSpan("Fetching Suppliers", &system_span, {&sampler});
        fs_span.AddAnnotation("Sending request for fetching suppliers.");

        // Add synthetic delay
        AddDelay(&fs_span, &sampler, 20);

        // Get list of potential suppliers
        std::vector<std::string> suppliers = GetSuppliers(ingredient, foodsupplier_stub);
        
        // End the current span
        fs_span.End();
        

        
        // Create a span for tracing the RPC from Foodfinder to FoodVendor service
        auto fv_span = opencensus::trace::Span::StartSpan("Fetching inventory info from vendors.", &system_span, {&sampler});
        fv_span.AddAnnotation("Fetching inventory info from vendors.");
        
        // Add synthetic delay
        AddDelay(&fv_span, &sampler, 20);

        // Fetch inventory info from vendors
        GetInfoFromVendor(ingredient, suppliers, fv_span, sampler, foodvendor_stub);

        // End the current span
        fv_span.End();

        // End the system span
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


int main(int argc, char** argv) {
    RungRPC();
    return 0;
}
