#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <chrono>
#include <libpmem.h>
#include <set>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <omp.h>
//#include <execution>
#include <random>

#include "Utils/Record.h"
#include "Utils/KeyPtrPair.h"
#include "Utils/HelpFunctions.h"
#include "Utils/paradis.h"
#include "SortMethod.h"
//#include "ParallelMethod.h"
//#include "SortMethodRecord.h"



using namespace std;
using namespace std::chrono;

#define PRINT_KEYS 0
#define PRINT_PTRS 0
#define CHECK_SORTED 1
#define CHECK_PTR_SORTED 0

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Num args supplied = " << argc << endl;
        cout << "Usage: <number_of_keys_to_sort>" << endl;
        return 0;
    }

    long long numKeys = atol(argv[1]);
    long long mem_num = numKeys/1000*1.1;
    //int mem_num = numKeys;
    char pfilepath[] = "/dcpmm/hanhua/UNSORTED_KEYS_R";
    char sortedpath[] = "/dcpmm/hanhua/Sorted/SORTED_KEYS";
    char sortedpath2[] = "/dcpmm/hanhua/Sorted/SORTED_KEYS_TEMP";

    long long targetLength = numKeys * sizeof(Record);
    bool sorted = true;
    cout  <<pfilepath << endl;
    Record* recordBaseAddr;
    // Record* sortedBaseAddr;
    KeyPtrPair* sortedBaseAddr;
    KeyPtrPair* sortedBaseAddrTemp;

    KeyPtrPair* ptrBaseAddr;
    vector<KeyPtrPair> *dramBase = new vector<KeyPtrPair>();
    dramBase->resize(mem_num);
    ptrBaseAddr = &(*dramBase)[0];

    // // For externalSort2
    // vector<Record>* bufferRecord = new vector<Record>();
    // bufferRecord->resize(mem_num);
    // Record* buffer = &(*bufferRecord)[0];

    recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);
    sortedBaseAddr = allocateNVMRegion<KeyPtrPair>(numKeys*sizeof(KeyPtrPair), sortedpath);

#if PRINT_KEYS
    cout << "Working... Printing generated keys\n";
    for (int i = 0; i < 10; i++) {
        cout << (recordBaseAddr + i)->key << endl;
    }
    cout << "Checking complete!\n";
#endif

    // Part1, read data
    // Read record into dram as key-ptr pairs

    // auto start_1 = high_resolution_clock::now();
    // #pragma omp parallel for num_threads(16)
    // for (int i = 0; i < numKeys; i++) {
    //     (ptrBaseAddr + i)->key = (recordBaseAddr + i)->key;
    //     (ptrBaseAddr + i)->recordPtr = recordBaseAddr + i;
    // }

    // auto stop_1 = high_resolution_clock::now();
    // auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
    // cout << "Conversion to Key-Pointer Time Used: " << duration_1.count() << "microseconds" << endl;



    // for in pm ptr quicksort
    auto start_2 = high_resolution_clock::now();
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (sortedBaseAddr + i)->key = (recordBaseAddr + i)->key;
        (sortedBaseAddr + i)->recordPtr = recordBaseAddr + i;
    }
    auto stop_2 = high_resolution_clock::now();
    auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Conversion to Key-Pointer Time Used: " << duration_2.count() << "mic roseconds" << endl;



    // for in pm record quicksort

    // sortedBaseAddr = allocateNVMRegion<Record>(targetLength, sortedpath);
    // auto start_2 = high_resolution_clock::now();
    // for (int i = 0; i < numKeys; i++) {
    //     (sortedBaseAddr + i)->key = (recordBaseAddr + i)->key;
    //     (sortedBaseAddr + i)->value = (recordBaseAddr + i)->value;
    // }
    // //pmem_memcpy_nodrain(sortedBaseAddr, recordBaseAddr, numKeys*sizeof(Record));
    // auto stop_2 = high_resolution_clock::now();
    // auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    // cout << "Copy key-value to sorted time used: " << duration_2.count() << "mic roseconds" << endl;


    // Read record into dram directly

    // sortedBaseAddr = allocateNVMRegion<Record>(targetLength, sortedpath);
    // auto start_2 = high_resolution_clock::now();
    // vector<Record>* dramRecords = new vector<Record>();
    // dramRecords->resize(mem_num);
    // Record* recordBaseAddr2 = &(*dramRecords)[0];
    // //Record* recordBaseAddr2 = new Record[numKeys];
    // memcpy(recordBaseAddr2, recordBaseAddr , numKeys * sizeof(Record));

    // auto stop_2 = high_resolution_clock::now();
    // auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    // cout << "Copy data to dram time used: " << duration_2.count() << "microseconds" << endl;


    // Part2, Sort data
    auto start_total = high_resolution_clock::now();
    cout << "Sorting" << endl;


    // int writes = 0;
    // int *count = &writes;
    // //quickSortMedianCount<KeyPtrPair>(ptrBaseAddr, 0, numKeys - 1, count);
    // quickSortMedianCount<Record>(recordBaseAddr2, 0, numKeys - 1, count);
    // cout << "Number of writes: " << *count << endl;

    //quickSort<KeyPtrPair>(sortedBaseAddr, 0, numKeys - 1);

    //sort(execution::par_unseq, sortedBaseAddr, sortedBaseAddr + numKeys, [](KeyPtrPair x, KeyPtrPair y) {return x.key < y.key;});
    //quickSortSample<KeyPtrPair>(sortedBaseAddr, numKeys);
    
    //long long count = 0;
    //quickSortSampleCount<KeyPtrPair>(sortedBaseAddr, numKeys, count);
    //cout << "Total number of writes: " << count << endl;

    //selectSort(recordBaseAddr, numKeys);
    //mergeSort(recordBaseAddr, numKeys);
    //mergeSort<KeyPtrPair>(ptrBaseAddr, numKeys);


    /* external sort record */
    // sortedBaseAddr = allocateNVMRegion<Record>(targetLength, sortedpath);
    // vector<Record>* dramRecords = new vector<Record>();
    // dramRecords->resize(mem_num);
    // Record* recordBaseAddr2 = &(*dramRecords)[0];
    // externalSort<Record>(recordBaseAddr, recordBaseAddr2, sortedBaseAddr, sortedBaseAddrTemp2, numKeys, mem_num);

    //sortedBaseAddrTemp = allocateNVMRegion<KeyPtrPair>(numKeys * sizeof(KeyPtrPair), sortedpath2);
    //externalSortParl<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, 8);
    //sortedBaseAddr = externalMergeSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, 8);

    


    //cout <<"numPass: " << numPass << endl;

    /* montres sort */
    montresSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, numKeys, mem_num, 1);
    //montresSort<Record>(recordBaseAddr, recordBaseAddr2, sortedBaseAddr, numKeys, mem_num);

    //externalSort2<Record>(recordBaseAddr, buffer, sortedBaseAddr, numKeys, mem_num);

    /*
    Below are parallel sort algorithms
    */
    int numThreads = 1;
    int partitions = 512;
    int size = numKeys / partitions;
    

    //quickSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
    //quickSortMedianParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
    //quickSortMedianParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
    //mergeSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
    //oddEvenTransSort<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
    //shellSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
    //radixSortParl(dramBase, numKeys, numThreads);
    //PARADIS(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads);
    //PARADIS(ptrBaseAddr, ptrBaseAddr + numKeys, numThreads);
    long long count = 0, r_count = 0;
    //PARADISCount(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads, count, r_count);
    cout << "Total number of writes: " << count << endl;
    cout << "Total number of repair writes: " << r_count << endl;

    //sort(ptrBaseAddr, ptrBaseAddr + numKeys, [](KeyPtrPair x, KeyPtrPair y) {return x.key < y.key;});
    auto stop_total = high_resolution_clock::now();
    auto duration_total = duration_cast<microseconds>(stop_total - start_total);
    cout << "Sort Time Used: " << duration_total.count() << "microseconds" << endl;

    //Nlog(N)
    //N log(N/K)

    // Part3, save data in NVM

    //save record

    /*
    // ptr->record
    auto start3 = high_resolution_clock::now();
    cout << "Converting to key-value and save in NVM" << endl;

    for (int i = 0; i < numKeys; i++) {
        (sortedBaseAddr + i) -> key = (ptrBaseAddr + i)->key;
        (sortedBaseAddr + i) -> value = ((ptrBaseAddr + i)->recordPtr)->value;
    }
    cout << "Convertion done" << endl;
    auto stop3 = high_resolution_clock::now();
    auto duration3 = duration_cast<microseconds>(stop3 - start3);
    cout << "Time Used: " << duration3.count() << "microseconds" << endl;
    */

    // ptr

    // auto start3 = high_resolution_clock::now();
    // cout << "Save ptr in NVM" << endl;
    // pmem_memcpy_nodrain(sortedBaseAddr, ptrBaseAddr, numKeys * sizeof(KeyPtrPair));
    // // #pragma omp parallel for num_threads(numThreads)
    // // for (int i = 0; i < numKeys; i++) {
    // //     (sortedBaseAddr + i)->key = (ptrBaseAddr + i)->key;
    // //     (sortedBaseAddr + i)->recordPtr = (ptrBaseAddr + i)->recordPtr;
    // // }
    // auto stop3 = high_resolution_clock::now();
    // auto duration3 = duration_cast<microseconds>(stop3 - start3);
    // cout << "Time Used: " << duration3.count() << "microseconds" << endl;



#if PRINT_PTRS
    cout << "Working... Printing sorted pointers\n";

    for (int i = 0; i < 10; i++) {
        cout << (ptrBaseAddr + i)->key << endl;
    }
    cout << "Printing complete!\n";
#endif

#if CHECK_SORTED
    cout << "Working... Verifying sorted keys\n";
    sorted = true;
    for (int i = 0; i < numKeys; i++) {
        if (i < 10 && i > 0 ) cout << "i: " << i << " value: " <<(sortedBaseAddr + i)->key << endl;
        //cout << (sortedBaseAddr + i)->key << endl;
        //if (i < 638800+10 && i >638800-10 ) cout << "i: " << i << " value: " <<(sortedBaseAddr + i)->key << endl;
        if ((sortedBaseAddr + i)->key != i) {
        //if ((sortedBaseAddr + i)->key != i) {
            cout << i << endl;
            cout << (sortedBaseAddr + i)->key << endl;
            cout << (sortedBaseAddr + i+1)->key << endl;
            cout << (sortedBaseAddr + i+2)->key << endl;
            cout << "Not sorted" << endl;
            sorted = false;
            break;
        }
    }
    if (sorted) cout << "Result sorted" << endl;
    cout << "Checking complete!\n";
#endif

#if CHECK_PTR_SORTED
    cout << "Working... Verifying sorted key-pointers\n";
    for (int i = 0; i < numKeys; i++) {
        if ((ptrBaseAddr + i)->key != i) {
            cout << "Not sorted" << endl;
            sorted = false;
            break;
        }
    }
    if (sorted) cout << "Key-Ptr sorted" << endl;
    cout << "Checking complete!\n";
#endif


    pmem_unmap(recordBaseAddr, sizeof(Record) * numKeys);
    pmem_unmap(sortedBaseAddr, sizeof(KeyPtrPair) * numKeys);
    //pmem_unmap(sortedBaseAddrTemp2, sizeof(Record) * numKeys);

    //delete[] recordBaseAddr2;
    //delete[] ptrBaseAddr;
    //delete[] dramBase;
    return 0;
}




