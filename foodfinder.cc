/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "foodfinder.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using foodsystem::SupplierList;
using foodsystem::Ingredient;
using foodsystem::FoodSystem;
using foodsystem::PriceInfo;
using foodsystem::PriceRequest;


/* ############################################################################ */
/* ################################# METRICS ################################## */
/* ############################################################################ */

opencensus::tags::TagKey status_key = opencensus::tags::TagKey::Register("Status");

/* ---------------------------- RPC LATENCY METRIC ---------------------------- */
ABSL_CONST_INIT const absl::string_view rpc_latency_measure_name = "rpc latency";

const opencensus::stats::MeasureDouble rpc_latency_measure = 
     opencensus::stats::MeasureDouble::Register(rpc_latency_measure_name , 
                                                "Latency measure for rpc calls", 
                                                "ms");

const auto rpc_latency_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/rpc_latency")
    .set_measure(rpc_latency_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Distribution(
                opencensus::stats::BucketBoundaries::Explicit(
                    {0, 7.5, 15, 22.5, 30, 37.5, 45, 52.5, 60, 67.5})))
    .add_column(status_key)
    .set_description("Latency for the RPCs");

/* ---------------------------- RPC ERRORS METRIC ----------------------------- */
ABSL_CONST_INIT const absl::string_view rpc_errors_measure_name = "rpc errors count";

const opencensus::stats::MeasureInt64 rpc_errors_measure = 
     opencensus::stats::MeasureInt64::Register(rpc_errors_measure_name , 
                                                "Rpc Errors Count", 
                                                "errors");

const auto rpc_errors_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/rpc_errors")
    .set_measure(rpc_errors_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Count())
    .add_column(status_key)
    .set_description("Cumulative count of RPC errors");

/* ---------------------------- RPC COUNT METRIC ------------------------------ */
ABSL_CONST_INIT const absl::string_view rpc_count_measure_name = "rpc count";

const opencensus::stats::MeasureInt64 rpc_count_measure = 
     opencensus::stats::MeasureInt64::Register(rpc_count_measure_name , 
                                                "Total rpc calls made", 
                                                "rpcs");

const auto rpc_count_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/rpc_count")
    .set_measure(rpc_count_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Count())
    .add_column(status_key)
    .set_description("Cumulative count of RPCs");


/* -------------------------- SUPPLIERS PER QUERY METRIC --------------------------- */
ABSL_CONST_INIT const absl::string_view suppliers_per_query_measure_name = "Suppliers per query";

const opencensus::stats::MeasureInt64 suppliers_per_query_measure = 
     opencensus::stats::MeasureInt64::Register(suppliers_per_query_measure_name, 
                                                "Suppliers per query/rpc", 
                                                "suppliers");

const auto suppliers_per_query_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/suppliers_per_query")
    .set_measure(suppliers_per_query_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Distribution(
                opencensus::stats::BucketBoundaries::Explicit(
                {0,1,2,3,4,5,6,7,8,9,10})))
    .add_column(status_key)
    .set_description("Distribution of suppliers per query");


/* ############################################################################ */
/* ####################### FUNCTION IMPLEMENTATIONS ########################### */
/* ############################################################################ */


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
    const absl::Time start = absl::Now();

    // Send the RPC
    const Status status = stub->GetSuppliers(&context_fs, request_fs, &reply_fs);

    // Get current time (used for measuring latency of rpc)
    const absl::Time end = absl::Now();
    const double latency = absl::ToDoubleMilliseconds(end - start);

    // Record data for metrics
    opencensus::stats::Record({{rpc_count_measure, 1}}, {{status_key, !status.ok() ? "Error" : "OK"}});
    opencensus::stats::Record({{rpc_latency_measure, latency}}, {{status_key, !status.ok() ? "Error" : "OK"}});
    opencensus::stats::Record({{suppliers_per_query_measure, reply_fs.items_size()}}, {{status_key, !status.ok() ? "Error" : "OK"}});

    if(!status.ok()){
        opencensus::stats::Record({{rpc_errors_measure, 1}});
        std::cout << "Error while fetching suppliers" << std::endl;
        return {};
    }

    std::vector<std::string> suppliers;

    std::cout << "SEARCH RESULTS FOR " << ingredient << "\n\n";

    // Check if we got any suppliers and accordingly populate 'suppliers' vector
    if(reply_fs.items_size() == 0) {
        std::cout << "No suppliers have " << ingredient << std::endl << std::endl;
    } else {
        for(const std::string& vendor: reply_fs.items()){
            suppliers.push_back(vendor);
        } 
    }

    return suppliers;
}


void GetInfoFromVendors(const std::string& ingredient,
                        const std::vector<std::string>& vendors,
                        opencensus::trace::Span& parent_span,
                        opencensus::trace::AlwaysSampler& sampler,
                        const std::unique_ptr<FoodSystem::Stub>& stub){

    // Declare the map which will hold the {key, value} pairs
    // of the form {vendor, price of the ingredeint}
    std::unordered_map<std::string, double> prices;

    // Map to hold the PriceInfo object s
    std::unordered_map<std::string, PriceInfo> price_infos;

    // Map to hold the trace spans for each vendor's RPC
    std::unordered_map<std::string, opencensus::trace::Span> spans;

    // The producer-consumer queue for asnchronous notifications
    CompletionQueue cq;

    // A tag struct which contains the name of the vendor and the time when the rpc
    // was made to that vendor the fetch price info
    struct tag {
        std::string vendor;
        absl::Time start_time;
        tag(std::string vendor, absl::Time start_time) : vendor(vendor), start_time(start_time) {}
    };

    // Get price info from each vendor
    for(const std::string& vendor: vendors){
        PriceRequest price_request;
        PriceInfo price_info;

        // Get a reference to the PriceInfo object which will be populated by the server
        // and later used to check the price in the client
        price_infos[vendor] = price_info;        

        // Set up request
        price_request.set_vendor(vendor);
        price_request.set_ingredient(ingredient);

        Status status;
        ClientContext context;        

        // Create rpc object
        std::unique_ptr<ClientAsyncResponseReader<PriceInfo>> rpc(stub->PrepareAsyncGetInfoFromVendor(&context, price_request, &cq));

        // Begin the span for the current vendor
        opencensus::trace::Span span = opencensus::trace::Span::StartSpan("Fetching price info from " + vendor, &parent_span, {&sampler});
        span.AddAnnotation("Fetching price info from " + vendor);

        // ERROR: This type of insertion is needed because the default 'Span()' constructor is a deleted function
        //        and maps need a default constructor if 'map[key] = value' type insertion is wanted
        // spans[vendor] = span;

        // Add to map using insert, which does not need a default constructor
        spans.insert({{vendor, span}});

        // Start time
        const absl::Time start_time = absl::Now();

        // Initiate the rpc call
        rpc->StartCall();

        // Request that, upon completion of the RPC, "reply" be updated with the
        // server's response; "status" with the indication of whether the operation
        // was successful. Tag the request with the vendor's name.
        // rpc->Finish(&price_infos[vendor], &status, static_cast<void*>(new std::string(vendor)));
        rpc->Finish(&price_infos[vendor], &status, static_cast<void*>(new tag(vendor, start_time)));
    }


    void* got_tag;
    bool ok = false;
    int counter = vendors.size();

    // Keep looping till we have not received response for all vendors
    // and then block till we get the next result in the completion queue
    while(counter && cq.Next(&got_tag, &ok)){
        // Get current tag
        std::unique_ptr<tag> curr_tag((tag*)got_tag);

        // Extract info from the current tag
        std::string& vendor = curr_tag->vendor;
        absl::Time& start_time = curr_tag->start_time;

        // Measure latency for receiving info from this particular vendor
        const absl::Time end = absl::Now();
        double latency = absl::ToDoubleMilliseconds(end - start_time);

        // Record data for metrics
        opencensus::stats::Record({{rpc_count_measure, 1}}, {{status_key, ok ? "Error" : "OK"}});
        opencensus::stats::Record({{rpc_latency_measure, latency}}, {{status_key, ok ? "Error" : "OK"}});

        if(!ok){
            opencensus::stats::Record({{rpc_errors_measure, 1}});
        }

        // Find the current vendor's corresponding span and end it
        auto it = spans.find(vendor);
        it->second.End();

        // ERROR: The line of code below produces an error because the default constructor 
        //        opencensus::trace::Span() has been marked as delete
        // spans[*vendor].End();

        // Decrement counter to keep track of how many vendors we have received price info for
        counter--;

        // Add to prices map for displaying results at the end
        prices[vendor] = price_infos[vendor].price();
    }

    cq.Shutdown();
    
    // Print results
    std::cout << "----------------------------\n";
    std::cout << "Vendor\t|\tPrice\n";
    std::cout << "----------------------------\n";
    auto it = prices.begin();
    for(;it != prices.end(); it++){
        if(it->second)
            std::cout << it->first << "\t|\t$" << it->second  << std::endl;
        else
            std::cout << it->first << "\t|\t" << "Error" << std::endl;
    }

    std::cout << std::endl;
}


void RungRPC() {
    // Register the OpenCensus gRPC plugin to enable stats and tracing in gRPC.
    grpc::RegisterOpenCensusPlugin();
    RegisterExporters();

    rpc_errors_view_descriptor.RegisterForExport();
    rpc_count_view_descriptor.RegisterForExport();
    rpc_latency_view_descriptor.RegisterForExport();
    suppliers_per_query_view_descriptor.RegisterForExport();

    // Create a channel to the FoodSupplier service to send RPCs over.
    std::shared_ptr<grpc::Channel> foodsupplier_channel = grpc::CreateChannel("foodsupplier:9001", grpc::InsecureChannelCredentials());
    std::unique_ptr<FoodSystem::Stub> foodsupplier_stub = FoodSystem::NewStub(foodsupplier_channel);

    // Create a channel to the FoodVendor service to send RPCs over.
    std::shared_ptr<grpc::Channel> foodvendor_channel = grpc::CreateChannel("foodvendor:9002", grpc::InsecureChannelCredentials());	
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
        AddDelay(&fs_span, &sampler, (rand() % 20) + 1);

        // Get list of potential suppliers
        std::vector<std::string> suppliers = GetSuppliers(ingredient, foodsupplier_stub);
        
        // End the current span
        fs_span.End();
        


        // Create a span for tracing the RPC from Foodfinder to FoodVendor service
        auto fv_span = opencensus::trace::Span::StartSpan("Fetching inventory info from vendors.", &system_span, {&sampler});
        fv_span.AddAnnotation("Fetching inventory info from vendors.");
        
        // Add synthetic delay
        AddDelay(&fv_span, &sampler, (rand() % 20) + 1);

        // Fetch inventory info from vendors
        if(suppliers.size())
            GetInfoFromVendors(ingredient, suppliers, fv_span, sampler, foodvendor_stub);        

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
        absl::SleepFor(absl::Seconds(7));
    }
}

int main(int argc, char** argv) {
    RungRPC();
    return 0;
}
