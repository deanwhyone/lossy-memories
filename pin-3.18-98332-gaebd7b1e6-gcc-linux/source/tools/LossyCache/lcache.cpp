/*
 * Copyright 2002-2020 Intel Corporation.
 *
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*! @file
 *  This file contains an ISA-portable cache simulator
 *  data cache hierarchies
 */


#include "pin.H"

#include <iostream>
#include <fstream>
#include <cstdlib>
#define RAND() (rand() & 0xffff)  /* ensure only 16-bits */

// #include "compression.h"
#include "lcache.h"
#include "pin_profile.H"
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

std::ofstream outFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "lcache.out", "specify lcache file name");
KNOB<UINT32> KnobThresholdHit(KNOB_MODE_WRITEONCE , "pintool",
   "rh", "100", "only report memops with hit count above threshold");
KNOB<UINT32> KnobThresholdMiss(KNOB_MODE_WRITEONCE, "pintool",
   "rm","100", "only report memops with miss count above threshold");
KNOB<UINT32> KnobL1CacheSize(KNOB_MODE_WRITEONCE, "pintool",
    "l1s","32", "cache size in kilobytes");
KNOB<UINT32> KnobL1Associativity(KNOB_MODE_WRITEONCE, "pintool",
    "l1a","8", "cache associativity (1 for direct mapped)");
KNOB<UINT32> KnobL2CacheSize(KNOB_MODE_WRITEONCE, "pintool",
    "l2s","256", "cache size in kilobytes");
KNOB<UINT32> KnobL2Associativity(KNOB_MODE_WRITEONCE, "pintool",
    "l2a","16", "cache associativity (1 for direct mapped)");
KNOB<UINT32> KnobTestFPCompression(KNOB_MODE_WRITEONCE, "pintool",
    "fpc","0", "run test on floating point compression accuracy");
KNOB<UINT32> KnobUseFPCompression(KNOB_MODE_WRITEONCE, "pintool",
    "compression","0", "run test with compression");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

// wrap configuation constants into their own name space to avoid name clashes
namespace LCACHE_L1
{
    const UINT32 max_sets = 4 * KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 32; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;
    const UINT32 line_size = 64;

    typedef CACHE_LRU(max_sets, max_associativity, allocation, line_size) CACHE;
}
namespace LCACHE_L2
{
    const UINT32 max_sets = 16 * KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 32; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;
    const UINT32 line_size = 64;

    typedef CACHE_LRU(max_sets, max_associativity, allocation, line_size) CACHE;
}

LCACHE_L1::CACHE* dl1 = NULL;
LCACHE_L1::CACHE* il1 = NULL;
LCACHE_L2::CACHE* l2 = NULL;

UINT32 L2CompressibleMiss_CTR = 0;

typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;

typedef enum
{
    L1I_CACHE = 0,
    L1D_CACHE = 1,
    L2_CACHE = 2,
    CACHE_NUM
} CACHE_ID;

typedef  COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;

// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> il1_profile;
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> dl1_profile;
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> l2_profile;

/* ===================================================================== */

std::map<ADDRINT, ADDRINT> cMap;

BOOL CheckCompressible(ADDRINT addr) {
    // outFile << "Address is " << addr << endl;
    if (KnobUseFPCompression.Value()) {
        for (std::pair<ADDRINT, ADDRINT> element : cMap) {
            if ((element.first <= addr) && (addr < (element.first + element.second))) {
                // outFile << "Data is Compressable" << endl;
                return true;
            }
        }
    }
    return false;
}

BOOL CheckCompressibleSize(ADDRINT addr, UINT32 size) {
    for (UINT32 offset = 0; offset < size; ++offset) {
        if (!CheckCompressible(addr + offset)) {
            return false;
        }
    }
    return true;
}

/* ===================================================================== */

/*
 * Called on load miss in either L1 cache
 * Always loads a full cache line from L2, store into appropriate L1
 */
VOID LoadMultiL2(ADDRINT addr, CACHE_ID cacheId, char* l2Data)
{
    UINT32 size = 64; // granularity of a 64 byte cache line
    UINT32 instId = l2_profile.Map(addr);
    // printf("LoadMultiL2 addr: 0x%lx, size %u\n", addr, size);
    // fflush(stdout);

    // align addr with 64 byte boundary
    addr = addr & 0xffffffffffffffc0;

    // check if access can be compressed
    BOOL isCompressible = CheckCompressibleSize(addr, size);

    PIN_SafeCopy((void *)l2Data, (void *)addr, size); // load from memory to L2

    UINT32 dataSize = 0;
    char *value;
    if (isCompressible) {
        if (KnobUseFPCompression.Value() == 1) {
            double compressed_cacheline[4];
            value = (char *)malloc(32);
            dataSize = 32;
            CacheDoubleFPCompress(l2Data, compressed_cacheline, 4);
            PIN_SafeCopy(value, &compressed_cacheline, 32);
        } else {
            char compressed_cacheline[32];
            value = (char *)malloc(32);
            dataSize = 32;
            CacheDownsampleCompress(l2Data, compressed_cacheline);
            PIN_SafeCopy(value, &compressed_cacheline, 32);
        }
    } else {
        value = (char *)malloc(size);
        PIN_SafeCopy(value, l2Data, size);
    }

    const BOOL l2Hit = l2->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value, dataSize);

    if (isCompressible) {
        if (KnobUseFPCompression.Value() == 1) {
            double compressed_cacheline[4];
            PIN_SafeCopy(&compressed_cacheline, value, 32);
            CacheDoubleFPDecompress(compressed_cacheline, l2Data, 4);
        } else {
            char compressed_cacheline[32];
            PIN_SafeCopy(&compressed_cacheline, value, 32);
            CacheDownsampleDecompress(compressed_cacheline, l2Data);
        }
    } else {
        PIN_SafeCopy((void *)l2Data, (void *)value, size);
    }

    free(value);

    const COUNTER counter = l2Hit ? COUNTER_HIT : COUNTER_MISS;
    l2_profile[instId][counter]++;

    if (!l2Hit) {
        if (isCompressible) {
            L2CompressibleMiss_CTR++;
        }
    }
    // printf("LML2 done\n");
    // fflush(stdout);
}

/* ===================================================================== */

/*
 * Called on eviction of dirty data in L1D (L1I never evicts dirty data)
 */
VOID StoreMultiL2(ADDRINT addr, CACHE_ID cacheId, std::vector<char> evictData)
{
    assert(evictData.size() == 64); // always evict a cache line
    UINT32 size = 64; // granularity of a 64 byte cache line
    UINT32 instId = l2_profile.Map(addr);
    // printf("StoreMultiL2 addr: %lx, size %u\n", addr, size);
    // fflush(stdout);
    // check addr is 64 byte boundary
    assert((addr & 0x3f) == 0);

    // check if access can be compressed
    BOOL isCompressible = CheckCompressibleSize(addr, size);

    UINT32 dataSize = size;
    char *value;
    if (isCompressible) {
        if (KnobUseFPCompression.Value() == 1) {
            double compressed_cacheline[4];
            value = (char *)malloc(32);
            dataSize = 32;
            CacheDoubleFPCompress(&evictData[0], compressed_cacheline, 4);
            PIN_SafeCopy(value, &compressed_cacheline, 32);
        } else {
            char compressed_cacheline[32];
            value = (char *)malloc(32);
            dataSize = 32;
            CacheDownsampleCompress(&evictData[0], compressed_cacheline);
            PIN_SafeCopy(value, &compressed_cacheline, 32);
        }
    } else {
        value = (char *)malloc(64);
        PIN_SafeCopy(value, &evictData[0], 64);
    }

    const BOOL l2Hit = l2->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value, dataSize);

    free(value);

    const COUNTER counter = l2Hit ? COUNTER_HIT : COUNTER_MISS;
    l2_profile[instId][counter]++;
    // printf("SML2 done\n");
    // fflush(stdout);
}

/* ===================================================================== */

VOID LoadMulti(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadMulti addr: 0x%lx, size %u\n", addr, size);
    // fflush(stdout);
    char* value = (char*)malloc(size);

    UINT64 val, nval;
    val = 0;
    if (size >= 8) {
        PIN_SafeCopy(&val, (void*)addr, 8); // only check first 8 bytes
    } else {
        PIN_SafeCopy(&val, (void*)addr, size);
    }

    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value, 0);
    const BOOL dl1Hit = dl1Access.hit;

    nval = 0;
    if (size >= 8) {
        PIN_SafeCopy(&nval, (void*)value, 8);
    } else {
        PIN_SafeCopy(&nval, (void*)value, size);
    }

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    if (dl1Hit) {
        if (!KnobUseFPCompression.Value() && CheckCompressibleSize(addr, size) && val != nval) {
            printf("lm: prior %zu, after %zu\n", val, nval);
            fflush(stdout);
        }
        if (!KnobUseFPCompression.Value() && CheckCompressibleSize(addr, size)) {
            assert(val == nval); // check that actual value matches value in cache
        }
    } else { // call L2 cache as needed
        // assuming size <= 64, will never need to crack into more than 2 loads
        ADDRINT offset = addr & 0x3f;
        char *l2_data = (char*)malloc(64); // load from l2 is 64 bytes
        if (offset + size > 64) {
            // access is split across 2 cache lines
            val = 0;
            if (size >= 8) {
                PIN_SafeCopy(&val, (void*)addr, 8); // only check first 8 bytes
            } else {
                PIN_SafeCopy(&val, (void*)addr, size);
            }

            // access 1
            ADDRINT this_addr = addr;
            UINT32 this_size = 64 - offset;
            LoadMultiL2(this_addr, L1D_CACHE, l2_data);

            dl1->Access(this_addr, 64, CACHE_BASE::ACCESS_TYPE_STORE, l2_data, 0); // redo to update cache

            if (CheckCompressibleSize(this_addr, this_size)) {
                // write compressible value from cache to mem
                PIN_SafeCopy((void*)this_addr, (void*)(&(l2_data[offset])), this_size);
            }

            // access 2
            this_addr = this_addr + this_size;
            this_size = size - this_size;
            LoadMultiL2(this_addr + offset, L1D_CACHE, l2_data);

            dl1->Access(this_addr, 64, CACHE_BASE::ACCESS_TYPE_STORE, l2_data, 0); // redo to update cache

            if (CheckCompressibleSize(this_addr, this_size)) {
                PIN_SafeCopy((void*)(this_addr), l2_data, this_size); // write value from cache to mem

                nval = 0;
                if (size >= 8) {
                    PIN_SafeCopy(&nval, (void*)addr, 8);
                } else {
                    PIN_SafeCopy(&nval, (void*)addr, size);
                }
                if (!KnobUseFPCompression.Value() && val != nval) {
                    printf("post lml2 cracked: prior %zu, after %zu\n", val, nval);
                    fflush(stdout);
                }
                if (!KnobUseFPCompression.Value()) {
                    assert(val == nval); // check that actual value matches value in cache
                }
            }
        } else {
            LoadMultiL2(addr, L1D_CACHE, l2_data);
            // l2_data holds full 64 byte line

            nval = 0;
            if (size >= 8) {
                PIN_SafeCopy(&nval, (void*)&(l2_data[offset]), 8);
            } else {
                PIN_SafeCopy(&nval, (void*)&(l2_data[offset]), size);
            }

            dl1->Access(addr - offset, 64, CACHE_BASE::ACCESS_TYPE_STORE, l2_data, 0); // redo to update cache
            if (CheckCompressibleSize(addr, size)) {
                if (!KnobUseFPCompression.Value() && val != nval) {
                    printf("post lml2: prior %zu, after %zu\n", val, nval);
                    fflush(stdout);
                }
                if (!KnobUseFPCompression.Value()) {
                    assert(val == nval); // check that actual value matches value in cache
                }
                // write compressible value from cache to mem
                PIN_SafeCopy((void*)addr, (void*)(&(l2_data[offset])), size);
            }
        }
        free(l2_data);
    }
    if (dl1Access.evicted_dirty) {
        for (size_t evictIdx = 0; evictIdx < dl1Access.evicted_data.size(); ++evictIdx) {
            StoreMultiL2(dl1Access.evicted_addr, L1D_CACHE, dl1Access.evicted_data[evictIdx]);
        }
    }
    // printf("LM done\n");
    // fflush(stdout);
}

// /* ===================================================================== */

VOID StoreMulti(VOID * address, UINT32 size, UINT32 instId)
{
    // printf("StoreMulti addr: %p, size %u\n", address, size);
    // fflush(stdout);
    char* value = (char*)malloc(size);

    ADDRINT addr = (ADDRINT)address;

    UINT64 val, nval;
    val = 0;
    if (size >= 8) {
        PIN_SafeCopy(&val, (void*)addr, 8); // only check first 8 bytes
    } else {
        PIN_SafeCopy(&val, (void*)addr, size);
    }

    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value, 0);
    const BOOL dl1Hit = dl1Access.hit;

    nval = 0;
    if (size >= 8) {
        PIN_SafeCopy(&nval, (void*)value, 8);
    } else {
        PIN_SafeCopy(&nval, (void*)value, size);
    }
    if (!KnobUseFPCompression.Value()) {
        assert(val == nval); // check that actual value matches value in cache
    }

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // write-back cache never triggers miss on write
    // value is allocated in cache w/ dirty bit and the cache pushes on eviction
    if (dl1Access.evicted_dirty) {
        for (size_t evictIdx = 0; evictIdx < dl1Access.evicted_data.size(); ++evictIdx) {
            StoreMultiL2(dl1Access.evicted_addr, L1D_CACHE, dl1Access.evicted_data[evictIdx]);
        }
    }
    // printf("SM done\n");
    // fflush(stdout);
}

/* ===================================================================== */

VOID LoadSingle(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadSingle addr: 0x%lx, size %u\n", addr, size);
    // fflush(stdout);
    char* value = (char*)malloc(size);
    UINT64 val = 0;

    PIN_SafeCopy(&val, (void*)addr, size);
    PIN_SafeCopy(value, (void*)addr, size);

    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    const BOOL dl1Hit = dl1Access.hit;

    UINT64 nval = 0;
    PIN_SafeCopy(&nval, (void*)value, size);


    if (CheckCompressibleSize(addr, size)) {
        // write compressible value from cache to mem
        PIN_SafeCopy((void*)addr, (void*)value, size);
    }

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    if (dl1Hit) {
        if (!KnobUseFPCompression.Value() && CheckCompressibleSize(addr, size) && val != nval) {
            printf("ls: prior %zu, after %zu\n", val, nval);
            fflush(stdout);
        }
        if (!KnobUseFPCompression.Value() && CheckCompressibleSize(addr, size)) {
            assert(val == nval); // check that actual value matches value in cache
        }
    } else { // call l2 cache as needed
        // assuming size <= 64, will never need to crack into more than 2 loads
        ADDRINT offset = addr & 0x3f;
        char *l2_data = (char*)malloc(64); // load from l2 is 64 bytes

        LoadMultiL2(addr, L1D_CACHE, l2_data);
        // l2_data holds full 64 byte line

        nval = 0;
        if (size >= 8) {
            PIN_SafeCopy(&nval, (void*)&(l2_data[offset]), 8);
        } else {
            PIN_SafeCopy(&nval, (void*)&(l2_data[offset]), size);
        }

        dl1->Access(addr - offset, 64, CACHE_BASE::ACCESS_TYPE_STORE, l2_data, 0); // redo to update cache
        if (CheckCompressibleSize(addr, size)) {
            if (!KnobUseFPCompression.Value() && val != nval) {
            printf("post lsl2: prior %zu, after %zu\n", val, nval);
            fflush(stdout);
            }
            if (!KnobUseFPCompression.Value()) {
                assert(val == nval); // check that actual value matches value in cache
            }
            // write compressible value from cache to mem
            PIN_SafeCopy((void*)addr, (void*)(&(l2_data[offset])), size);
        }

        free(l2_data);
    }
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1D_CACHE, dl1Access.evicted_data[0]);
    }

    free(value);
    // printf("LS done\n");
    // fflush(stdout);
}

/* ===================================================================== */

VOID StoreSingle(VOID * address, UINT32 size, UINT32 instId)
{
    // printf("StoreSingle addr: %p, size %u\n", address, size);
    // fflush(stdout);
    char* value = (char*)malloc(size);

    ADDRINT addr = (ADDRINT)address;

    UINT64 val = 0;
    PIN_SafeCopy(&val, (void*)addr, size);
    PIN_SafeCopy(value, (void*)addr, size);

    const CACHE_ACCESS_STRUCT dl1Access = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    const BOOL dl1Hit = dl1Access.hit;

    UINT64 nval = 0;
    PIN_SafeCopy(&nval, (void*)value, size);
    if (!KnobUseFPCompression.Value()) {
        assert(val == nval); // check that actual value matches value in cache
    }

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // write-back cache never triggers miss on write
    // value is allocated in cache w/ dirty bit and the cache pushes on eviction
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1D_CACHE, dl1Access.evicted_data[0]);
    }
    // printf("SS done\n");
    // fflush(stdout);
}

/* ===================================================================== */

VOID LoadSingleInstruction(ADDRINT addr, UINT32 instId)
{
    // printf("LoadSingleInstruction addr: %lx\n", addr);
    // fflush(stdout);
    // assume 64 bit instruction
    UINT32 size = 8;

    const CACHE_ACCESS_STRUCT il1Access = il1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, NULL);
    const BOOL il1Hit = il1Access.hit;

    const COUNTER counter = il1Hit ? COUNTER_HIT : COUNTER_MISS;
    il1_profile[instId][counter]++;

    // call L2 cache as needed
    if (!il1Hit) {
        LoadMultiL2(addr, L1I_CACHE, NULL);
        ADDRINT offset = addr & 0x3f;
        il1->Access(addr - offset, 64, CACHE_BASE::ACCESS_TYPE_LOAD, NULL, 0); // redo to update cache
    }

    // icache should never evict anything dirty because never written
    assert(!il1Access.evicted_dirty);
    // printf("LSI done\n");
    // fflush(stdout);
}

/* ===================================================================== */

VOID RecordSkip(ADDRINT addr, UINT32 size)
{
    // printf("Skipping %u byte access at addr 0x%lx\n", size, addr);
    // fflush(stdout);
}

/* ===================================================================== */

VOID Instruction(INS ins, void *v)
{
    // all instructions incur a memory read for the instruction pointer
    const ADDRINT iaddr = INS_Address(ins);
    // map sparse INS addresses to dense IDs
    UINT32 instId = il1_profile.Map(iaddr);
    // instruction fetch is always single since fetched one at a time
    // this is not realistic for modern CPUs but is ok for a baseline
    INS_InsertPredicatedCall(
        ins, IPOINT_BEFORE, (AFUNPTR) LoadSingleInstruction,
        IARG_ADDRINT, iaddr, IARG_UINT32, instId,
        IARG_END);

    if (!INS_IsStandardMemop(ins)) return;

    UINT32 memOperands = INS_MemoryOperandCount(ins);
    if (memOperands == 0) return;

    UINT32 readSize=0, writeSize=0;
    UINT32 readOperandCount=0, writeOperandCount=0;

    // map sparse INS addresses to dense IDs
    instId = dl1_profile.Map(iaddr);

    for (UINT32 opIdx = 0; opIdx < INS_MemoryOperandCount(ins); opIdx++) {
        if (INS_MemoryOperandIsRead(ins, opIdx)) {
            readSize = INS_MemoryOperandSize(ins, opIdx);

            const BOOL single = (readSize <= 1);

            if ( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR) LoadSingle,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
                    IARG_UINT32, instId,
                    IARG_END);
            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadMulti,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
                    IARG_UINT32, instId,
                    IARG_END);
            }

            readOperandCount++;
        }
        if (INS_MemoryOperandIsWritten(ins, opIdx)) {
            writeSize = INS_MemoryOperandSize(ins, opIdx);

            const BOOL single = (writeSize <= 1);

            if (INS_IsValidForIpointAfter(ins))
            {
                if ( single )
                {
                    INS_InsertPredicatedCall(
                        ins, IPOINT_AFTER,  (AFUNPTR) StoreSingle,
                        IARG_MEMORYOP_EA, opIdx,
                        IARG_MEMORYWRITE_SIZE,
                        IARG_UINT32, instId,
                        IARG_END);
                }
                else
                {
                    INS_InsertPredicatedCall(
                        ins, IPOINT_AFTER,  (AFUNPTR) StoreMulti,
                        IARG_MEMORYOP_EA, opIdx,
                        IARG_MEMORYWRITE_SIZE,
                        IARG_UINT32, instId,
                        IARG_END);
                }

            } else {
                INS_InsertPredicatedCall(
                        ins, IPOINT_BEFORE,  (AFUNPTR) RecordSkip,
                        IARG_MEMORYOP_EA, opIdx,
                        IARG_MEMORYWRITE_SIZE,
                        IARG_END);
            }

            writeOperandCount++;
        }
    }
}

/* ===================================================================== */

VOID MarkCompressible(ADDRINT addr, ADDRINT size) {
    // outFile << "Marking addr 0x" << std::hex << addr << " with size " << size << " as compressible" << endl;
    cMap[addr] = size;
}

VOID Compress(IMG img, VOID *v) {
    for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym)) {
        string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
        //  Find the __COMPRESS__() function.
        if (undFuncName == "__COMPRESS__") {
            RTN compressRtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            if (RTN_Valid(compressRtn)) {
                RTN_Open(compressRtn);

                RTN_InsertCall(compressRtn, IPOINT_BEFORE, (AFUNPTR)MarkCompressible,
                               IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                               IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                               IARG_END);

                RTN_Close(compressRtn);
            }
        }
    }
}

/* ===================================================================== */

VOID Fini(int code, VOID *v)
{
    // print cache profiles
    // @todo what does this print

    outFile << "PIN:MEMLATENCIES 1.0. 0x0\n";

    outFile <<
        "#\n"
        "# CACHE stats\n"
        "#\n";

    outFile << il1->StatsLong("# ", CACHE_BASE::CACHE_TYPE_ICACHE);

    outFile << dl1->StatsLong("# ", CACHE_BASE::CACHE_TYPE_DCACHE);

    outFile << l2->StatsLong("# ", CACHE_BASE::CACHE_TYPE_DCACHE);

    outFile << "Compressible Load Misses: " << L2CompressibleMiss_CTR << "\n\n";

    outFile <<
        "#\n"
        "# L1D stats\n"
        "#\n";

    outFile << dl1_profile.StringLong();

    outFile <<
        "#\n"
        "# L2 stats\n"
        "#\n";

    outFile << l2_profile.StringLong();

    outFile.close();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    outFile.open(KnobOutputFile.Value().c_str());

    il1 = new LCACHE_L1::CACHE("L1 Instruction Cache",
        KnobL1CacheSize.Value() * KILO,
        KnobL1Associativity.Value());

    dl1 = new LCACHE_L1::CACHE("L1 Data Cache",
        KnobL1CacheSize.Value() * KILO,
        KnobL1Associativity.Value());

    l2 = new LCACHE_L2::CACHE("L2 Unified Cache",
        KnobL2CacheSize.Value() * KILO,
        KnobL2Associativity.Value());

    il1_profile.SetKeyName("iaddr          ");
    il1_profile.SetCounterName("cache:miss        cache:hit");

    dl1_profile.SetKeyName("iaddr          ");
    dl1_profile.SetCounterName("cache:miss        cache:hit");

    l2_profile.SetKeyName("iaddr          ");
    l2_profile.SetCounterName("cache:miss        cache:hit");

    COUNTER_HIT_MISS threshold;

    threshold[COUNTER_HIT] = KnobThresholdHit.Value();
    threshold[COUNTER_MISS] = KnobThresholdMiss.Value();

    il1_profile.SetThreshold( threshold );
    dl1_profile.SetThreshold( threshold );
    l2_profile.SetThreshold( threshold );

    L2CompressibleMiss_CTR = 0;

    INS_AddInstrumentFunction(Instruction, 0);
    IMG_AddInstrumentFunction(Compress, 0);
    PIN_AddFiniFunction(Fini, 0);

    /* testing compression functions */
    if (KnobTestFPCompression.Value()) {
        char cacheline_prior[64];
        char cacheline_post[64];
        int N = 4;
        INT64 rval;

        srand(time(NULL));

        size_t total_diff = 0;
        for (size_t ctr = 0; ctr < 1000; ++ctr) {
            for (size_t idx = 0; idx < 64; idx = idx + 8) {
                // rval = ((INT64)RAND()<<48) ^ ((INT64)RAND()<<32) ^ ((INT64)RAND()<<16) ^ ((INT64)RAND());
                rval = ((INT64)RAND()<<12) ^ ((INT64)RAND());
                // printf("random value %zd\n", rval);
                memcpy((&(cacheline_prior[idx])), &rval, 8);
                // for (size_t i = 0; i < 8; ++i) {
                //     printf("cacheline_prior[%zu] = %d\n", idx + i, (unsigned char)cacheline_prior[idx + i]);
                // }
            }

            double compressed_cacheline[N];
            CacheDoubleFPCompress(cacheline_prior, compressed_cacheline, N);

            CacheDoubleFPDecompress(compressed_cacheline, cacheline_post, N);

            // for (size_t idx = 0; idx < 64; ++idx) {
            //     printf("cacheline_post[%zu] = %d\n", idx, (unsigned char)cacheline_post[idx]);
            // }
            size_t cumulative_diff = 0;
            for (size_t idx = 0; idx < 64; ++idx) {
                int diff = cacheline_post[idx] - cacheline_prior[idx];
                cumulative_diff = cumulative_diff + abs(diff);
            }
            // printf("cumulative_diff due to compression: %zu\n", cumulative_diff);
            total_diff += cumulative_diff;
        }
        printf("Total diff is %zu\n", total_diff);
    }

    // Never returns

    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
