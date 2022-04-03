
#include <iostream>
#include <memory>
#include <string>

#include <common.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpcconnect::ConnectService;
using grpcconnect::Futures;
// using grpcconnect::DesireTopic;
using grpcconnect::PlatformReply;

class Producer {
public:
  Producer(std::shared_ptr<Channel> channel)
      : stub_(ConnectService::NewStub(channel)) {}

  std::string SendFuturePrice(const std::string &price) {
    Futures request;
    request.set_data(price);
    PlatformReply reply;
    ClientContext context;

    context.set_compression_algorithm(
        grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);

    auto writer = stub_->Produce(&context, &reply);

    for (int i = 0; i < 5; i++) {
      auto result = writer->Write(request);
      if (!result) {
        std::cerr << "Producer write error" << std::endl;
      }
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<ConnectService::Stub> stub_;
};

int main(int argc, char** argv) {
  Producer producer(grpc::CreateChannel("localhost:50051",
                                        grpc::InsecureChannelCredentials()));
  std::string future_price("future-100");
  std::string reply = producer.SendFuturePrice(future_price);
  std::cout << "producer received: " << reply << std::endl;
  return 0;
}
