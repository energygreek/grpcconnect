
#include <grpcpp/impl/codegen/client_context.h>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <Common.hpp>

using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
// using grpcconnect::DesireTopic;
using grpcconnect::BroadcastService;
using grpcconnect::Futures;
using grpcconnect::ProduceService;
using grpcconnect::Reply;

struct Consumer {
  ClientContext context;
  Reply response;
  std::unique_ptr<::grpc::ClientWriter<::grpcconnect::Futures>> writer;
};

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public ProduceService::Service {
public:
  GreeterServiceImpl(std::vector<std::shared_ptr<Consumer>> &consumers)
      : m_consumers(consumers) {}
  Status Produce(ServerContext *context, ::grpc::ServerReader<Futures> *reader,
                 Reply *response) override {
    Futures msg;
    while (reader->Read(&msg)) {
      std::cout << "reciever new future " << msg.data() << std::endl;
      for (const auto &consumer : m_consumers) {
        consumer->writer->Write(msg);
      }
    }
    response->set_message("we finish");
    return Status::OK;
  }
  std::vector<std::shared_ptr<Consumer>> &m_consumers;
};

void RunServer(std::vector<std::shared_ptr<Consumer>> &consumers) {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service(consumers);

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

  std::vector<std::string> ports;
  for (int i = 1; i < argc; ++i) {
    ports.emplace_back(argv[i]);
  }

  // Create the stubs
  std::vector<std::shared_ptr<BroadcastService::Stub>> stubs;
  for (auto &it : ports) {
    auto address = "0.0.0.0:" + it;
    auto channel =
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    stubs.push_back(BroadcastService::NewStub(channel));
  }

  std::vector<std::shared_ptr<Consumer>> consumers;
  for (auto &stub : stubs) {
    auto consumer = std::make_shared<Consumer>();
    consumer->context.set_compression_algorithm(
        grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);
    consumer->writer = stub->Push(&consumer->context, &consumer->response);
    consumers.emplace_back(std::move(consumer));
  }
  RunServer(consumers);
  return 0;
}
