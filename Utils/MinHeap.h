#pragma once

struct MinHeapNode {
    uint64_t key;
    Record* recordPtr;
    int groupNum;
};

void swap(MinHeapNode* x, MinHeapNode* y)
{
   MinHeapNode temp = *x;
   *x = *y;
   *y = temp;
}

class MinHeap {
   MinHeapNode* harr;
   int heap_size;

   public:
      MinHeap(MinHeapNode a[], int size);
      void MinHeapify(int);
      int left(int i) {
       return (2 * i + 1);
      }
      int right(int i) {
         return (2 * i + 2);
      }
      MinHeapNode getMin() {
         return harr[0];
    }
      void replaceMin(MinHeapNode x) {
         harr[0] = x;
         MinHeapify(0);
      }
};

MinHeap::MinHeap(MinHeapNode a[], int size) {

   heap_size = size;
   harr = a;
   int i = (heap_size - 1) / 2;
   while (i >= 0) {
      MinHeapify(i);
      i--;
   }
}

void MinHeap::MinHeapify(int i) {

   int l = left(i);
   int r = right(i);
   int smallest = i;
   if (l < heap_size && harr[l].key < harr[i].key)
      smallest = l;
   if (r < heap_size && harr[r].key < harr[smallest].key)
      smallest = r;
   if (smallest != i) {
      swap(&harr[i], &harr[smallest]);
      MinHeapify(smallest);
   }
}
