/*
Copyright 2019 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
 * This file implements the priority queue. It's the same as the
 * org.brail.utils.PriorityQueue class, and was adapted from Sedgewick.
 */
 
#include <stdlib.h>

#include "apib_priorityq.h"

#define INITIAL_SIZE 16

pq_Queue* pq_Create()
{
  pq_Queue* q =
    (pq_Queue*)malloc(sizeof(pq_Queue));
  q->allocated = INITIAL_SIZE;
  q->size = 1;
  q->items = 
    (pq_Item*)malloc(sizeof(pq_Item) * INITIAL_SIZE);
  q->items[0].weight = 0;
  q->items[0].item = NULL;
  return q;
}

void pq_Free(pq_Queue* q) {
  free(q->items);
  free(q);
}

/*
 * An element was added at the end, so fix the heap by moving it upwards
 * until it is of lower priority than both its children.
 */

static void upheap(pq_Queue* q, int p)
{
  int pos = p;
  pq_Item endVal = q->items[p];
  
  while (q->items[pos / 2].weight > endVal.weight) {
    q->items[pos] = q->items[pos / 2];
    pos /= 2;
  }
  q->items[pos] = endVal;
}

/*
 * An element was moved to the top, so fix the heap by moving it downwards
 * until it is of higher priority than its parent.
 */

static void downheap(pq_Queue* q, int p)
{
  int pos = p;
  pq_Item endVal;
  
  if (q->size > 2) {
    endVal = q->items[pos];
    while (pos <= (q->size / 2)) {
      int pos2 = pos * 2;
      /* Make sure we descend down the lowest-priority child */
      if ((pos2 < (q->size - 1)) &&
          (q->items[pos2].weight > q->items[pos2 + 1].weight)) {
        pos2++;
      }
      if ((pos2 < q->size) &&
          (endVal.weight > q->items[pos2].weight)) {
        q->items[pos] = q->items[pos2];
        pos = pos2;
      } else {
        /* The heap doesn't need fixing */
        break;
      }
    }
    q->items[pos] = endVal;
  }
}

void pq_Push(pq_Queue* q, void* item, long long priority)
{
  if (q->size >= q->allocated) {
    /* Expand the array */
    q->allocated *= 2;
    q->items = realloc(q->items, sizeof(pq_Item) * q->allocated);
  }
  /* Put the element on the end and fix up the heap */
  q->items[q->size].item = item;
  q->items[q->size].weight = priority;
  upheap(q, q->size);
  q->size++;
}

void* pq_Pop(pq_Queue* q)
{
  if (q->size > 1) {
    void* ret = q->items[1].item;
    q->size--;
    q->items[1] = q->items[q->size];
    downheap(q, 1);
    return ret;
  } 
  return NULL;
}

const void* pq_Peek(const pq_Queue* q)
{
  if (q->size > 1) {
    return q->items[1].item;
  }
  return NULL;
}
 
long long pq_PeekPriority(const pq_Queue* q)
{
  if (q->size > 1) {
    return q->items[1].weight;
  }
  return 0LL;
}
