# Food System Service
## About
This system consists of three services: 
- The FoodFinder service allows a user to specify an ingredient they need, and consults with other services to try to find vendors with that for the user. 
- The FoodSupplier service acts as a database of FoodVendors, providing a candidate set of vendors who it believes may deal in the ingredient the user is looking for.
- The FoodVendor service offers certain ingredients at certain prices, and maintains an inventory of each.

## Workflow

The order of RPCs that relay between the services can be inferred easily from the flowchart below
![Screenshot 2020-06-10 at 12 16 51 PM](https://user-images.githubusercontent.com/31712484/84308985-498e9c80-ab14-11ea-86ce-fb79d3ab3283.png)


## Screenshots

### Results
![Screenshot 2020-06-10 at 12 23 56 PM](https://user-images.githubusercontent.com/31712484/84309870-9c1c8880-ab15-11ea-8815-7378417b4f1b.png)


### Metrics on Google Cloud
![Screenshot 2020-06-10 at 11 56 39 AM](https://user-images.githubusercontent.com/31712484/84308643-b6edfd80-ab13-11ea-99e8-6cc731b4eebd.png)

### Traces on Google Cloud
![Screenshot 2020-06-10 at 11 57 07 AM](https://user-images.githubusercontent.com/31712484/84308641-b6556700-ab13-11ea-86a1-033492a7b9d2.png)

## Frameworks/Languages Used
- Languages: C++
- Frameworks: gRPC, Protocol Buffers, and Opencensus library.

## How to use?

### Building
To build all 3 services, simply run the following command on the command line:
```
bazel build :all
```

### Running the services
To run the 3 services, open 3 different terminals and run each of the following commands on a separate terminal:
```
sh scripts/foodfinder.sh
sh scripts/foodvendor.sh
sh scripts/foodsupplier.sh
```
**Note** - Make sure to run the FoodSupplier and FoodVendor services BEFORE running the FoodFinder service.

## How to use with Docker?

### Building


```
bazel build :foodsupplier_image.tar
docker load -i ./bazel-bin/foodsupplier_image.tar
bazel build :foodvendor_image.tar
docker load -i ./bazel-bin/foodvendor_image.tar
bazel build :foodfinder_image.tar
docker load -i ./bazel-bin/foodfinder_image.tar
```

### Running


```
docker create network fooddemo
docker run --name foodsupplier --network fooddemo bazel:foodsupplier_image
docker run --name foodvendor --network fooddemo bazel:foodvendor_image
docker run -ti --name foodfinder --network fooddemo bazel:foodfinder_image
```
