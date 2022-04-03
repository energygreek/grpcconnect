#include <grpcconnect.grpc.pb.h>
#include <grpcconnect.pb.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>

#include <grpcpp/server.h>

#include <Common.hpp>
#include <vector>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using grpcconnect::ConnectService;
using grpcconnect::DesireTopic;
using grpcconnect::Futures;
using grpcconnect::Reply;

class ServerImpl final {
public:
  ~ServerImpl() {}

  void Shutdown() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // cq_C = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    std::vector<std::thread> threads;
    for (int i = 0; i < 1; i++) {
      threads.emplace_back([this]() { HandleRpcsFromA(); });
    }
    // 5 for send
    for (int i = 0; i < 1; i++) {
      threads.emplace_back([this]() { HandleRpcsFromC(); });
    }
    for (auto &t : threads) {
      t.join();
    }
  }
  void create_new_a() {
    auto ptr = std::make_shared<CallData_A>(&service_, cq_.get(), this);
    calls.insert({ptr.get(), ptr});
  }
  void create_new_c() {
    auto ptr = std::make_shared<CallData_C>(&service_, cq_.get(), this);
    calls.insert({ptr.get(), ptr});
  }

private:
  static constexpr int CONNECT = 0, READ = 1, WRITE = 2, FINISH = 3, DONE = 4;

  class CallData_Interface {
  public:
    virtual void Proceed(int event, bool ok) = 0;
  };

  class Subscriber_Interface {
  public:
    virtual void AddFuture(const Futures &future) = 0;
  };

  struct Event {
    Event(CallData_Interface *cd, int event) : cd(cd), event(event) {}
    void Process(bool ok) { cd->Proceed(event, ok); };
    CallData_Interface *cd;
    int event;
    bool pending = false;
  };

  class CallData_A : public CallData_Interface {
  public:
    CallData_A(ConnectService::AsyncService *service,
               ServerCompletionQueue *cq_a, ServerImpl *server)
        : service_(service), cq_(cq_a), server(server) {
      read_stream = new grpc::ServerAsyncReader<Reply, Futures>(&ctx_);
      ctx_.AsyncNotifyWhenDone(&doneevent);
      service_->RequestProduce(&ctx_, read_stream, cq_, cq_, &connectevent);

      reply_.set_message("we finish");
    }

    void Proceed(int status_, bool ok) override {
      switch (status_) {
      case CONNECT:
        server->create_new_a();
        read_stream->Read(&request_, &readevent);
        break;
      case READ:
        if (ok) {
          // send to queue and continue read
          std::cout << "get new future " << request_.data() << std::endl;
          server->AddFuture(request_);
          read_stream->Read(&request_, &readevent);
        } else {
          // ok false means stream writedone in peer, so begin reply
          std::cout << "Read done" << std::endl;
          read_stream->Finish(reply_, Status::OK, &finishevent);
        }
        break;
      case DONE:
        // client normally finish or canceled
        if (ctx_.IsCancelled()) {
          std::cout << "Call canceled: " << std::endl;
        }
        break;
      default:
        if (status_ != FINISH) {
          std::cout << "RPC Finish status" << status_ << "ok?" << ok
                    << std::endl;
        }
        server->remove_finished_rpc(this);
      }
    }

  private:
    ConnectService::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;

    grpc::ServerAsyncReader<Reply, Futures> *read_stream;

    Futures request_;
    Reply reply_;

    ServerImpl *server;

    std::mutex mutex_write;
    std::mutex mutex_queue;
    std::queue<std::unique_ptr<Futures>> send_queue;
    // events
    Event connectevent{this, CONNECT};
    Event readevent{this, READ};
    Event writeevent{this, WRITE};
    Event finishevent{this, FINISH};
    Event doneevent{this, DONE};
  };

  class CallData_C : public CallData_Interface, public Subscriber_Interface {
  public:
    CallData_C(ConnectService::AsyncService *service,
               ServerCompletionQueue *cq_c, ServerImpl *server)
        : service_(service), cq_(cq_c), server(server) {
      ctx_.set_compression_algorithm(
          grpc_compression_algorithm::GRPC_COMPRESS_STREAM_GZIP);
      write_stream = new grpc::ServerAsyncWriter<Futures>(&ctx_);

      ctx_.AsyncNotifyWhenDone(&doneevent);
      service_->RequestSubscribe(&ctx_, &request_, write_stream, cq_, cq_,
                                 &connectevent);
    }

    void Proceed(int status_, bool ok) override {
      switch (status_) {
      case CONNECT:
        server->create_new_c();
        std::cout << "new subsribe for" << request_.topic() << std::endl;
        server->Subscribe(this);
        reply();
        break;
      case WRITE:
        writeevent.pending = false;
        if (!ok) {
          std::cout << "write error" << std::endl;
        } else {
          reply();
        }
        break;
      case DONE:
        // client normally finish or canceled
        if (ctx_.IsCancelled()) {
          std::cout << "Call canceled: " << std::endl;
        }
        break;
      default:
        if (status_ != FINISH) {
          std::cout << "RPC Finish status" << status_ << "ok?" << ok
                    << std::endl;
        }
        server->UnSubscribe(this);
        server->remove_finished_rpc(this);
      }
    }

    virtual void AddFuture(const Futures &future) override {
      std::unique_ptr<Futures> new_future(new Futures(future));

      {
        std::lock_guard<std::mutex> lk(mutex_queue);
        future_queue.emplace(std::move(new_future));
      }

      reply();
    }

    void reply() {
      std::unique_ptr<Futures> reply;
      {
        std::lock_guard<std::mutex> lk(mutex_queue);
        if (future_queue.empty() || writeevent.pending || finishevent.pending) {
          return;
        }
        reply = std::move(future_queue.front());
        future_queue.pop();
      }

      if (reply) {
        writeevent.pending = true;
        write_stream->Write(*reply, &writeevent);
      } else {
        finishevent.pending = true;
        write_stream->Finish(Status::OK, &finishevent);
      }
    }

    void finish() {
      {
        std::lock_guard<std::mutex> lk(mutex_queue);
        future_queue.emplace(nullptr);
      }
      // invoke in case no tag in cq
      reply();
    }

  private:
    ConnectService::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;

    grpc::ServerAsyncWriter<Futures> *write_stream;
    DesireTopic request_;
    Futures reply_;

    ServerImpl *server;

    std::mutex mutex_queue;
    std::queue<std::unique_ptr<Futures>> future_queue;

    // events
    Event connectevent{this, CONNECT};
    Event readevent{this, READ};
    Event writeevent{this, WRITE};
    Event finishevent{this, FINISH};
    Event doneevent{this, DONE};
  };

private:
  void HandleRpcsFromA() {
    create_new_a();
    void *tag; // uniquely identifies a request.
    bool ok;
    while (true) {
      // Next return if there's an event or when Shutdown
      GPR_ASSERT(cq_->Next(&tag, &ok));
      static_cast<Event *>(tag)->Process(ok);
    }
  }

  void HandleRpcsFromC() {
    create_new_c();
    void *tag; // uniquely identifies a request.
    bool ok;
    while (true) {
      // Next return if there's an event or when Shutdown
      GPR_ASSERT(cq_->Next(&tag, &ok));
      static_cast<Event *>(tag)->Process(ok);
    }
  }

  void remove_finished_rpc(CallData_Interface *p) {
    if (calls.find(p) == calls.end()) {
      std::cout << "Not find rpc" << std::endl;
      return;
    }
    calls.erase(p);
  }

  void AddFuture(const Futures &future) {
    std::lock_guard<std::mutex> lk(mutex_subscribers);
    for (auto &subscriber : subscribers) {
      dynamic_cast<CallData_C &>(*subscriber.second).AddFuture(future);
    }
  }

  void Subscribe(CallData_Interface *clientc) {
    std::shared_ptr<CallData_Interface> ptr;
    // find the call from call queue
    {
      std::lock_guard<std::mutex> lk(mutex_calls);
      auto it = calls.find(clientc);
      if (it == calls.end()) {
        std::cerr << "Not find rpc" << std::endl;
        return;
      }
      ptr = it->second;
    }

    std::lock_guard<std::mutex> lk(mutex_subscribers);
    subscribers.emplace(clientc, ptr);
  }

  void UnSubscribe(CallData_Interface *clientc) {
    std::lock_guard<std::mutex> lk(mutex_subscribers);
    subscribers.erase(clientc);
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  ConnectService::AsyncService service_;
  std::unique_ptr<Server> server_;

  // 存在1种rpc， 1种状态： 等待发送的c
  std::mutex mutex_subscribers;
  std::map<CallData_Interface *, std::shared_ptr<CallData_Interface>>
      subscribers;

  // 存在2种rpc， 2种状态：监听的和等待发送的a+c
  std::mutex mutex_calls;
  std::map<CallData_Interface *, std::shared_ptr<CallData_Interface>> calls;
};

int main(int argc, char **argv) {
  ServerImpl server;
  server.Run();

  std::thread t1([&server]() {
    sleep(6000);
    server.Shutdown();
  });
  return 0;
}
