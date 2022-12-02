#pragma once

#include "Utils/Record.h"
#include "Utils/KeyPtrPair.h"
#include "Utils/HelpFunctions.h"
#include "Utils/MinHeap.h"

using namespace std;
using namespace std::chrono;

template <typename T>
long long findMedianCount(T* recordBaseAddr, long long l, long long r, int* countR, int* countW) {
    long long a = (recordBaseAddr + l)->key;
    long long b = (recordBaseAddr + (l+r)/2)->key;
    long long c = (recordBaseAddr + r)->key;
    long long median = max(min(a,b), min(max(a,b),c));
    long long res = l;
    *countR += 3;
    if (median == b) res = (l + r) / 2;
    else if (median == c) res = r;
    return res;
}

template <typename T>
void quickSortParlHelperCount(T* recordBaseAddr, long long l, long long r, int* countR, int* countW) {
	if (l >= r) return;
	long long mid = (l + r) / 2;
	long long median = findMedian(recordBaseAddr, l, r);
	long long i = l - 1, j = r + 1, x = (recordBaseAddr + median)->key;
	if (median != mid) {
		swap(recordBaseAddr + median, recordBaseAddr + mid);
        *countR += 2;
        *countW += 2;
	}
	while(i < j) {
		do i++, (*countR)++; while ((recordBaseAddr + i)->key < x);
		do j--, (*countR)++; while ((recordBaseAddr + j)->key > x);
		if (i < j) {
			swap(recordBaseAddr + i, recordBaseAddr + j);
            *countR += 2;
            *countW += 2;
		}
	}
	#pragma omp task default(none) firstprivate(recordBaseAddr, l, j, countR, countW)
	{
		quickSortParlHelperCount(recordBaseAddr, l, j, countR, countW);
	}
	#pragma omp task default(none) firstprivate(recordBaseAddr, r, j, countR, countW)
	{
		quickSortParlHelperCount(recordBaseAddr, j + 1, r, countR, countW);
	}

}

/* method 8 parallel implemnetation of quicksort */
template <typename T>
void quickSortParlCount(T* recordBaseAddr, long long numKeys, int numThreads, int* countR, int* countW) {
	omp_set_dynamic(0);
	#pragma omp parallel default(none) shared(recordBaseAddr, numKeys, countR, countW) num_threads(numThreads)
	{
		#pragma omp single nowait
		quickSortParlHelperCount<T>(recordBaseAddr, 0, numKeys - 1, countR, countW);
	}
}


template <typename T>
void mergeSortCount(T* RecordBaseAddr, int numKeys, int* countR, int* countW) {
    vector<T> tmp(numKeys);
    mergeSortHelperCount(RecordBaseAddr, tmp, 0, numKeys - 1, countR, countW);
}

template <typename T>
void mergeSortHelperCount(T* RecordBaseAddr, vector<T>& tmp, int l, int r, int* countR, int* countW) {
    if (l >= r) return;

    int mid = l + r >> 1;
    mergeSortHelperCount(RecordBaseAddr, tmp, l, mid, countR, countW);
    mergeSortHelperCount(RecordBaseAddr, tmp, mid + 1, r, countR, countW);

    int k = 0, i = l, j = mid + 1;
    while (i <= mid && j <= r) {
        (*countR) += 2, (*countW)++;
        if ((RecordBaseAddr + i) ->key <= (RecordBaseAddr + j) -> key) tmp[k++] = *(RecordBaseAddr + i++);
        else tmp[k++] = *(RecordBaseAddr + j++);
    }
    while (i <= mid) tmp[k++] = *(RecordBaseAddr + i++), (*countR) ++, (*countW)++;
    while (j <= r) tmp[k++] = *(RecordBaseAddr + j++), (*countR) ++, (*countW)++;

    for (int i = l, j = 0; i <= r; i++, j++) *(RecordBaseAddr + i) = tmp[j], (*countW)++;
}


template <typename T>
void heapify(T* arr, int N, int i, int* countR, int* countW)
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
        *countR += 2;
        *countW += 2;
        // Recursively heapify the affected sub-tree
        heapify(arr, N, largest, countR, countW);
    }
}

template <typename T>
void heapSortCount(T* arr, int N, int* countR, int* countW)
{
    for (int i = N / 2 - 1; i >= 0; i--)
        heapify(arr, N, i, countR, countW);
 
    // One by one extract an element from heap
    for (int i = N - 1; i > 0; i--) {
        // Move current root to end
        swap(arr, arr + i);
        *countR += 2;
        *countW += 2;
        // call max heapify on the reduced heap
        heapify(arr, i, 0, countR, countW);
    }
}
