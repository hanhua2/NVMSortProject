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
#include "../Utils/KeyPtrPair.h"
#include "../Utils/HelpFunctions.h"
#include "../Utils/MinHeap.h"




using namespace std;
using namespace std::chrono;



/* test the effect of minHeap size on merge time of external sort */
int main(int argc, char* argv[]) {
    long long numKeys;
    char pfilepath[] = "/dcpmm/hanhua/UNSORTED_KEYS25G";


    long long targetLength = numKeys * sizeof(Record);
    bool sorted = true;
    cout <<pfilepath << endl;
    Record* recordBaseAddr = allocateNVMRegion<Record>(targetLength, pfilepath);

    long long dataSizes[2] = {16777216, 167772160};
    //long long dataSizes[3] = {16777216, 167772160, 1677721600};
    long long heapSizes[6] = {10, 100, 1000, 10000, 100000, 1000000};
    vector<long long> datas(167772160);
    for (int i = 0; i < 167772160; i++) {
        datas[i] = i;
    }
    srand(2341);
    random_shuffle(datas.begin(), datas.end());

    for (int i = 0; i < sizeof(dataSizes) / sizeof(dataSizes[0]); i++) {
        for (int j = 0 ; j < sizeof(heapSizes) / sizeof(heapSizes[0]); j++) {
            auto start_1 = high_resolution_clock::now();

            numKeys = dataSizes[i];
            int grp_num = heapSizes[j];
            //cout << "numKeys: " << numKeys << endl;
            //cout << "heapSize: " << grp_num << endl; 
            long long count;
            MinHeapNode* harr = new MinHeapNode[grp_num];
            for (int i = 0; i < grp_num; i++) {
                harr[i].key = datas[i];
            }
            count = grp_num;
            MinHeap hp(harr, grp_num);
            while (count < numKeys) {
                MinHeapNode root = hp.getMin();
                root.key = datas[count];

                hp.replaceMin(root);
                count++;
            }

            auto stop_1 = high_resolution_clock::now();
            auto duration_1= duration_cast<microseconds>(stop_1 - start_1);
            cout << "numKeys: " << numKeys << " minHeap size: " << grp_num << " time used: " << duration_1.count() << "microseconds" << endl;
        }
    }
    cout << endl;

    return 0;

}




