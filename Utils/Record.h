#pragma once

typedef struct BYTE_16 { unsigned char val[16]; } BYTE_16;

typedef struct BYTE_8 { unsigned char val[8]; } BYTE_8;
typedef struct BYTE_24 { unsigned char val[24]; } BYTE_24;
typedef struct BYTE_248 { unsigned char val[248]; } BYTE_248;
typedef struct BYTE_4088 { unsigned char val[4088]; } BYTE_4088;

struct Record {
    uint64_t key;
    BYTE_8 value;
};


struct MinMaxIndex {
    uint64_t min;
    uint64_t max;
    uint64_t num;
    bool isNatural;
};


