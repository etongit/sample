
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/bitcointracker.grpc.pb.h"
#else
#include "bitcointracker.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::BitcoinTracker;
using helloworld::AddAddressRequest;
using helloworld::AddAddressResponse;
using std::string;

class BitcoinTrackerClient {
 public:
  BitcoinTrackerClient(std::shared_ptr<Channel> channel)
      : stub_(BitcoinTracker::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string AddAddress(const std::string& user, const std::string& address) {
    // Data we are sending to the server.
    AddAddressRequest request;
    request.set_username(user);
    *request.add_address() = address;

    // Container for the data we expect from the server.
    AddAddressResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AddAddress(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      for (const auto& address : reply.address()) {
         std::cout << address << std::endl;
      }
      return "Successfully added address";
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<BitcoinTracker::Stub> stub_;
};

string findOrReturnDefault(int argc, char** argv, string defaultvalue, string matcher) {
  for (int i = 1; i < argc; i++) {
      std::string arg_val = argv[i];
      size_t start_pos = arg_val.find(matcher);
      if (start_pos != string::npos) {
      start_pos += matcher.size();
      if (arg_val[start_pos] == '=') {
        return arg_val.substr(start_pos + 1);
      } else {
        return "";
      }
      }
  }
  return defaultvalue;
}

string getAddress(int argc, char** argv) {
  return findOrReturnDefault(argc, argv, "somebitcoinaddress", "--address");
}

string getUsername(int argc, char** argv) {
  return findOrReturnDefault(argc, argv, "someuser", "--user");

}

string getTarget(int argc, char** argv) {
  return findOrReturnDefault(argc, argv, "localhost:50051", "--target");
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str = getTarget(argc, argv);
  std::string address = getAddress(argc, argv);
  std::string username = getUsername(argc, argv);
  if (address == "") {
        std::cout << "The only correct argument syntax is --address="
                  << std::endl;
  }
  if (target_str == "") {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
  }
  BitcoinTrackerClient bitcoinTracker(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string reply = bitcoinTracker.AddAddress(username, address);
  std::cout << "BitcoinTracker received: " << reply << std::endl;

  return 0;
}
