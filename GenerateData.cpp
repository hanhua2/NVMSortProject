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
    if (argc != 4) {
        cout << "Num args supplied = " << argc << endl;
        cout << "Usage: <number_of_keys_to_generate> <integer_seed>" << endl;
        return 0;
    }

    int numKeys = atol(argv[1]);
    int seed = atol(argv[2]);
    int isP = atol(argv[3]);
    int fd;
    char* memoryBase;
    char filepath[] = "./Record/UnSortedRecord.dat";
    char pfilepath[] = "/optane/hanhua/UNSORTED_KEYS2.5G";

    cout << "Generating Data to Sort" << endl;
    cout << "Record Unit Size = " << sizeof(Record) << " bytes\n";
    cout << "Number of keys to generate: " << numKeys << endl;
    cout << "Using seed: " << seed << endl;

    auto start = high_resolution_clock::now();
    
    
    // Create Random Keys
    srand(seed);
    vector<uint64_t> keys(numKeys);
    for(int i = 0; i < numKeys; i++) {
        keys[i] = i + 1;
        //keys[i] = numKeys - i - 1;
    }
    random_shuffle(keys.begin(), keys.end());
    
    
    // int num1, num2;
    // for (int i = 0; i < numKeys * 0.05; i ++) {
    //     num1 = rand() % numKeys;
    //     num2 = rand() % numKeys;
    //     //keys[num1] = num2;
        
    //     if (num1 != num2) {
    //        iter_swap(keys.begin() + num1, keys.begin() + num2);
    //     }   
    // } 
    
    
    

#if PRINT_GENERATED_KEYS
    cout << "Print Generated Keys:" << endl;
    for (int i = 0; i < numKeys; i++) {
        cout << keys[i] << endl;
    }
    cout << "Print Generated Keys Done" << endl;
#endif

    size_t targetLength = numKeys * sizeof(Record);


    
    if (isP) {
        
        Record* recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);

        cout << "Working... Copying generated keys into NVM\n";
        #pragma omp parallel for num_threads(16)
        for (int i = 0; i < numKeys; i++) {
            Record r;
            BYTE_24 val;
            r.key = keys[i];
            r.value.val[0] = keys[i];
            pmem_memcpy_nodrain((void*) (recordBaseAddr + i), (void*) &r, sizeof(Record));
        }

        pmem_unmap((char* ) recordBaseAddr, targetLength);

    } else{
        Record* recordBaseAddr;

        if ((fd = open(filepath, O_CREAT|O_RDWR|O_TRUNC, 0666)) < 0) {
            cout << "Create file failed" << endl;
            return 0;
        }
        memoryBase = (char *)mmap(NULL, sizeof(Record) * numKeys, \
                PROT_READ | PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);

        cout << "Working... Copying generated keys into Disk\n";
        recordBaseAddr = (Record*) memoryBase;
        for (int i = 0; i < numKeys; i++) {
            Record tempr;
            BYTE_24 val;
            tempr.key = keys[i];
            tempr.value.val[0] = keys[i];
            memcpy(recordBaseAddr + i, &tempr, sizeof(Record));
        }

        cout << "Writting generated unsorted file" << endl;
        write(fd, recordBaseAddr, sizeof(Record) * numKeys);
        cout << "Writting finished" << endl;

        close(fd);
        munmap(recordBaseAddr, sizeof(Record) * numKeys);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time Used:" << duration.count() << endl;
    cout << "Total size of Records generated (KB) = " << ((double) targetLength / (1 << 10)) << " KB\n";
    cout << "Total size of Records generated (MB) = " << ((double) targetLength / (1 << 20)) << " MB\n";
    cout << "Total size of Records generated (GB) = " << ((double) targetLength / (1 << 30)) << " GB\n";

    return 0;
}





