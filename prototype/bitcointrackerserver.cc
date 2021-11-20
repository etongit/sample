#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <tuple>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "examples/protos/bitcointracker.grpc.pb.h"
#else
#include "bitcointracker.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::BitcoinTracker;
using helloworld::AddAddressRequest;
using helloworld::AddAddressResponse;
using helloworld::DetectTransactionsRequest;
using helloworld::DetectTransactionsResponse;
using std::get;
using std::make_tuple;
using std::string;
using std::set;
using std::tuple;
using std::vector;

// Transaction id, wallet id, timestamp, direction, amount.
using transaction = std::tuple<string, string, string, string, double>;
using transactionpairs = std::pair<string, string>;

std::map<string, set<string>> username_address;
set<string> addresses;
vector<transaction> transactions;

// Custom sort function to sort transactions by timestamp.
struct {
	bool operator()(transaction a, transaction b) {
		return std::get<2>(a) > std::get<2>(b);
	}
} lesserTimestampFirst;

// Takes as input the addresses that we need to detect the transactions for, and returns
// the transaction id pairs.
vector<transactionpairs> detectTransactions(set<string> addresses) {
	vector<transactionpairs> matched_transactions;
	vector<transaction> outgoing;
	vector<transaction> incoming;
	// Separate incoming and outgoing transactions.
	for (const auto& one_transaction : transactions) {
		// Negative transactions are ignored.
		if (std::get<4>(one_transaction) <= 0) {
			continue;
		}
		if (std::get<3>(one_transaction) == "out") {
			outgoing.push_back(one_transaction);
		} else {
			incoming.push_back(one_transaction);
		}
	}
	// Sort transactions by timestamp.
	sort(outgoing.begin(), outgoing.end(), lesserTimestampFirst);
	sort(incoming.begin(), incoming.end(), lesserTimestampFirst);

	int oi = 0;
	int ii = 0;
	string outgoingTimestamp;
	string incomingTimestamp;
	// Find matching timestamps.
	while (oi < outgoing.size() && ii < incoming.size()) {
		outgoingTimestamp = std::get<2>(outgoing[oi]);
		incomingTimestamp = std::get<2>(incoming[ii]);
		if (outgoingTimestamp < incomingTimestamp) {
				++oi;
				continue;
		} else if (incomingTimestamp < outgoingTimestamp) {
			++ii;
			continue;
		}
		// If the address is from one user, then return matched transactions.
		if (addresses.find(get<1>(outgoing[oi])) != addresses.end() ||
				addresses.find(get<1>(incoming[ii])) != addresses.end()) {
			matched_transactions.push_back(
				make_pair(std::get<0>(outgoing[oi]),
					  std::get<0>(incoming[ii])));
		}
		oi++;
		ii++;
	}

	return matched_transactions;
}

// Bitcoin tracker server's behavior. Allows adding addresses and detecting transactions
// for a user.
class BitcoinTrackerImpl final : public BitcoinTracker::Service {
  Status AddAddress(ServerContext* context, const AddAddressRequest* request,
                  AddAddressResponse* response) override {
    for (const auto& address : request->address()) {
	    if (addresses.find(address) != addresses.end()) {
		std::cout << "Address is already claimed.";
		// TODO(aakanksha): find the right error code to send.
		continue;
	    }
	    addresses.insert(address);
	    username_address[request->username()].insert(address);
    }
    for (const auto& address : username_address[request->username()]) {
	   *response->add_address() = address;
    } 
    return Status::OK;
  }

  Status DetectTransactions(
		  ServerContext* context,
		  const DetectTransactionsRequest* request,
		  DetectTransactionsResponse* response) override {
    // No address are being tracked for this user, return early.  
    if (username_address.find(request->username()) == username_address.end()) {
       return Status::OK;
    }
    auto transactionpairs = detectTransactions(username_address[request->username()]);

    for (const auto& tpairs : transactionpairs) {
	auto* one_transaction = response->add_transaction();
	one_transaction->set_from_id(tpairs.first);
	one_transaction->set_to_id(tpairs.second);
    }
    return Status::OK;
  } 
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  BitcoinTrackerImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  transactions.push_back(make_tuple("1", "someaddress", "2", "out", 4));
  transactions.push_back(make_tuple("2", "anotheraddress", "2", "out", 4));
  transactions.push_back(make_tuple("3", "3", "2", "in", 4));
  transactions.push_back(make_tuple("4", "4", "2", "in", 4));
  transactions.push_back(make_tuple("5", "someaddress", "4", "in", 4));
  transactions.push_back(make_tuple("6", "3", "4", "out", 4));
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
