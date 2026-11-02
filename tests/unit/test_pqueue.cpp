// tests/unit/test_pqueue.cpp
//
// PriorityQueue: the A* open list behind every move path. find(id) used to
// scan the whole heap on every decrease-key; it now looks the id up in a
// position index. These tests drive the queue the way the path code does
// (insert, remove the best, change a key, clear between searches) and check
// that find() returns exactly what the old linear scan returned.

#include "doctest.h"

#include <cstddef>		// pqueue.h uses NULL
#include <cstdint>

#include "pqueue.h"

#include <vector>

namespace
{

// What find() returned before the index: the first slot, from the sentinel
// at 0 up to numItems, holding the id.
int linearFind(PriorityQueue& q, uint32_t id)
{
	for (int i = 0; i <= q.getNumItems(); ++i)
		if (q.getItem(i)->id == id)
			return i;
	return 0;
}

// Fixed-seed generator: the same sequence on every platform.
struct Lcg
{
	uint32_t s;
	uint32_t next(uint32_t n) { s = s * 1664525u + 1013904223u; return (s >> 8) % n; }
};

PQNode node(int32_t key, uint32_t id)
{
	PQNode n;
	n.key = key;
	n.id = id;
	n.row = (int32_t)(id / 1000);
	n.col = (int32_t)(id % 1000);
	return n;
}

}

TEST_CASE("pqueue: find matches the linear scan through inserts, removes and key changes")
{
	PriorityQueue q;
	q.init(5000);
	Lcg rng{12345u};

	for (int search = 0; search < 20; ++search) {
		q.clear();
		std::vector<bool> open(200000, false);
		for (int step = 0; step < 3000; ++step) {
			uint32_t id = rng.next(200000);
			int op = (int)rng.next(10);
			if (op < 5 && !open[id] && q.getNumItems() < 4000) {
				PQNode n = node((int32_t)rng.next(100000), id);
				CHECK(q.insert(n) == 0);
				open[id] = true;
			}
			else if (op < 7 && !q.isEmpty()) {
				PQNode best;
				q.remove(best);
				open[best.id] = false;
			}
			else if (open[id]) {
				int at = q.find(id);
				REQUIRE(at == linearFind(q, id));
				REQUIRE(at != 0);
				q.change(at, (int32_t)rng.next(100000));
			}
			REQUIRE(q.find(id) == linearFind(q, id));
		}
		// every id still open is found where the scan finds it
		for (uint32_t id = 0; id < open.size(); ++id)
			if (open[id])
				REQUIRE(q.find(id) == linearFind(q, id));
	}
}

TEST_CASE("pqueue: still a min-heap")
{
	PriorityQueue q;
	q.init(100);
	const int32_t keys[] = {50, 10, 70, 10, 30, 90, 20, 60, 40, 80};
	for (uint32_t i = 0; i < 10; ++i) {
		PQNode n = node(keys[i], 1000 + i);
		q.insert(n);
	}
	int at = q.find(1005);		// 90 -> 5
	REQUIRE(at != 0);
	q.change(at, 5);
	int32_t last = -1000000;
	while (!q.isEmpty()) {
		PQNode best;
		q.remove(best);
		CHECK(best.key >= last);
		last = best.key;
		CHECK(q.find(best.id) == 0);
	}
}

TEST_CASE("pqueue: an id queued twice falls back to the first match")
{
	// The path code never queues a cell twice, but find() must still answer
	// as the scan did if some caller does.
	PriorityQueue q;
	q.init(100);
	for (int32_t k = 0; k < 8; ++k) {
		PQNode n = node(100 - k, 7000 + (uint32_t)k);
		q.insert(n);
	}
	PQNode dup = node(1, 7003);
	q.insert(dup);
	CHECK(q.find(7003) == linearFind(q, 7003));
	PQNode best;
	q.remove(best);
	CHECK(q.find(7003) == linearFind(q, 7003));
	CHECK(q.find(7003) != 0);

	q.clear();
	PQNode n = node(5, 7003);
	q.insert(n);
	CHECK(q.find(7003) == 1);
	CHECK(q.find(7004) == 0);
}

TEST_CASE("pqueue: ids past the index limit are still found")
{
	PriorityQueue q;
	q.init(100);
	PQNode a = node(3, 0x7FFFFFF0u);
	PQNode b = node(1, 42);
	q.insert(a);
	q.insert(b);
	CHECK(q.find(0x7FFFFFF0u) == linearFind(q, 0x7FFFFFF0u));
	CHECK(q.find(42) == linearFind(q, 42));
	CHECK(q.find(43) == 0);
}

TEST_CASE("pqueue: a full queue refuses the next insert instead of writing past the end")
{
	// init(n) allocates n + 2 nodes, slot 0 the sentinel: n + 1 items fit.
	PriorityQueue q;
	q.init(4);
	for (uint32_t i = 0; i < 5; ++i) {
		PQNode n = node((int32_t)(10 - i), 100 + i);
		CHECK(q.insert(n) == 0);
	}
	PQNode extra = node(1, 200);
	CHECK(q.insert(extra) == 1);
	CHECK(q.getNumItems() == 5);
	CHECK(q.find(200) == 0);
	for (uint32_t i = 0; i < 5; ++i)
		CHECK(q.find(100 + i) == linearFind(q, 100 + i));
}
