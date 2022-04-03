#include <iostream>
#include <memory>
#include <string>

#include <common.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpcconnect::ConnectService;
using grpcconnect::Futures;
using grpcconnect::DesireTopic;
// using grpcconnect::PlatformReply;

class Consumer {
public:
  Consumer(std::shared_ptr<Channel> channel)
      : stub_(ConnectService::NewStub(channel)) {}

  void Subscribe(const std::string &user) {
    DesireTopic request;
    request.set_topic(user);
    Futures reply;
    ClientContext context;

    context.set_compression_algorithm(
        grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);
    auto reader = stub_->Subscribe(&context, request);

    for(;;){
        auto result = reader->Read(&reply);
        if (result) {
          std::cout << reply.data() << std::endl;
        } else {
          std::cerr << "Consumer read error" << std::endl;
          break;
        }
    }

    Status status = reader->Finish();
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

 private:
  std::unique_ptr<ConnectService::Stub> stub_;
};

int main(int argc, char** argv) {
  Consumer consumer(grpc::CreateChannel("localhost:50051",
                                        grpc::InsecureChannelCredentials()));
  std::string topic("some future");
  consumer.Subscribe(topic);
  return 0;
}
