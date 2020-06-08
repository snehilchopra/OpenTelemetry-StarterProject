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

#include "foodfinder.h"

void AddDelay(opencensus::trace::Span* parent_span, opencensus::trace::AlwaysSampler* sampler, int delay){
    auto child_span = opencensus::trace::Span::StartSpan("Delay Span", parent_span, {sampler});
    child_span.AddAnnotation("delay");
    absl::SleepFor(absl::Milliseconds(delay));  // Working hard here.
    child_span.End();
};


std::vector<std::string> GetSuppliers(std::string& ingredient,
                                      std::unique_ptr<FoodSystem::Stub>& stub){
    // Set up the request to send to FoodSupplier service
    Ingredient request_fs;
    request_fs.set_name(ingredient);

    // This will fetch results from the FoodSupplier service
    SupplierList reply_fs;
    
    ClientContext context_fs;

    // Get current time (used for measuring latency of rpc)
    absl::Time start = absl::Now();

    // Send the RPC
    Status status = stub->GetSuppliers(&context_fs, request_fs, &reply_fs);

    // Get current time (used for measuring latency of rpc)
    absl::Time end = absl::Now();
    double latency = absl::ToDoubleMilliseconds(end - start);

    // Record data for metrics
    opencensus::stats::Record({{rpc_count_measure, 1}});
    opencensus::stats::Record({{rpc_latency_measure, latency}});

    if(!status.ok()){
        opencensus::stats::Record({{rpc_errors_measure, 1}});
    }

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


void GetInfoFromVendor(std::string& ingredient,
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

        absl::Time start = absl::Now();

        // Send the rpc
        Status status = stub->GetInfoFromVendor(&context, price_request, &price_info);

        absl::Time end = absl::Now();
        double latency = absl::ToDoubleMilliseconds(end - start);

        if(!status.ok()){
            opencensus::stats::Record({{rpc_errors_measure, 1}});
        }
        // Record data for metrics
        opencensus::stats::Record({{rpc_count_measure, 1}});
        opencensus::stats::Record({{rpc_latency_measure, latency}});

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

    rpc_errors_view_descriptor.RegisterForExport();
    rpc_count_view_descriptor.RegisterForExport();
    rpc_latency_view_descriptor.RegisterForExport();

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
