#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

using namespace std;

// Transaction id, wallet id, timestamp, direction, amount.
using transaction = std::tuple<string, string, string, string, double>;
using transactionpairs = std::pair<string, string>;

// Custom sort function to sort transactions by timestamp.
struct {
	bool operator()(transaction a, transaction b) {
		return std::get<2>(a) > std::get<2>(b);
	}
} lesserTimestampFirst;

vector<transactionpairs> detectTransactions(vector<transaction> transactions) {
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
	std::sort(outgoing.begin(), outgoing.end(), lesserTimestampFirst);
	std::sort(incoming.begin(), incoming.end(), lesserTimestampFirst);

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
		matched_transactions.push_back(
				make_pair(std::get<0>(outgoing[oi++]),
					  std::get<0>(incoming[ii++])));
	}

	return matched_transactions;
}

void TestEmptyTransactionsReturnsNothing() {
	vector<transaction> transactions;
	auto transactionpairs = detectTransactions(transactions);
	assert(transactionpairs.size() == 0);
}

bool CompareTransactionPairs(vector<transactionpairs>& a, vector<transactionpairs>& b) {
	if (a.size() != b.size()) {
		return false;
	}
	for (int i = 0; i < a.size(); i++) {
		if (a[i].first != b[i].first || a[i].second != b[i].second) {
			return false;
		}
	}
	return true;
}

void TestDuplicateTimestampMatchesPairs() {
	vector<transaction> transactions;
	transactions.push_back(make_tuple("1", "1", "2", "out", 4));
	transactions.push_back(make_tuple("2", "2", "2", "out", 4));
	transactions.push_back(make_tuple("3", "3", "2", "in", 4));
	transactions.push_back(make_tuple("4", "4", "2", "in", 4));
	auto transactionresults = detectTransactions(transactions);
	for (const auto& tp: transactionresults) {
		std::cout << tp.first << " " << tp.second << " " << std::endl;
	}
	assert(transactionresults.size() == 2);
	vector<transactionpairs> expected_output;
	expected_output.push_back(make_pair("1", "3"));
	expected_output.push_back(make_pair("2", "4"));
	assert(CompareTransactionPairs(transactionresults, expected_output));
}

int main() {
	std::cout << "Hello world!!" << std::endl;
	TestEmptyTransactionsReturnsNothing();
	TestDuplicateTimestampMatchesPairs();
	return 0;
}
