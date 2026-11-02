//***************************************************************************
//
//	PQueue.h -- Prototype for priority queues
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef PQUEUE_H
#define PQUEUE_H

#include <cstdint>
#include <vector>

//***************************************************************************

//--------------
// Include Files

//--------------------------------
// Structure and Class Definitions

typedef struct _PQNode {
	int32_t         key;			// sort value
	uint32_t        id;				// hash value for this map position
	int32_t         row;			// HB-specific
	int32_t         col;			// HB-specific
} PQNode;

class PriorityQueue {

	protected:

		PQNode*		pqList;
		int32_t     maxItems;
		int32_t     numItems;
		int32_t     keyMin;

		// find() index: posById[id] is the slot of id while it is queued
		// (checked against pqList, so stale entries are harmless). An id past
		// kMaxIndexedId, or one queued twice, falls back to the linear scan
		// until the next clear(), so find() always answers as the scan did.
		static const uint32_t kMaxIndexedId = 1u << 20;
		std::vector<int32_t> posById;
		bool        linearFind;

		void setPos (int index) {
			uint32_t id = pqList[index].id;
			if (id < posById.size())
				posById[id] = index;
		}

		void downHeap (int curIndex);

		void upHeap (int curIndex);

	public:

		void init (void) {
			pqList = NULL;
			numItems = 0;
			linearFind = false;
		}

		PriorityQueue (void) {
			maxItems = 0;
			keyMin = 0;
			init();
		}

		int init (int maxItems, int keyMinValue = -2000000);

		int insert (PQNode& item);

		void remove (PQNode& item);

		void change (int itemIndex, int newValue);

		int find (unsigned int id);

		int findByKey (int32_t key, uint32_t id, int startIndex = 1);
		
		void clear (void) {
			numItems = 0;
			linearFind = false;
		}

		int getNumItems (void) { return(numItems); }
		
		bool isEmpty (void) {
			return(numItems == 0);
		}
		
		PQNode* getItem (int itemIndex) {
			return(&pqList[itemIndex]);
		}

		void destroy (void);

		~PriorityQueue (void) {
			destroy();
		}
};

typedef PriorityQueue* PriorityQueuePtr;

//***************************************************************************

#endif
