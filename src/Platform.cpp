
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <common.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpcconnect::ConnectService;
using grpcconnect::DesireTopic;
using grpcconnect::Futures;
using grpcconnect::PlatformReply;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public ConnectService::Service {
public:
  Status Produce(ServerContext *context, ::grpc::ServerReader<Futures> *reader,
                 PlatformReply *response) override {
    return Status::OK;
  }
  Status Subscribe(ServerContext *context, const DesireTopic *request,
                   ::grpc::ServerWriter<Futures> *writer) override {
    context->set_compression_algorithm(
        grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);
    return Status::OK;
  }

private:
  std::queue<Futures> m_message_queue;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  RunServer();

  return 0;
}
