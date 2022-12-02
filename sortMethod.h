#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <libpmem.h>
#include <set>
#include <chrono>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#include "Utils/Record.h"
#include "Utils/KeyPtrPair.h"
#include "Utils/HelpFunctions.h"
#include "Utils/MinHeap.h"

using namespace std;
using namespace std::chrono;

template <typename T>
void swap(T* a, T* b) {
    T temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

template <typename T>
long long findMedian(T* recordBaseAddr, long long l, long long r) {
    long long a = (recordBaseAddr + l)->key;
    long long b = (recordBaseAddr + (l+r)/2)->key;
    long long c = (recordBaseAddr + r)->key;
    long long median = max(min(a,b), min(max(a,b),c));
    long long res = l;
    if (median == b) res = (l + r) / 2;
    else if (median == c) res = r;
    return res;
}

template <typename T>
long long partition( T* recordBaseAddr, long long l, long long r)
{
    // Declaration
	long long median = findMedian(recordBaseAddr, l, r);
	if (median != r) swap(recordBaseAddr + median, recordBaseAddr + r);
    long long pivot = (recordBaseAddr + r)->key;
    long long i = (l - 1);

    // Rearranging the array
    for (long long j = l; j <= r - 1; j++) {
        if ((recordBaseAddr + j)->key < pivot) {
            i++;
            swap(recordBaseAddr + i, recordBaseAddr + j);
        }
    }
    swap(recordBaseAddr + i + 1, recordBaseAddr + r);

    // Returning the respective index
    return (i + 1);
}


template <typename T>
void quickSortCore( T* recordBaseAddr, long long l, long long r) {
    if (l < r)
    {
        long long index = partition(recordBaseAddr, l, r);
        quickSortCore(recordBaseAddr, l, index - 1);
        quickSortCore(recordBaseAddr, index + 1, r);
    }
}


/* method 1*/
template <typename T>
void quickSort( T* recordBaseAddr, long long numKeys) {
    quickSortCore<T>(recordBaseAddr, 0, numKeys - 1);
}


template <typename T>
void quickSort2(T* recordBaseAddr, long long l, long long r) {
    if (l >= r) return;
	long long mid = (l + r) / 2;
    long long median = findMedian(recordBaseAddr, l, r);
    long long i = l - 1, j = r + 1, x = (recordBaseAddr + median)->key;
    if (median != mid) {
        swap(recordBaseAddr + median, recordBaseAddr + mid);
    }
    while(i < j) {
        do i++; while ((recordBaseAddr + i)->key < x);
        do j--; while ((recordBaseAddr + j)->key > x);
        if (i < j) {
            swap(recordBaseAddr + i, recordBaseAddr + j);
        }
    }
    quickSort2(recordBaseAddr, l, j);
    quickSort2(recordBaseAddr, j + 1, r);
}

template <typename T>
void quickSort2Count(T* recordBaseAddr, long long l, long long r, long long* count) {
    if (l >= r) return;
	long long mid = (l + r) / 2;
    long long median = findMedian(recordBaseAddr, l, r);
    long long i = l - 1, j = r + 1, x = (recordBaseAddr + median)->key;
    if (median != mid) {
        swap(recordBaseAddr + median, recordBaseAddr + mid);
        *count += 2;
    }
    while(i < j) {
        do i++; while ((recordBaseAddr + i)->key < x);
        do j--; while ((recordBaseAddr + j)->key > x);
        if (i < j) {
            swap(recordBaseAddr + i, recordBaseAddr + j);
            *count += 2;
        }
    }
    quickSort2Count(recordBaseAddr, l, j, count);
    quickSort2Count(recordBaseAddr, j + 1, r, count);
}

/* method 2*/
template <typename T>
void mergeSort(T* RecordBaseAddr, int numKeys) {
    vector<T> tmp(numKeys);
    mergeSortHelper(RecordBaseAddr, tmp, 0, numKeys - 1);
}

template <typename T>
void mergeSortHelper(T* RecordBaseAddr, vector<T>& tmp, int l, int r) {
    if (l >= r) return;

    int mid = l + r >> 1;
    mergeSortHelper(RecordBaseAddr, tmp, l, mid);
    mergeSortHelper(RecordBaseAddr, tmp, mid + 1, r);

    int k = 0, i = l, j = mid + 1;
    while (i <= mid && j <= r) {
        if ((RecordBaseAddr + i) ->key <= (RecordBaseAddr + j) -> key) tmp[k++] = *(RecordBaseAddr + i++);
        else tmp[k++] = *(RecordBaseAddr + j++);
    }
    while (i <= mid) tmp[k++] = *(RecordBaseAddr + i++);
    while (j <= r) tmp[k++] = *(RecordBaseAddr + j++);

    for (int i = l, j = 0; i <= r; i++, j++) *(RecordBaseAddr + i) = tmp[j];
}

/* method 6 heapSort*/
template <typename T>
void heapify(T* arr, int N, int i)
{
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;
    // If left child is larger than root
    if (l < N && (arr + l)->key > (arr + largest)->key)
        largest = l;
    // If right child is larger than largest
    if (r < N && (arr + r)->key > (arr + largest)->key)
        largest = r;
    // If largest is not root
    if (largest != i) {
        swap(arr + i, arr + largest);
        // Recursively heapify the affected sub-tree
        heapify(arr, N, largest);
    }
}

template <typename T>
void heapSort(T* arr, int N)
{
    for (int i = N / 2 - 1; i >= 0; i--)
        heapify(arr, N, i);
 
    // One by one extract an element from heap
    for (int i = N - 1; i > 0; i--) {
        // Move current root to end
        swap(arr, arr + i);

        // call max heapify on the reduced heap
        heapify(arr, i, 0);
    }
}


/* method 8 parallel implemnetation of quicksort */
template <typename T>
void quickSortParlHelper(T* recordBaseAddr, long long l, long long r) {
	if (l >= r) return;
	long long mid = (l + r) / 2;
	long long median = findMedian(recordBaseAddr, l, r);
	long long i = l - 1, j = r + 1, x = (recordBaseAddr + median)->key;
	if (median != mid) {
		swap(recordBaseAddr + median, recordBaseAddr + mid);
	}
	while(i < j) {
		do i++; while ((recordBaseAddr + i)->key < x);
		do j--; while ((recordBaseAddr + j)->key > x);
		if (i < j) {
			swap(recordBaseAddr + i, recordBaseAddr + j);
		}
	}
	#pragma omp task default(none) firstprivate(recordBaseAddr, l, j)
	{
		quickSortParlHelper(recordBaseAddr, l, j);
	}
	#pragma omp task default(none) firstprivate(recordBaseAddr, r, j)
	{
		quickSortParlHelper(recordBaseAddr, j + 1, r);
	}

}

/* method 8 parallel implemnetation of quicksort */
template <typename T>
void quickSortParl(T* recordBaseAddr, long long numKeys, int numThreads) {
	omp_set_dynamic(0);
	#pragma omp parallel default(none) shared(recordBaseAddr, numKeys) num_threads(numThreads)
	{
		#pragma omp single nowait
		quickSortParlHelper<T>(recordBaseAddr, 0, numKeys - 1);
	}
}

/* method 9 parallel implemnetation of mergesort */
template <typename T>
void mergeSortParl(T* recordBaseAddr, long long numKeys, int numThreads, T* sortedBaseAddrTemp, bool useDRAM) {
	omp_set_dynamic(0);
    T* tmp;
	if (useDRAM) tmp = new T[numKeys];
	else {
        tmp = sortedBaseAddrTemp;
    }
    #pragma omp parallel num_threads(numThreads)
	{
		#pragma omp single
		mergeSortParlHelper(recordBaseAddr, tmp, 0  , numKeys - 1);
	}
	if (useDRAM) delete[] tmp;

}

template <typename T>
void mergeSortHelper(T* recordBaseAddr, T* tmp, long long l, long long r) {
	if (l >= r) return;
	int mid = l + r >> 1;
	mergeSortHelper(recordBaseAddr, tmp, l, mid);
	mergeSortHelper(recordBaseAddr, tmp, mid + 1, r);
	merge(recordBaseAddr, tmp, mid, l, r);

}

template <typename T>
void mergeSortParlHelper(T* recordBaseAddr, T* tmp, long long l, long long r) {
	if (l >= r) return;
	int mid = l + r >> 1;
	if ((r - l > 10000)) {
		#pragma omp task shared(recordBaseAddr, tmp)
		mergeSortParlHelper(recordBaseAddr, tmp, l, mid);
		#pragma omp task shared(recordBaseAddr, tmp)
		mergeSortParlHelper(recordBaseAddr, tmp, mid + 1, r);
		#pragma omp taskwait
		merge(recordBaseAddr, tmp, mid, l, r);
	} else {
		mergeSortHelper(recordBaseAddr, tmp, l, r);
	}
}

template <typename T>
void merge(T* recordBaseAddr, T* tmp, long long mid, long long l, long long r) {
	int k = l, i = l, j = mid + 1;
	while (i <= mid && j <= r) {
		if ((recordBaseAddr + i) ->key <= (recordBaseAddr + j) -> key) tmp[k++] = *(recordBaseAddr + i++);
		else tmp[k++] = *(recordBaseAddr + j++);
	}
	while (i <= mid) tmp[k++] = *(recordBaseAddr + i++);
	while (j <= r) tmp[k++] = *(recordBaseAddr + j++);

	for (int i = l; i <= r; i++) *(recordBaseAddr + i) = tmp[i];
}





/* method 3*/
template <typename T>
void externalSortParl(Record *recordBaseAddr, T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int mem_num, int numThreads) {
    // case 1: DRAM space enough, in fact a quickSort
    //cout <<"test0"<<endl;
    if (total_num <= mem_num) {
        cout << "memcpy" <<endl;
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < total_num; i++) {
            (ptrBaseAddr + i)->key = (recordBaseAddr + i)->key;
            (ptrBaseAddr + i)->recordPtr = recordBaseAddr + i;
        }

        cout << "quicksort" <<endl;
        //quickSortMedian<KeyPtrPair>(ptrBaseAddr, 0, total_num - 1);
        omp_set_dynamic(0);
        quickSortParl<KeyPtrPair>(ptrBaseAddr, total_num, numThreads);
        pmem_memcpy_nodrain(sortedBaseAddr, ptrBaseAddr, total_num * sizeof(KeyPtrPair));

        return;
    }

    cout<<"external" <<endl;

    // case 2: small DRAM as buffer
    auto start_2 = high_resolution_clock::now();
    /* read data as key-ptr pairs and sort in dram. Save sorted key-pter temporarily in NVM */
    sortGroupParl(recordBaseAddr, ptrBaseAddr, sortedBaseAddrTemp, total_num, mem_num, numThreads);
    auto stop_2 = high_resolution_clock::now();
    auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Sort Runs Time Used: " << duration_2.count() << "microseconds" << endl;

    /* merge the sorted groups using minHeap */
    start_2 = high_resolution_clock::now();
    mergeGroup(ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, total_num, mem_num, numThreads);
    stop_2 = high_resolution_clock::now();
    duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Merge Runs Time Used: " << duration_2.count() << "microseconds" << endl;
    return;
}



template <typename T>
void sortGroupParl(Record *recordBaseAddr, T *ptrBaseAddr, T *sortedBaseAddrTemp, int total_num, int mem_num, int numThreads) {
    omp_set_dynamic(0);
    int sort_num;

    int grp_num = total_num / mem_num;
    int left_num = total_num;

    for (int i = 0; i <= grp_num; i++) {
        //cout << i << endl;
        sort_num = min(mem_num, left_num);
        #pragma omp parallel for num_threads(numThreads)
        for (int j = 0; j < sort_num; j++) {
            (ptrBaseAddr + j)->key = (recordBaseAddr + i * mem_num + j)->key;
            (ptrBaseAddr + j)->recordPtr = recordBaseAddr + i * mem_num + j;
        }

        quickSortParl<KeyPtrPair>(ptrBaseAddr, sort_num, numThreads);
        //PARADIS(ptrBaseAddr, ptrBaseAddr + sort_num, numThreads);

        // store into NVM
        pmem_memcpy_nodrain(sortedBaseAddrTemp + i * mem_num, ptrBaseAddr, sort_num * sizeof(KeyPtrPair));

        // print sorted grps
        // for (int j = 0; j < sort_num; j++) {
        //     cout << (ptrBaseAddr + j)->key << endl;
        // }
        // cout << endl;
        
        left_num -= sort_num;
        if (left_num == 0) {
            break;
        }
    }
}

template <typename T>
void mergeGroup(T *ptrBaseAddr, T* sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int mem_num, int numThreads) {
    /* merge the sorted groups using minHeap */
    int grp_num = total_num / mem_num;
    if (total_num % grp_num != 0) grp_num++;

    MinHeapNode* harr = new MinHeapNode[grp_num];
    for (int i = 0; i < grp_num; i++) {
        harr[i].key = (sortedBaseAddrTemp + i * mem_num)->key;
        harr[i].recordPtr = (sortedBaseAddrTemp + i * mem_num)->recordPtr;
        harr[i].groupNum = i;
    }

    MinHeap hp(harr, grp_num);
    int count = 0;
    int out = 0;
    int grp;
    int* grps = new int[grp_num]; // count sorted number in each group
    for (int i = 0; i < grp_num; i++) {
        grps[i] = 0;
    }

    while (count != grp_num) {
        MinHeapNode root = hp.getMin();
        (sortedBaseAddr + out) -> key = root.key;
        (sortedBaseAddr + out) -> recordPtr = root.recordPtr;

        // get new node from the group
        grp = root.groupNum;
        grps[grp]++;

        if (total_num % mem_num != 0 && grp == grp_num - 1 && grps[grp] == total_num % mem_num) {
            root.key = INT64_MAX;
            count++;
        } else if (grps[grp] == mem_num) {
            root.key = INT64_MAX;
            count++;
        } else {
            root.key = (sortedBaseAddrTemp + grp * mem_num + grps[grp]) -> key;
            root.recordPtr = (sortedBaseAddrTemp + grp * mem_num + grps[grp]) -> recordPtr;
        }
        hp.replaceMin(root);
        out++;
    }

    delete[] grps;
    return;
}

/* method 4 && 11*/
/* k-way merge using min-heap */


// /* runPass with out buffer and min heap*/
// template <typename T>
// void runPass(T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int block_size, int numThreads, int grp_num, int grp_size, int block_num, T* blocks[]) {
//     int dataSize = sizeof(T);
//     // cout << "grp_num: " << grp_num << endl;
//     // cout << "block_num: " << block_num << endl;
//     // cout << "total_num: " << total_num << endl;
//     int buffer_count = 0;
//     int buffer_round = 0;
//     T* outBuffer = new T[block_size];
//     #pragma omp parallel for num_threads(4)
//     for (int i = 0; i < grp_num - 1; i++) {
//         pmem_memcpy_nodrain(blocks[i], sortedBaseAddrTemp + i * grp_size, dataSize * block_size);
//     }

//     // copy data blocks to dram
//     bool hasLeft = total_num % grp_size;
//     int grp_sizes[grp_num];
//     for (int i = 0; i < grp_num; i++) grp_sizes[i] = grp_size;
//     if (hasLeft) grp_sizes[grp_num - 1] = total_num % grp_size;
//     if (!hasLeft) memcpy(blocks[grp_num-1], sortedBaseAddrTemp + (grp_num-1) * grp_size, dataSize * block_size);
//     else memcpy(blocks[grp_num-1], sortedBaseAddrTemp + (grp_num-1) * grp_size, dataSize * grp_sizes[grp_num - 1] );

//     // initialzation of Min Heap
//     MinHeapNode* harr = new MinHeapNode[grp_num];
//     for (int i = 0; i < grp_num; i++) {
//         harr[i].key = (ptrBaseAddr + i * block_size)->key;
//         harr[i].recordPtr = (ptrBaseAddr + i * block_size)->recordPtr;
//         harr[i].groupNum = i;
//     }

//     MinHeap hp(harr, grp_num);
//     int count = 0; // number of sorted groups
//     int sorted_total = 0;
//     int grp;
//     int* grps = new int[grp_num]; // count sorted number in each group
//     int* grp_blocks = new int[grp_num]; // count sorted number in each dram block
//     for (int i = 0; i < grp_num; i++) {
//         grps[i] = 0;
//         grp_blocks[i] = 0;
//     }

//     while (count != grp_num) {
//         MinHeapNode root = hp.getMin();
//         //cout << root.key << endl;
//         // (sortedBaseAddr + sorted_total) -> key = root.key;
//         // (sortedBaseAddr + sorted_total) -> recordPtr = root.recordPtr;
//         (outBuffer + buffer_count) -> key = root.key;
//         (outBuffer + buffer_count) -> recordPtr = root.recordPtr;
//         buffer_count++;
//         if (buffer_count == block_size) {
//             memcpy(sortedBaseAddr + block_size * buffer_round, outBuffer, dataSize * block_size);
//             buffer_round++;
//             buffer_count = 0;
//         }
//         /* get new node from the group */
//         grp = root.groupNum;

//         grps[grp]++;;
//         grp_blocks[grp]++;
//         if (grp_blocks[grp] == block_size) {
//             int left = grp_sizes[grp] - grps[grp];
//             //cout << grp_sizes[grp] << endl;
//             if (left >= block_size) memcpy(blocks[grp], sortedBaseAddrTemp + grp * grp_size + grps[grp], dataSize * block_size);
//             else memcpy(blocks[grp], sortedBaseAddrTemp + grp * grp_size + grps[grp], dataSize * left);
//             grp_blocks[grp] = 0;
//         }
//         if (hasLeft && grp == grp_num - 1 && grps[grp] == total_num % grp_size) {
//             root.key = INT64_MAX;
//             count++;
//             //cout << "count: " << count << endl;
//         } else if (grps[grp] == grp_size) {
//             root.key = INT64_MAX;
//             count++;
//             //cout << "count: " << count << endl;
//         } else {
//             root.key = (ptrBaseAddr + grp * block_size + grp_blocks[grp]) -> key;
//             root.recordPtr = (ptrBaseAddr + grp * block_size + grp_blocks[grp]) -> recordPtr;
//             //cout << "input: " << root.key << endl;
//         }
//         hp.replaceMin(root);
//         sorted_total++;
//         //cout << endl;
//     }
//     if (buffer_count > 0) pmem_memcpy_nodrain(sortedBaseAddr + block_size * buffer_round, outBuffer, dataSize * buffer_count);

//     delete[] grps;
//     delete[] harr;
//     delete[] outBuffer;
//     return;
// }

// /* runPass with min heap*/
// template <typename T>
// void runPass(T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int block_size, int numThreads, int grp_num, int grp_size, int block_num, T* blocks[]) {
//     int dataSize = sizeof(T);
//     // cout << "grp_num: " << grp_num << endl;
//     // cout << "block_num: " << block_num << endl;
//     // cout << "total_num: " << total_num << endl;
//     #pragma omp parallel for num_threads(4)
//     for (int i = 0; i < grp_num - 1; i++) {
//         memcpy(blocks[i], sortedBaseAddrTemp + i * grp_size, dataSize * block_size);
//     }

//     // copy data blocks to dram
//     bool hasLeft = total_num % grp_size;
//     int grp_sizes[grp_num];
//     for (int i = 0; i < grp_num; i++) grp_sizes[i] = grp_size;
//     if (hasLeft) grp_sizes[grp_num - 1] = total_num % grp_size;
//     if (!hasLeft || grp_sizes[grp_num - 1] >= block_size) memcpy(blocks[grp_num-1], sortedBaseAddrTemp + (grp_num-1) * grp_size, dataSize * block_size);
//     else memcpy(blocks[grp_num-1], sortedBaseAddrTemp + (grp_num-1) * grp_size, dataSize * grp_sizes[grp_num - 1] );

//     // initialzation of Min Heap
//     MinHeapNode* harr = new MinHeapNode[grp_num];
//     for (int i = 0; i < grp_num; i++) {
//         harr[i].key = (ptrBaseAddr + i * block_size)->key;
//         harr[i].recordPtr = (ptrBaseAddr + i * block_size)->recordPtr;
//         harr[i].groupNum = i;
//     }

//     MinHeap hp(harr, grp_num);
//     int count = 0; // number of sorted groups
//     int out = 0;
//     int grp;
//     int* grps = new int[grp_num]; // count sorted number in each group
//     int* grp_blocks = new int[grp_num]; // count sorted number in each dram block
//     for (int i = 0; i < grp_num; i++) {
//         grps[i] = 0;
//         grp_blocks[i] = 0;
//     }

//     while (count != grp_num) {
//         MinHeapNode root = hp.getMin();
//         //cout << root.key << endl;
//         (sortedBaseAddr + out) -> key = root.key;
//         (sortedBaseAddr + out) -> recordPtr = root.recordPtr;

//         /* get new node from the group */
//         grp = root.groupNum;

//         grps[grp]++;;
//         grp_blocks[grp]++;
//         if (grp_blocks[grp] == block_size) {
//             int left = grp_sizes[grp] - grps[grp];
//             //cout << grp_sizes[grp] << endl;
//             if (left >= block_size) memcpy(blocks[grp], sortedBaseAddrTemp + grp * grp_size + grps[grp], dataSize * block_size);
//             else memcpy(blocks[grp], sortedBaseAddrTemp + grp * grp_size + grps[grp], dataSize * left);
//             grp_blocks[grp] = 0;
//         }
//         if (hasLeft && grp == grp_num - 1 && grps[grp] == total_num % grp_size) {
//             root.key = INT64_MAX;
//             count++;
//             //cout << "count: " << count << endl;
//         } else if (grps[grp] == grp_size) {
//             root.key = INT64_MAX;
//             count++;
//             //cout << "count: " << count << endl;
//         } else {
//             root.key = (ptrBaseAddr + grp * block_size + grp_blocks[grp]) -> key;
//             root.recordPtr = (ptrBaseAddr + grp * block_size + grp_blocks[grp]) -> recordPtr;
//             //cout << "input: " << root.key << endl;
//         }
//         hp.replaceMin(root);
//         out++;
//         //cout << endl;
//     }
//     delete[] grps;
//     delete[] harr;
//     return;
// }

#define MINKEY 0  
#define MAXKEY  1677721600 
#define SWAP(a,b) {a=a^b;b=b^a;a=a^b;}

template <typename T>
void adjust(T* b,int* ls,int s,int k)
{

    int t = (s + k) / 2;
    //cout << "adjust " << s << endl;
    while (t > 0) {
        // iterate until t is root
        //cout << b[s].key << " " << b[ls[t]].key << endl;
        if (b[s].key > b[ls[t]].key){
            SWAP(s,ls[t]);
        }
        t/= 2;
    }

    ls[0] = s;
    //cout << "ls[0] : " << s << endl;
}



/**
 * arry:多路归并的所有数据
 * b:存放多路归并的首地址数组
 * ls:败者树数组
 * k:几路归并
 */
template <typename T>
void createLoserTree(T *arry[], T* b,int* ls,int k)
{

    for (int i = 0; i < k; ++i){
        b[i].key = (arry[i] + 0)->key;//每一路的首元素进行赋予
        b[i].recordPtr = (arry[i] + 0)->recordPtr;
    }
    //最后一个元素用于存放默认的最小值，用于刚开始的对比
    b[k].key = MINKEY;


    //设置ls数组中败者的初始值，既b[k]最小值
    for (int i=0; i < k; i++) {

        ls[i] = k;
    }

    //有k个叶子节点
    //从最后一个叶子节点开始，沿着从叶子节点到根节点的路径调整
    for (int i = k - 1; i >= 0; --i){
        adjust(b, ls, i, k);
        // for (int j = 0; j < k; j++) {
        //     cout << ls[j] << " ";
        // }
        // cout << endl;
    }

    // for (int i = 0; i < k; i ++) {
    //     cout << ls[i] << endl;
    // }

}

/**
 *多路归并操作
 */
template <typename T>
void kMerge(T* sortedBaseAddr, T* arr[], int* arrayCount, int k, int* ls, T* b)
{

    //index数组记录每一路读取的索引，
    int index[k];
    for (int i = 0; i < k; i++){

        index[i] = 0;
    }

    //cout << "test1" << endl;
    int out_num = 0;
    while (b[ls[0]].key!=MAXKEY) {

        int s = ls[0];
        (sortedBaseAddr + out_num)->key = b[s].key;
        (sortedBaseAddr + out_num)->recordPtr = b[s].recordPtr;
        ++index[s];

        if (index[s] < arrayCount[s]){

            b[s].key = (arr[s] + index[s])->key;
            b[s].recordPtr = (arr[s] + index[s])->recordPtr;
        }else{

            b[s].key = MAXKEY;
        }
        adjust(b, ls, s, k);
        out_num++;
    }
}


/* parallelizable runPass with loser tree*/
template <typename T>
void runPass(T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int block_size, int numThreads, int grp_num, int grp_size, int block_num, T* blocks[]) {
    // calculate size of each groups to be merged
    bool hasLeft = total_num % grp_size;
    int grp_sizes[grp_num];
    for (int i = 0; i < grp_num; i++) grp_sizes[i] = grp_size;
    if (hasLeft) grp_sizes[grp_num - 1] = total_num % grp_size;

    int k = grp_num;
    int ls[k];
    T b[k+1];

    T** arr = new T*[grp_num];
    for (int i = 0; i < grp_num; i++) arr[i] = sortedBaseAddrTemp + i * grp_size;

    createLoserTree(arr,b,ls,k);

    kMerge<KeyPtrPair>(sortedBaseAddr, arr, grp_sizes, k, ls, b);

    delete[] arr;
}

template <typename T>
T* runMerge(T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int mem_num, int numThreads, int grp_num, int grp_size, int block_num, T* blocks[]) {
    int block_size = mem_num / block_num;
    int dataSize = sizeof(T);
    //void runPass(T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int block_size, int grp_num, int grp_size, int block_num) {
    int passCount = 0;
    T* ptrBaseAddr2 = new KeyPtrPair[mem_num];
    while (grp_num > 1) {
        cout << "running pass " << passCount << endl;
        if (passCount > 0) { // swap sortedBaseAddr and sortedBaseAddr in each round
            T* tempAddr = sortedBaseAddr;
            sortedBaseAddr = sortedBaseAddrTemp;
            sortedBaseAddrTemp = tempAddr;
        }
        if (grp_num <= block_num) {
            runPass(ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, total_num, block_size, numThreads, grp_num, grp_size, block_num, blocks);
            //runPass(ptrBaseAddr, ptrBaseAddr2, sortedBaseAddrTemp, total_num, block_size, numThreads, grp_num, grp_size, block_num, blocks);
            grp_num = 1;
        } else {
            int numMerges = grp_num / block_num; // how many merge operation in one pass
            int grpLeft = grp_num;
            int numLeft = total_num;
            if (grp_num % block_num) numMerges++;
            int numGrpToMerge[numMerges];
            int numToSort[numMerges];
           
            for (int i = 0; i < numMerges; i++) {
                numGrpToMerge[i] = min(block_num, grpLeft);
                numToSort[i] = min(block_num * grp_size, numLeft);
                grpLeft -= block_num;
                numLeft -= block_num * grp_size;
            }
            #pragma omp parallel for num_threads(1)
            for (int i = 0; i < numMerges; i++) {
                runPass(ptrBaseAddr, sortedBaseAddr + i*block_num*grp_size, sortedBaseAddrTemp + i*block_num*grp_size, numToSort[i], block_size, numThreads, numGrpToMerge[i], grp_size, block_num, blocks);
            }
            grp_size *= block_num;
            grp_num = numMerges;
        }
        passCount++;
    }
    return sortedBaseAddr;
}

template <typename T>
T* externalMergeSort(Record *recordBaseAddr, T *ptrBaseAddr, T *sortedBaseAddr, T *sortedBaseAddrTemp, int total_num, int mem_num, int numThreads) {
    /* step 1, sorted runs of block size */
    const int block_num = 10;
    int block_size = mem_num / block_num;

    T* blocks[block_num];
    for (int i = 0; i < block_num; i++) blocks[i] = ptrBaseAddr + i * block_size;

    // case 2: small DRAM as buffer
    auto start_2 = high_resolution_clock::now();
    /* read data as key-ptr pairs and sort in dram. Save sorted key-pter temporarily in NVM */
    sortGroupParl(recordBaseAddr, ptrBaseAddr, sortedBaseAddrTemp, total_num, mem_num, numThreads);
    auto stop_2 = high_resolution_clock::now();
    auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Sort Runs Time Used: " << duration_2.count() << "microseconds" << endl;


    /* merge the sorted groups using minHeap */
    start_2 = high_resolution_clock::now();
    int grp_num = total_num / mem_num;
    if (total_num % mem_num) grp_num++;

    sortedBaseAddr = runMerge<T>(ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, total_num, mem_num, numThreads, grp_num, mem_num, block_num, blocks);
    return sortedBaseAddr;
}


/* method 5 montres*/ 

template <typename T>
void quickSort( T* recordBaseAddr, int l, int r) {
    if (l < r)
    {
        int index = partition(recordBaseAddr, l, r);
        if (index < 10000) cout << index << endl;
        quickSort(recordBaseAddr, l, index - 1);
        quickSort(recordBaseAddr, index + 1, r);
    }
}

template <typename T>
void quickSortMedian(T* recordBaseAddr, int l, int r) {
    if (l >= r) return;
	int mid = (l + r) / 2;
    int median = findMedian(recordBaseAddr, l, r);
    int i = l - 1, j = r + 1, x = (recordBaseAddr + median)->key;
    if (median != mid) {
        swap(recordBaseAddr + median, recordBaseAddr + mid);
    }
    while(i < j) {
        do i++; while ((recordBaseAddr + i)->key < x);
        do j--; while ((recordBaseAddr + j)->key > x);
        if (i < j) {
            swap(recordBaseAddr + i, recordBaseAddr + j);
        }
    }
    quickSortMedian(recordBaseAddr, l, j);
    quickSortMedian(recordBaseAddr, j + 1, r);
}


template <typename T>
int partitionMinMax( T* recordBaseAddr, int l, int r)
{
    // Declaration
    int pivot = (recordBaseAddr + r)->min;
    int i = (l - 1);

    // Rearranging the array
    for (int j = l; j <= r - 1; j++) {
        if ((recordBaseAddr + j)->min < pivot) {
            i++;
            swap(recordBaseAddr + i, recordBaseAddr + j);
        }
    }
    swap(recordBaseAddr + i + 1, recordBaseAddr + r);

    // Returning the respective index
    return (i + 1);
}

template <typename T> // for montresSort
void quickSortMinMax( T* recordBaseAddr, int l, int r) {
    if (l < r)
    {
        int index = partitionMinMax(recordBaseAddr, l, r);
        // Parallel sections
        {
            #pragma omp task default(none) firstprivate(recordBaseAddr,l,index)
            {
                // Evaluating the left half
                quickSortMinMax(recordBaseAddr, l, index - 1);
            }
            #pragma omp task default(none) firstprivate(recordBaseAddr,r,index)
            {
                // Evaluating the right half
                quickSortMinMax(recordBaseAddr, index + 1, r);
            }
        }
    }
}

void findNaturalRun(int block_num, vector<MinMaxIndex>* naturalRuns, vector<MinMaxIndex>* minMaxs) {
    vector<int>* tempResult;
    vector<int>* naturalRunIndex = new vector<int>(); // temporal index store for natural runs
    int s, t;
    for (int i = 0; i < 100; i++) {
        s = i;
        tempResult = new vector<int>();
        int j = i + 1;
        while (j < block_num) {
            (*tempResult).push_back(s);
            for (; j < block_num; j++) {
                t = j;
                if ((*minMaxs)[t].min > (*minMaxs)[s].max) {
                    s = t;
                    break;
                }
            }
        }
        if ((*tempResult).size() >  (*naturalRunIndex).size()) {
            naturalRunIndex = tempResult;
        }
    }

    // save natural run indexs
    for (int i = 0; i <(*naturalRunIndex).size(); i++ ) {
        (*minMaxs)[(*naturalRunIndex)[i]].isNatural = true;
        (*naturalRuns).push_back((*minMaxs)[(*naturalRunIndex)[i]]);
        //cout << (*naturalRuns)[i].num << " "<< (*naturalRuns)[i].min << " " << (*naturalRuns)[i].max  << " " << (*naturalRuns)[i].isNatural << endl;
    }

    // remove natural run block from MinMax Indexes
    vector<MinMaxIndex>* minMaxs2 = new vector<MinMaxIndex>();
    std::copy_if ((*minMaxs).begin(), (*minMaxs).end(), std::back_inserter(*(minMaxs2)), [](MinMaxIndex i){return i.isNatural == false;} );
    (*minMaxs) = (*minMaxs2);

    minMaxs2 -> clear();
    tempResult -> clear();
    naturalRunIndex -> clear();
}

template <typename T>
void generateMinMaxIndex(Record *recordBaseAddr, T *ptrBaseAddr, int block_num, int block_size, vector<MinMaxIndex>* minMaxs, int thread_num) {

    omp_set_dynamic(0);
    #pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < block_num; i++) {
        int curr;
        (*minMaxs)[i].min = INT64_MAX;
        (*minMaxs)[i].max = 0;
        (*minMaxs)[i].num = i;
        (*minMaxs)[i].isNatural = false;
        for (int j = 0; j < block_size; j++) {
            curr = (recordBaseAddr + i * block_size + j)->key;
            if (curr < (*minMaxs)[i].min) {
                (*minMaxs)[i].min = curr;
            }
            if (curr > (*minMaxs)[i].max) {
                (*minMaxs)[i].max = curr;
            }
        }
    }
    // #pragma omp parallel default(none) shared(minMaxs, block_num) num_threads(thread_num)
    // {
    //     #pragma omp single nowait
    //     quickSortMinMax<MinMaxIndex>(&(*minMaxs)[0], 0, block_num-1);
    // }
    quickSortMinMax<MinMaxIndex>(&(*minMaxs)[0], 0, block_num-1);

    cout << "Done" << endl;
    cout << (*minMaxs)[(*minMaxs).size()-1].min << endl;
}

template <typename T>
void montresSort(Record *recordBaseAddr, T *ptrBaseAddr, T *sortedBaseAddr, int total_num, int mem_num, int thread_num) {

    int block_size = 512;
    int block_num = total_num/block_size;
    int numWrites = 0;
    if (total_num % block_num != 0) block_num += 1;
    vector<MinMaxIndex>* minMaxs = new vector<MinMaxIndex>();
    minMaxs->resize(block_num);
    vector<MinMaxIndex>* naturalRuns = new vector<MinMaxIndex>();

    // Generate MinMax Indexs
    cout << "Generating Minmax Indexs..." << endl;
    auto start_2 = high_resolution_clock::now();
    generateMinMaxIndex(recordBaseAddr, ptrBaseAddr, block_num, block_size, minMaxs, thread_num);
    auto stop_2 = high_resolution_clock::now();
    auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Generating Minmax Indexs Time Used: " << duration_2.count() << "microseconds" << endl;

    // Find Natural Run
    cout << "Finding Natural Run..." << endl;
    start_2 = high_resolution_clock::now();
    findNaturalRun(block_num, naturalRuns, minMaxs);
    stop_2 = high_resolution_clock::now();
    duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Find Natural Run Time Used: " << duration_2.count() << "microseconds" << endl;


    // Run Generation
    cout << "Run Generation..." << endl;
    start_2 = high_resolution_clock::now();
    static const char* RUN_FILE_PATH_PREFIX = "/optane/hanhua/RUNS";
    int naturalBlockNum = 0; // block_num of naturalRunIndex
    int minMaxBlockNum = 0; // block_num of minMaxs
    int Wn = 0; // natural run element index in dram
    int Ws = block_size; // minmax element index in dram
    int runNum = 0; // current run num
    int totalRunNum; // total number of sorted runs
    int memSize = mem_num / block_size - 1; // dram block number for minMax blocks
    int nextMin; // minimum value of next block
    int sortedNum = 0; // number of sorted datas

    Record* naturalBaseAddr;
    Record* minMaxBaseAddr;
    vector<KeyPtrPair*> *sortedRuns = new vector<KeyPtrPair*>(); // store pointers to sorted runs
    totalRunNum = minMaxs -> size() / memSize;
    if (minMaxs -> size() % memSize != 0) totalRunNum += 1;
    int* runSize = new int[totalRunNum]; // size of generated sorted run
    int* runCount = new int[totalRunNum]; // count merged number in each sorted run;
    for (int i = 0; i < totalRunNum; i++) {
        runSize[i] = 0;
        runCount[i] = 0;
    }
    //cout <<" size " << totalRunNum  << " " << minMaxs -> size() << " " << memSize << endl;

    // initialize Wn in dram
    if (naturalBlockNum < naturalRuns->size()) {
        naturalBaseAddr = recordBaseAddr + (*naturalRuns)[naturalBlockNum++].num * block_size;
        for (int i = 0; i < block_size; i++) {
            (ptrBaseAddr + i) -> key = (naturalBaseAddr + i)->key;
            (ptrBaseAddr + i) -> recordPtr = (naturalBaseAddr + i);
        }
        quickSortMedian<KeyPtrPair>(ptrBaseAddr, 0, block_size - 1);
    }

    while (minMaxBlockNum != minMaxs->size() ) {
        //cout << "UnSorted Ws data" << endl;
        int dram_blocks = 0;

        for (int i = 1; i <= memSize; i++) {
            if (minMaxBlockNum == minMaxs->size()) break;
            minMaxBaseAddr = recordBaseAddr + (*minMaxs)[minMaxBlockNum++].num * block_size;
            for (int j = 0; j < block_size; j++) {
                (ptrBaseAddr + i * block_size + j) -> key = (minMaxBaseAddr + j)->key;
                (ptrBaseAddr + i * block_size + j) -> recordPtr = (minMaxBaseAddr + j);
                //cout << (ptrBaseAddr + i * block_size + j) -> key << endl;
            }
            dram_blocks++;
        }

        quickSortMedian<KeyPtrPair>(ptrBaseAddr + block_size, 0, block_size * dram_blocks - 1); // 最后一次 memSize 没有满 待解决
        /*
        cout << "Sorted Ws data" << endl;
        for (int i = block_size; i < block_size*(1+memSize) ; i++) {
            //cout << i << endl;
            cout << (ptrBaseAddr + i) -> key << endl;
        }
        cout << endl; */

        nextMin = (*minMaxs)[minMaxBlockNum].min;
        // merge on fly
        int grp_num = sortedRuns -> size() + 2;
        int grp;
        int* grps = new int[grp_num]; // count sorted number in each group
        int* grp_sizes = new int[grp_num]; // total size of each group
        int count = 0; // number of sorted group in merge phase
        MinHeapNode* harr;
        harr = new MinHeapNode[grp_num];
        for (int i = 0; i < grp_num - 2; i++) {
            grps[i] = runCount[i];
        }
        grps[grp_num - 2] = 0;
        grps[grp_num - 1] = Wn;

        for (int i = 0; i< sortedRuns -> size(); i++) {
            harr[i].key = ((*sortedRuns)[i] + runCount[i]) -> key;
            harr[i].recordPtr = ((*sortedRuns)[i] + runCount[i]) -> recordPtr;
            harr[i].groupNum = i;
        }
        // need modificatoin, if Wn == block_size, add INT64MAX as node for natural run
        harr[grp_num - 2].key = (ptrBaseAddr + Ws) -> key;
        harr[grp_num - 2].recordPtr = (ptrBaseAddr + Ws) -> recordPtr;
        harr[grp_num - 2].groupNum = grp_num - 2;

        if (Wn == block_size) {
            harr[grp_num - 1].key = INT64_MAX;
            harr[grp_num - 1].groupNum = grp_num - 1;
            count == 1;
        } else {
            //harr[grp_num - 1].key = INT64_MAX; //fix
            harr[grp_num - 1].key = (ptrBaseAddr + Wn) -> key;
            harr[grp_num - 1].recordPtr = (ptrBaseAddr + Wn) -> recordPtr;
            harr[grp_num - 1].groupNum = grp_num - 1;
        }
        count = 1;
        MinHeap hp(harr, grp_num);

        grp_sizes[grp_num - 1] = block_size ;
        grp_sizes[grp_num - 2] = dram_blocks * block_size;

        for (int i = 0; i < sortedRuns -> size(); i++) {
            grp_sizes[i] = runSize[i];
        }
        while (count != grp_num) {
            MinHeapNode root = hp.getMin();
            if (root.key >= nextMin) {
                //cout << "merge on fly ends" << endl;
                break;
            }
            (sortedBaseAddr + sortedNum) -> key = root.key;
            (sortedBaseAddr + sortedNum) -> recordPtr = root.recordPtr;
            sortedNum++;
            //numWrites += 1;

            // gets next node from group
            grp = root.groupNum;
            grps[grp]++;
            if (grps[grp] == grp_sizes[grp] && grp != grp_num - 1) {
                //cout << "Group" << grp << " all merged" << endl;
                root.key = INT64_MAX;
                count++;
            } else {
                if (grp == grp_num - 2) { // node from Ws
                    numWrites += 1;
                    root.key = (ptrBaseAddr + block_size + grps[grp]) -> key;
                    root.recordPtr = (ptrBaseAddr + block_size + grps[grp]) -> recordPtr;
                } else if (grp == grp_num -1) { // node from Wn
                    numWrites += 1;
                    Wn++;
                    if (Wn == block_size) {
                        if (naturalBlockNum < naturalRuns->size()) {
                            naturalBaseAddr = recordBaseAddr + (*naturalRuns)[naturalBlockNum++].num * block_size;
                            for (int i = 0; i < block_size; i++) {
                                (ptrBaseAddr + i) -> key = (naturalBaseAddr + i)->key;
                                (ptrBaseAddr + i) -> recordPtr = (naturalBaseAddr + i);
                            }
                            quickSortMedian<KeyPtrPair>(ptrBaseAddr, 0, block_size - 1);
                            Wn = 0;
                            grps[grp] = 0;
                            root.key = (ptrBaseAddr + grps[grp]) -> key;
                            root.recordPtr = (ptrBaseAddr + grps[grp]) -> recordPtr;
                        } else {
                            count++;
                            root.key = INT64_MAX;
                        }
                    } else {
                        root.key = (ptrBaseAddr + Wn) -> key;
                        root.recordPtr = (ptrBaseAddr + Wn) -> recordPtr;
                    }

                } else { // node from previous sorted runs
                    root.key = ((*sortedRuns)[grp] + grps[grp]) -> key;
                    root.recordPtr = ((*sortedRuns)[grp] + grps[grp]) -> recordPtr;
                }
            }
            root.groupNum = grp;
            hp.replaceMin(root);

        }

        /*
        cout << "Sorted Num:" <<sortedNum <<endl;
        for (int i = 0; i < sortedNum; i++) {
            cout << (sortedBaseAddr + i) -> key << endl;
        } */

        // update previous sorted run
        for (int i = 0; i < grp_num - 2; i++) {
            runCount[i] = grps[i];
            //cout << "runCount i:" << i << " " << runCount[i] << endl;
         }

        // save sorted run
        string runNameString(RUN_FILE_PATH_PREFIX);
        runNameString.append(to_string(runNum)); // name of sorted runs
        int cur_run_size = dram_blocks * block_size - grps[grp_num - 2];
        //cout << runNameString.c_str() << endl;
        //cout << cur_run_size << endl;
        KeyPtrPair* runBaseAddr;

        if (cur_run_size != 0) {
            runBaseAddr = allocateNVMRegion<KeyPtrPair>(cur_run_size*sizeof(KeyPtrPair), runNameString.c_str());
            pmem_memcpy_nodrain(runBaseAddr, ptrBaseAddr + block_size + grps[grp_num - 2], cur_run_size*sizeof(KeyPtrPair));
            //numWrites += cur_run_size;
            sortedRuns->push_back(runBaseAddr);
            runSize[runNum] = cur_run_size;
            //cout << "Run " << runNum << " is empty" <<endl;
        } else {
            runBaseAddr = allocateNVMRegion<KeyPtrPair>(1 * sizeof(KeyPtrPair), runNameString.c_str());
            //pmem_memcpy_nodrain(runBaseAddr, ptrBaseAddr + block_size + grps[grp_num - 2], cur_run_size*sizeof(KeyPtrPair));
            sortedRuns->push_back(runBaseAddr);
            runSize[runNum] = cur_run_size;
        }
        /*
        cout << runNameString.c_str() << endl;
        for (int i = 0; i < cur_run_size; i++) {
            cout << (runBaseAddr + i) -> key << " ";
        }
        cout << endl;
        cout << endl;  */
        runNum++;
    }
    cout << "Done" << endl;
    stop_2 = high_resolution_clock::now();
    duration_2= duration_cast<microseconds>(stop_2 - start_2);\
    cout << "Run Generation Time Used: " << duration_2.count() << "microseconds" << endl;

    // Merge Phase
    cout << "Run Merging..." << endl;
    start_2 = high_resolution_clock::now();
    int grp_num = sortedRuns -> size() + 1;
    bool natural = false;
    int grp;
    int* grps = new int[grp_num]; // count sorted number in each group
    int* grp_sizes = new int[grp_num];
    int count = 0; // number of sorted group in merge phase
    if (Wn != block_size) {
        natural = true;
    }
    //natural = false; //fix
    if (!natural) count += 1;
    for (int i = 0; i < sortedRuns -> size(); i++) {
        grp_sizes[i] = runSize[i];
        grps[i] = runCount[i];
    }
    //grp_sizes[grp_num - 1] = 0; // fix
    grp_sizes[grp_num - 1] = block_size;
    grps[grp_num - 1] = Wn;

    MinHeapNode* harr;
    harr = new MinHeapNode[grp_num];
    for (int i = 0; i< sortedRuns -> size(); i++) {
        if (grps[i] < grp_sizes[i]) {
            harr[i].key = ((*sortedRuns)[i] + runCount[i]) -> key;
            harr[i].recordPtr = ((*sortedRuns)[i] + runCount[i]) -> recordPtr;
            harr[i].groupNum = i;
        } else {
            harr[i].key = INT64_MAX;
            harr[i].groupNum = i;
            count++;
        }
    }

    if (natural) {
        harr[grp_num - 1].key = (ptrBaseAddr + Wn) -> key;
        harr[grp_num - 1].recordPtr = (ptrBaseAddr + Wn) -> recordPtr;
        harr[grp_num - 1].groupNum = grp_num - 1;
    } else {
        harr[grp_num - 1].key = INT64_MAX;
        harr[grp_num - 1].groupNum = grp_num - 1;
    }

    MinHeap hp(harr, grp_num);
    while (count != grp_num) {
        MinHeapNode root = hp.getMin();

        (sortedBaseAddr + sortedNum) -> key = root.key;
        (sortedBaseAddr + sortedNum) -> recordPtr = root.recordPtr;
        sortedNum++;
        //numWrites++;

        // gets next node from group
        grp = root.groupNum;
        grps[grp]++;

        /*
        cout << "nextMin" << nextMin << " SortedNum: " << sortedNum << endl;
        cout << "root: " << root.key << endl;
        cout << "root grp: " << root.groupNum << endl;
        cout << naturalBlockNum << endl;
        cout << Wn << endl;
        cout << grp << endl;
        cout << "size"<<grp_sizes[grp] <<endl;
        //cout << naturalRuns->size() << endl;
        cout << count << " group num:"<< grp_num << " merged grp num:"<< grps[grp] << endl;
        */

        if (grps[grp] == grp_sizes[grp] && grp != grp_num - 1) {
            //cout << "Group" << grp << " all merged" << endl;
            root.key = INT64_MAX;
            count++;
        } else {
            if (grp == grp_num -1) { // node from Wn
                Wn++;
                if (Wn == block_size) {
                    if (naturalBlockNum < naturalRuns->size()) {
                        naturalBaseAddr = recordBaseAddr + (*naturalRuns)[naturalBlockNum++].num * block_size;
                        for (int i = 0; i < block_size; i++) {
                            (ptrBaseAddr + i) -> key = (naturalBaseAddr + i)->key;
                            (ptrBaseAddr + i) -> recordPtr = (naturalBaseAddr + i);
                        }
                        quickSort<KeyPtrPair>(ptrBaseAddr, 0, block_size - 1);
                        Wn = 0;
                        grps[grp] = 0;
                        root.key = (ptrBaseAddr + grps[grp]) -> key;
                        root.recordPtr = (ptrBaseAddr + grps[grp]) -> recordPtr;
                    } else {
                        count++;
                        root.key = INT64_MAX;
                    }
                } else {
                    root.key = (ptrBaseAddr + grps[grp]) -> key;
                    root.recordPtr = (ptrBaseAddr + grps[grp]) -> recordPtr;
                }

            } else { // node from previous sorted runs
                root.key = ((*sortedRuns)[grp] + grps[grp]) -> key;
                root.recordPtr = ((*sortedRuns)[grp] + grps[grp]) -> recordPtr;
            }
        }
        hp.replaceMin(root);
    }
    cout << sortedNum << endl;
    cout << "Done" << endl;
    stop_2 = high_resolution_clock::now();
    duration_2= duration_cast<microseconds>(stop_2 - start_2);\
    cout << "Run Merge Time Used: " << duration_2.count() << "microseconds" << endl;
    cout << "Number of merge-on-fly writes: " << numWrites << endl;
    return;
}


/* */
