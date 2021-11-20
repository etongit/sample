# Run and test the detect_transfers file.

g++ detect_transfers.cc
./a.out

# Questions

1) Time complexity of the algo

O(N(LogN)) -- Where N is number of transactions.
Sorting the transactions by timestamp takes this time.
If sort was not required, the complexity would of O(N)

2) For close in time transaction

The matching pool instead of being one timestamp becomes a group
of timestamps. So we keep 2 separate sets of timestamps for incoming
and outgoing transactions, so if we consider matches within 3 minutes
to be valid, then these timestamps are in the set and we find the nearest
match for this.
