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
#include "sortMethod.h"
#include "SortMethodCount.h" 

using namespace std;
using namespace std::chrono;

#define CHECK_SORTED 1

void convertToPtrDram(Record* recordBaseAddr, KeyPtrPair* ptrBaseAddr, long long numKeys);
void convertToPtrNVM(Record* recordBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys);
void savePtrToNVM(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys);
void readDataDram(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys); 
void readDataNVM(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys);

int main(int argc, char* argv[]) {


    if (argc != 4) {
        cout << "Num args supplied = " << argc << endl;
        cout << "Usage: <number_of_keys_to_sort>" << endl;
        return 0;
    }

    /* address of unsorted data*/

    const char* pfilepath = "/optane/hanhua/UNSORTED_KEYS";


    bool useDRAM = atol(argv[1]);
    long long numKeys = atol(argv[2]);
    int sortMethod = atol(argv[3]);

    long long targetLength = numKeys * sizeof(Record);
    int numThreads = 1;

    const char* sortedpath = "/optane/hanhua/SORTED/SORTED_KEYS";
    const char* sortedpatht = "/optane/hanhua/SORTED/SORTED_KEYS_TEMP";

    Record* recordBaseAddr; // place of unsorted data in NVM
    KeyPtrPair* sortedBaseAddr; // place to store sorted key-pointer in NVM
    KeyPtrPair* ptrBaseAddr; // place to store temporary key-pointer in DRAM

    KeyPtrPair* sortedBaseAddrTemp = allocateNVMRegion<KeyPtrPair>(numKeys * sizeof(KeyPtrPair), sortedpatht); // extra space to store key-ptr in NVM for external sort

    long long mem_num = numKeys /16*1.01;
    if (sortMethod != 3 && sortMethod != 4 && sortMethod != 10 && sortMethod != 11) mem_num = numKeys;

    ptrBaseAddr = new KeyPtrPair[mem_num];

    recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);
    sortedBaseAddr = allocateNVMRegion<KeyPtrPair>(numKeys * sizeof(KeyPtrPair), sortedpath);

    memset(sortedBaseAddr, 0, numKeys * sizeof(KeyPtrPair));

    if (sortMethod != 3 && sortMethod != 4 && sortMethod != 10 && sortMethod != 11) {
        if (useDRAM) {
            convertToPtrDram(recordBaseAddr, ptrBaseAddr, numKeys);
        } else {
            convertToPtrNVM(recordBaseAddr, sortedBaseAddr, numKeys);
        }
    }


    auto start_1 = high_resolution_clock::now();
    switch (sortMethod) {
    case 1:
        if (useDRAM) {
            cout << "quickSort in dram" << endl;
            quickSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
            //readDataDram(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "quickSort in nvm" << endl;
            quickSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
            //readDataNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        }
        break;
    case 2:
        if (useDRAM) {
            cout << "mergeSort in dram" << endl;
            mergeSort<KeyPtrPair>(ptrBaseAddr, numKeys);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "mergeSort in nvm" << endl;
            mergeSort<KeyPtrPair>(sortedBaseAddr, numKeys);
        }
        break;
    case 3:
        cout << "externalSort" << endl;
        externalSortParl<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, numThreads);
        break;
    case 4:
        cout << "externalMergeSort" << endl;
        sortedBaseAddr = externalMergeSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, numThreads);
        break;
    case 5:
        cout << "montresSort" << endl;
        montresSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, numKeys, mem_num, numThreads);
        break;
    case 6:
        if (useDRAM) {
            cout << "heapSort in dram" << endl;
            heapSort<KeyPtrPair>(ptrBaseAddr, numKeys);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "heapSort in nvm" << endl;
            heapSort<KeyPtrPair>(sortedBaseAddr, numKeys);
        }
        break;
    case 7:
        if (useDRAM) {
            cout << "sequentail paradisSort in dram" << endl;
            PARADIS(ptrBaseAddr, ptrBaseAddr + numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
            //readDataDram(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "sequentail paradisSort in nvm" << endl;
            PARADIS(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads);
        }
    case 8:
        numThreads = 1;
        if (useDRAM) {
            cout << "parallel quickSort in dram" << endl;
            quickSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
            //readDataDram(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "parallel quickSort in nvm" << endl;
            quickSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
            //readDataNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        }
        break;
    case 9:
        numThreads = 32;
        if (useDRAM) {
            cout << "parallel mergeSort in dram" << endl;
            mergeSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads, sortedBaseAddrTemp, true);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "paralle mergeSort in nvm" << endl;
            mergeSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads, sortedBaseAddrTemp, false);
        }
        break;    
    case 10:
        numThreads = 8;
        cout << "parallel externalSort" << endl;
        externalSortParl<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, numThreads);
        //quickSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, 8);
        break;
    case 11:
        numThreads = 8;
        cout << "parallel externalMergeSort" << endl;
        sortedBaseAddr = externalMergeSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, sortedBaseAddrTemp, numKeys, mem_num, numThreads);
        //readDataNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        break;  
    case 12:
        break;
    case 13:
        numThreads = 16;
        if (useDRAM) {
            cout << "parallel paradisSort in dram" << endl;
            PARADIS(ptrBaseAddr, ptrBaseAddr + numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
            //readDataDram(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "parallel paradisSort in nvm" << endl;
            PARADIS(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads);
        }      
    }
    

    auto stop_1 = high_resolution_clock::now();
    auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
    cout << "Sort Time Used: " << duration_1.count() << "microseconds" << endl; 
    
    // auto start_3 = high_resolution_clock::now();
    // for (int i = 0; i < numKeys; i++) {
    //     auto key = (sortedBaseAddrTemp2 + i)->key;
    //     auto val = (sortedBaseAddrTemp2 + i)->value;
    // }
 
    // auto stop_3 = high_resolution_clock::now();
    // auto duration_3= duration_cast<microseconds>(stop_3 - start_3);
    // cout << "Read key-value time: " << duration_3.count() << "mic roseconds" << endl;

#if CHECK_SORTED
    cout << "Working... Verifying sorted keys\n";
    bool sorted = true;
    for (int i = 0; i < numKeys - 1; i++) {
        //if (i < 10 && i >= 0 ) cout << "i: " << i << " value: " <<(sortedBaseAddr + i)->key << endl;
        if ((sortedBaseAddr + i)->key >= (sortedBaseAddr + i + 1)->key) {
            // cout << i << endl;
            // cout << (sortedBaseAddr + i)->key << endl;
            // cout << (sortedBaseAddr + i+1)->key << endl;;
            cout << "Not sorted" << endl;
            sorted = false;
            break;
        }
    }
    if (sorted) cout << "Result sorted" << endl;
    cout << "Checking complete!\n";
#endif
    return 0;
}


void convertToPtrDram(Record* recordBaseAddr, KeyPtrPair* ptrBaseAddr, long long numKeys) {
    auto start_1 = high_resolution_clock::now();
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (ptrBaseAddr + i)->key = (recordBaseAddr + i)->key;
        (ptrBaseAddr + i)->recordPtr = recordBaseAddr + i;
    }

    auto stop_1 = high_resolution_clock::now();
    auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
    cout << "Conversion to Key-Pointer in Dram Time Used: " << duration_1.count() << "microseconds" << endl; 
}

void convertToPtrNVM(Record* recordBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys) {
    auto start_2 = high_resolution_clock::now();
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (sortedBaseAddr + i)->key = (recordBaseAddr + i)->key;
        (sortedBaseAddr + i)->recordPtr = recordBaseAddr + i;
    }
    auto stop_2 = high_resolution_clock::now();
    auto duration_2= duration_cast<microseconds>(stop_2 - start_2);
    cout << "Conversion to Key-Pointer in NVM Time Used: " << duration_2.count() << "mic roseconds" << endl;
}

void savePtrToNVM(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys) {
    auto start3 = high_resolution_clock::now();
    cout << "Save ptr in NVM" << endl;
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (sortedBaseAddr + i)->key = (ptrBaseAddr + i)->key;
        (sortedBaseAddr + i)->recordPtr = (ptrBaseAddr + i)->recordPtr;
    }
    auto stop3 = high_resolution_clock::now();
    auto duration3 = duration_cast<microseconds>(stop3 - start3);
    cout << "Time Used: " << duration3.count() << "microseconds" << endl;   
}

void readDataDram(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys) {
    auto start3 = high_resolution_clock::now();

    Record* resBaseAddr = new Record[numKeys];
    cout << "Convert to key-value in dram" << endl;
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (resBaseAddr + i)->key = (ptrBaseAddr + i)->key;
        (resBaseAddr + i)->value = (*((ptrBaseAddr + i)->recordPtr)).value;
    }
    auto stop3 = high_resolution_clock::now();
    auto duration3 = duration_cast<microseconds>(stop3 - start3);
    cout << "Time Used: " << duration3.count() << "microseconds" << endl;   
}

void readDataNVM(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys) {
    auto start3 = high_resolution_clock::now();

    Record* resBaseAddr = new Record[numKeys];
    cout << "Convert to key-value in nvm" << endl;
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (resBaseAddr + i)->key = (sortedBaseAddr + i)->key;
        (resBaseAddr + i)->value = (*((sortedBaseAddr + i)->recordPtr)).value;
    }
    auto stop3 = high_resolution_clock::now();
    auto duration3 = duration_cast<microseconds>(stop3 - start3);
    cout << "Time Used: " << duration3.count() << "microseconds" << endl;   
}

