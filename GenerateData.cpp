#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <chrono>
#include <libpmem.h>
#include <omp.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "Utils/Record.h"
#include "Utils/KeyPtrPair.h"
#include "Utils/HelpFunctions.h"

using namespace std;
using namespace std::chrono;

#define CHECK_KEYS 1
#define PRINT_GENERATED_KEYS 0


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Num args supplied = " << argc << endl;
        cout << "Usage: <number_of_keys_to_generate> <integer_seed>" << endl;
        return 0;
    }

    int numKeys = atol(argv[1]);
    int seed = atol(argv[2]);

    char pfilepath[] = "/optane/hanhua/UNSORTED_KEYS"; 

    cout << "Generating Data to Sort" << endl;
    cout << "Record Unit Size = " << sizeof(Record) << " bytes\n";
    cout << "Number of keys to generate: " << numKeys << endl;
    cout << "Using seed: " << seed << endl;
    cout << "Record size: " << sizeof(Record) << endl;
    auto start = high_resolution_clock::now();
    
    
    // Create Random Keys
    srand(seed);
    vector<uint64_t> keys(numKeys);
    for(int i = 0; i < numKeys; i++) {
        keys[i] = i + 1;
    }
    random_shuffle(keys.begin(), keys.end());
    

#if PRINT_GENERATED_KEYS
    cout << "Print Generated Keys:" << endl;
    for (int i = 0; i < numKeys; i++) {
        cout << keys[i] << endl;
    }
    cout << "Print Generated Keys Done" << endl;
#endif

    size_t targetLength = numKeys * sizeof(Record);

    Record* recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);

    cout << "Working... Copying generated keys into NVM\n";
    #pragma omp parallel for num_threads(16)
    for (int i = 0; i < numKeys; i++) {
        Record r;
        r.key = keys[i];
        r.value.val[0] = keys[i];
        pmem_memcpy_nodrain((void*) (recordBaseAddr + i), (void*) &r, sizeof(Record));
    }

    pmem_unmap((char* ) recordBaseAddr, targetLength);


    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time Used:" << duration.count() << endl;
    cout << "Total size of Records generated (KB) = " << ((double) targetLength / (1 << 10)) << " KB\n";
    cout << "Total size of Records generated (MB) = " << ((double) targetLength / (1 << 20)) << " MB\n";
    cout << "Total size of Records generated (GB) = " << ((double) targetLength / (1 << 30)) << " GB\n";

    return 0;
}





