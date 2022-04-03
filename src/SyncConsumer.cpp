
#include <iostream>
#include <iterator>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <Common.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpcconnect::BroadcastService;
using grpcconnect::Futures;
using grpcconnect::Reply;

class GreeterServiceImpl final : public BroadcastService::Service {
public:
  Status Push(ServerContext *context,
              ::grpc::ServerReader<::grpcconnect::Futures> *reader,
              ::grpcconnect::Reply *response) override {
    context->set_compression_algorithm(
        grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);

    Futures msg;
    while (reader->Read(&msg)) {
      std::cout << "reciever new future " << msg.data() << std::endl;
    }
    return Status::OK;
  }
};

void RunServer(const std::string &endpoint) {
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(endpoint, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << endpoint << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  std::string endpoint = "localhost:" + std::string(argv[1]);
  RunServer(endpoint);

  return 0;
}
