#ifndef FOOD_FINDER_H
#define FOOD_FINDER_H

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
#include "opencensus/trace/sampler.h"
#include "opencensus/stats/aggregation.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/view_descriptor.h"
#include "opencensus/tags/tag_key.h"
#include "opencensus/stats/stats.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using foodsystem::SupplierList;
using foodsystem::Ingredient;
using foodsystem::FoodSystem;
using foodsystem::PriceInfo;
using foodsystem::PriceRequest;

/* ############################################################################ */
/* ################################# METRICS ################################## */
/* ############################################################################ */

auto key1 = opencensus::tags::TagKey::Register("key1");

/* ############################ RPC LATENCY METRIC ########################## */
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
    .add_column(key1)
    .set_description("Latency for the RPCs");

/*############################# RPC ERRORS METRIC ##########################*/
ABSL_CONST_INIT const absl::string_view rpc_errors_measure_name = "rpc errors count";

const opencensus::stats::MeasureInt64 rpc_errors_measure = 
     opencensus::stats::MeasureInt64::Register(rpc_errors_measure_name , 
                                                "Rpc Errors Count", 
                                                "errors");

const auto rpc_errors_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/rpc_errors")
    .set_measure(rpc_errors_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Count())
    .add_column(key1)
    .set_description("Cumulative count of RPC errors");

/*############################# RPC COUNT METRIC ############################*/
ABSL_CONST_INIT const absl::string_view rpc_count_measure_name = "rpc count";

const opencensus::stats::MeasureInt64 rpc_count_measure = 
     opencensus::stats::MeasureInt64::Register(rpc_count_measure_name , 
                                                "Total rpc calls made", 
                                                "rpcs");

const auto rpc_count_view_descriptor = opencensus::stats::ViewDescriptor()
    .set_name("food_finder/rpc_count")
    .set_measure(rpc_count_measure_name)
    .set_aggregation(opencensus::stats::Aggregation::Count())
    .add_column(key1)
    .set_description("Cumulative count of RPCs");


/* ############################################################################ */
/* ############################ HELPER FUNCTIONS ############################## */
/* ############################################################################ */

/*
* Adds synthetic delays to better simulate processing time
* and view more descriptive traces on GCP.
*
* @param parent_span - The span of which we make a child and add a small delay for descriptive results
* @param sampler - An Always sampler which exports/process every span
* @param delay - Amount of delay ( Range is [0,20] )
*/
void AddDelay(opencensus::trace::Span* parent_span, opencensus::trace::AlwaysSampler* sampler, int delay);


/*
* Fetches list of suppliers who have a user-specified ingredient.
*
* @param ingredient - The user specified ingredient 
* @param stub - The FoodSystem stub used to send RPCs
* @return suppliers - The list of suppliers who have the user specified ingredient
*/
std::vector<std::string> GetSuppliers(std::string& ingredient,
                                      std::unique_ptr<FoodSystem::Stub>& stub);


/*
* Fetches price of the ingredient from each vendor which has the user
* specified ingredient.
*   
* @param ingredient - The user specified ingredient
* @param vendors - List of vendors who have the user specified ingredient
* @param parent_span - The span of which we create child spans for each RPC
* @param stub - FoodSystem stub used to send RPCs to FoodVendor service
*/
void GetInfoFromVendor(std::string& ingredient,
                        std::vector<std::string>& vendors,
                        opencensus::trace::Span& parent_span,
                        opencensus::trace::AlwaysSampler& sampler,
                        std::unique_ptr<FoodSystem::Stub>& stub);


/*
* Runs the main gRPC procedure
*/
void RungRPC();


#endif