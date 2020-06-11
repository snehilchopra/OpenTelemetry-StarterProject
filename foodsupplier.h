#ifndef FOOD_SUPPLIER_H
#define FOOD_SUPPLIER_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpcpp/opencensus.h>

#include "exporters.h"
#include "foodsystem.grpc.pb.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"


/* 
* This class implements the FoodSupplier service. We only implement the
* the 'GetSuppliers' method of the FoodSystem Service. This will be invoked
* by the running gRPC server.
*/
class FoodSupplier final : public foodsystem::FoodSystem::Service {
public:

  /*
  * Fetches a list of potential suppliers who have a certain user specified ingredient
  * 
  * @param context - Context object which carries scoped values and propagates states between the client and server
  * @param request - Contains the user's request, i.e., the ingredient to look for
  * @param reply - Server generated reply which contains the list of potential suppliers
  * @return grpc::Status::OK - A field which tells us that the operation completed successfully
  */
  grpc::Status GetSuppliers(grpc::ServerContext* context,
                        const foodsystem::Ingredient* request,
                        foodsystem::SupplierList* reply) override;

private:
  /* Statically stored database of suppliers and their respective inventory */
  std::map<std::string, std::vector<std::string>> suppliers = {{"Amazon", {"onion", "tomato", "cheese", "eggs", "mango"}},
                                                               {"Walmart", {"onion", "eggs", "milk", "orange"}}, 
                                                               {"Costco", {"eggs", "potato", "cheese", "tomato", "avocado"}},
                                                               {"Bazaar", {"onion", "milk", "potato", "orange"}},
                                                               {"Safeway", {"orange", "cheese", "avocado"}}
                                                               };

};

/*
* Runs the gRPC Server
*/
void RunServer();


#endif