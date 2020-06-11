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

#ifndef FOOD_FINDER_H
#define FOOD_FINDER_H

#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <thread>

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
                                      std::unique_ptr<foodsystem::FoodSystem::Stub>& stub);


/*
* Fetches price of the ingredient from each vendor which has the user
* specified ingredient.
*   
* @param ingredient - The user specified ingredient
* @param vendors - List of vendors who have the user specified ingredient
* @param parent_span - The span of which we create child spans for each RPC
* @param stub - FoodSystem stub used to send RPCs to FoodVendor service
*/
void GetInfoFromVendor(const std::string& ingredient,
                       const std::vector<std::string>& vendors,
                       opencensus::trace::Span& parent_span,
                       opencensus::trace::AlwaysSampler& sampler,
                       std::unique_ptr<foodsystem::FoodSystem::Stub>& stub);


/*
* Runs the main gRPC procedure
*/
void RungRPC();


#endif