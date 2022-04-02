
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include <grpcconnect.grpc.pb.h>
#include <grpcconnect.pb.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpcconnect::ConnectService;
using grpcconnect::Futures;
// using grpcconnect::DesireTopic;
using grpcconnect::PlatformReply;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(ConnectService::NewStub(channel)) {}

  std::string SayHello(const std::string& user) {
    Futures request;
    request.set_data(user);
    PlatformReply reply;
    ClientContext context;

    auto writer = stub_->Produce(&context, &reply);

    for(;;){
        auto result = writer->Write(request);
    }

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
  GreeterClient greeter(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = greeter.SayHello(user);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
