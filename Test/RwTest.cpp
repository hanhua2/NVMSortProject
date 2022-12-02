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

#include "../Utils/Record.h"
#include "../Utils/HelpFunctions.h"
#include "../Utils/KeyPtrPair.h"

using namespace std;
using namespace std::chrono;



/* test the read and write performance on NVM with varying data num and data size */

int main(int argc, char* argv[]) {
    const size_t numKeys = 16777216;

    const char* pfilepath = "/optane/hanhua/UNSORTED_KEYS256_16M";
    const char* sortedpath = "/optane/hanhua/SORTED/SORTED_KEYS";

    size_t targetLength = numKeys * sizeof(Record);
    Record* recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);
    Record* dramBaseAddr = new Record[numKeys];
    Record* sortedBaseAddr = allocateNVMRegion<Record>(numKeys * sizeof(Record), sortedpath);

    //auto start_1 = high_resolution_clock::now();
    #pragma omp parallel for num_threads(8)
    for (int i = 0; i < numKeys; i++) {
        (dramBaseAddr + i)->key = (recordBaseAddr + i)->key;
        (dramBaseAddr + i)->value = (recordBaseAddr + i)->value;
    }
    //memcpy(dramBaseAddr, recordBaseAddr, targetLength);

    auto start_1 = high_resolution_clock::now();

    #pragma omp parallel for num_threads(8)
    // for (int i = 0; i < numKeys; i++) {
    //     (sortedBaseAddr + i)->key = (dramBaseAddr + i)->key;
    //     (sortedBaseAddr + i)->value = (dramBaseAddr + i)->value;
    // }
    for (int i = 0; i < numKeys; i++) {
        (sortedBaseAddr + i)->key = (recordBaseAddr + i)->key;
        (sortedBaseAddr + i)->value = (recordBaseAddr + i)->value;
    }

    /* copy to pmem, postponing drain step until the end */
    // pmem_memcpy_nodrain(sortedBaseAddr, dramBaseAddr, targetLength);
    // pmem_drain();


    auto stop_1 = high_resolution_clock::now();
    auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
    cout << "Time Used: " << duration_1.count() << "microseconds" << endl; 
    
    delete[] dramBaseAddr;
    return 0;

}





