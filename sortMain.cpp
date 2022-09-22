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

using namespace std;
using namespace std::chrono;

#define CHECK_SORTED 1

void convertToPtrDram(Record* recordBaseAddr, KeyPtrPair* ptrBaseAddr, long long numKeys);
void convertToPtrNVM(Record* recordBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys);
void savePtrToNVM(KeyPtrPair* ptrBaseAddr, KeyPtrPair* sortedBaseAddr, long long numKeys);

int main(int argc, char* argv[]) {


    if (argc != 4) {
        cout << "Num args supplied = " << argc << endl;
        cout << "Usage: <number_of_keys_to_sort>" << endl;
        return 0;
    }

    //const char* pfilepaths[3] = {"/optane/hanhua/UNSORTED_KEYS4M", "/optane/hanhua/UNSORTED_KEYS16M", "/optane/hanhua/UNSORTED_KEYS25G"};
    const char* pfilepaths[3] = {"/optane/hanhua/UNSORTED_KEYS4M", "/optane/hanhua/UNSORTED_KEYS", "/optane/hanhua/UNSORTED_KEYS25G"};
    long long keySizes[3] = {4194304, 16777216, 1677721600};

    int data = atol(argv[1]);
    bool useDram = atol(argv[2]);
    int sortMethod = atol(argv[3]);

    long long numKeys = keySizes[data];
    long long targetLength = numKeys * sizeof(Record);
    int numThreads = 1;

    const char* pfilepath = pfilepaths[data];
    const char* sortedpath = "/optane/hanhua/SORTED/SORTED_KEYS";
    const char* sortedpatht = "/optane/hanhua/SORTED/SORTED_KEYS_TEMP";

    Record* recordBaseAddr; // initial key-value data in NVM
    KeyPtrPair* sortedBaseAddr; // sorted key-pointer in NVM
    KeyPtrPair* ptrBaseAddr; // temporary key-pointer in DRAM
    KeyPtrPair* sortedBaseAddrTemp = allocateNVMRegion<KeyPtrPair>(numKeys*sizeof(KeyPtrPair), sortedpatht); // extra space to store key-ptr in NVM for external sort
    vector<KeyPtrPair> *dramBase = new vector<KeyPtrPair>();

    long long mem_num = numKeys / 100 * 1.1;
    dramBase->resize(mem_num);
    ptrBaseAddr = &(*dramBase)[0];

    recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);
    sortedBaseAddr = allocateNVMRegion<KeyPtrPair>(numKeys*sizeof(KeyPtrPair), sortedpath);

    if (useDram) {
        cout << "convert to dram" << endl;
        convertToPtrDram(recordBaseAddr, ptrBaseAddr, numKeys);
    } else {
        cout << "convert to nvm" << endl;
        convertToPtrNVM(recordBaseAddr, sortedBaseAddr, numKeys);
    }


    auto start_1 = high_resolution_clock::now();
    switch (sortMethod) {
    case 1:
        if (useDram) {
            cout << "quickSort in dram" << endl;
            quickSort<KeyPtrPair>(ptrBaseAddr, numKeys);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "quickSort in nvm" << endl;
            quickSort<KeyPtrPair>(sortedBaseAddr, numKeys);
        }
        break;
    case 2:
        if (useDram) {
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
        //montresSort<KeyPtrPair>(recordBaseAddr, ptrBaseAddr, sortedBaseAddr, numKeys, mem_num, numThreads);
        break;
    case 6:
        cout << "sequential splitSort" << endl;
        break;
    case 7:
        if (useDram) {
            cout << "sequentail paradisSort in dram" << endl;
            PARADIS(ptrBaseAddr, ptrBaseAddr + numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "sequentail paradisSort in nvm" << endl;
            PARADIS(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads);
        }
    case 8:
        numThreads = 8;
        if (useDram) {
            cout << "parallel quickSort in dram" << endl;
            quickSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "parallel quickSort in nvm" << endl;
            quickSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
        }
        break;
    case 9:
        numThreads = 8;
        if (useDram) {
            cout << "parallel mergeSort in dram" << endl;
            mergeSortParl<KeyPtrPair>(ptrBaseAddr, numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "paralle mergeSort in nvm" << endl;
            mergeSortParl<KeyPtrPair>(sortedBaseAddr, numKeys, numThreads);
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
        break;  
    case 12:
        break;
    case 13:
        numThreads = 16;
        if (useDram) {
            cout << "parallel paradisSort in dram" << endl;
            PARADIS(ptrBaseAddr, ptrBaseAddr + numKeys, numThreads);
            savePtrToNVM(ptrBaseAddr, sortedBaseAddr, numKeys);
        } else {
            cout << "parallel paradisSort in nvm" << endl;
            PARADIS(sortedBaseAddr, sortedBaseAddr + numKeys, numThreads);
        }      
    }
    

    auto stop_1 = high_resolution_clock::now();
    auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
    cout << "Sort Time Used: " << duration_1.count() << "microseconds" << endl; 

#if CHECK_SORTED
    cout << "Working... Verifying sorted keys\n";
    bool sorted = true;
    for (int i = 0; i < numKeys; i++) {
        if (i < 10 && i > 0 ) cout << "i: " << i << " value: " <<(sortedBaseAddr + i)->key << endl;
        if ((sortedBaseAddr + i)->key != i) {
            cout << i << endl;
            cout << (sortedBaseAddr + i)->key << endl;
            cout << (sortedBaseAddr + i+1)->key << endl;;
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

