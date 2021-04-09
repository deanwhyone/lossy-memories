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
    "l2a","4", "cache associativity (1 for direct mapped)");
KNOB<UINT32> KnobTestFPCompression(KNOB_MODE_WRITEONCE, "pintool",
    "fpc","0", "run test on floating point compression accuracy");

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
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 64; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;
    const UINT32 line_size = 64;
    const bool keep_data = true;

    typedef CACHE_LRU(max_sets, max_associativity, allocation, line_size, keep_data) CACHE;
}
namespace LCACHE_L2
{
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 64; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;
    const UINT32 line_size = 64;
    const bool keep_data = false;

    typedef CACHE_LRU(max_sets, max_associativity, allocation, line_size, keep_data) CACHE;
}

LCACHE_L1::CACHE* dl1 = NULL;
LCACHE_L1::CACHE* il1 = NULL;
LCACHE_L2::CACHE* l2 = NULL;

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

/*
 * Called on load miss in either L1 cache
 * Always loads a full cache line from L2, store into appropriate L1
 */
VOID LoadMultiL2(ADDRINT addr, CACHE_ID cacheId, UINT32 instId)
{
    UINT32 size = 64; // granularity of a 64 byte cache line
    // printf("LoadMultiL2 addr: 0x%lx, size %u\n", addr, size);

    // align addr with 64 byte boundary
    addr = addr & 0xfffffffffffffe00;

    const BOOL l2Hit = l2->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, NULL);

    const COUNTER counter = l2Hit ? COUNTER_HIT : COUNTER_MISS;
    l2_profile[instId][counter]++;
}

/* ===================================================================== */

/*
 * Called on eviction of dirty data in L1D (L1I never evicts dirty data)
 */
VOID StoreMultiL2(ADDRINT addr, CACHE_ID cacheId, UINT32 instId)
{
    UINT32 size = 64; // granularity of a 64 byte cache line
    // printf("StoreMultiL2 addr: %p, size %u\n", address, size);
    // align addr with 64 byte boundary
    addr = addr & 0xfffffffffffffe00;

    const BOOL l2Hit = l2->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, NULL);

    const COUNTER counter = l2Hit ? COUNTER_HIT : COUNTER_MISS;
    l2_profile[instId][counter]++;
}

/* ===================================================================== */

VOID LoadMulti(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadMulti addr: 0x%lx, size %u\n", addr, size);
    char* value = (char*)malloc(size);

    UINT64 val, nval;
    val = 0;
    PIN_SafeCopy(&val, (void*)addr, 8); // only check first 8 bytes

    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    const BOOL dl1Hit = dl1Access.hit;

    nval = 0;
    PIN_SafeCopy(&nval, (void*)value, 8);
    if (val != nval) {
        printf("lm: prior %zu, after %zu\n", val, nval);
    }
    assert(val == nval); // check that actual value matches value in cache

    PIN_SafeCopy((void*)addr, (void*)value, size); // copying value from cache into mem

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // call L2 cache as needed
    if (!dl1Hit) {
        LoadMultiL2(addr, L1D_CACHE, instId);
    }
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1I_CACHE, instId);
    }
}

/* ===================================================================== */

VOID StoreMulti(VOID * address, UINT32 size, UINT32 instId)
{
    // printf("StoreMulti addr: %p, size %u\n", address, size);
    char* value = (char*)malloc(size);

    ADDRINT addr = (ADDRINT)address;

    UINT64 val, nval;
    val = 0;
    PIN_SafeCopy(&val, (void*)addr, 8); // only check first 8 bytes

    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    const BOOL dl1Hit = dl1Access.hit;

    nval = 0;
    PIN_SafeCopy(&nval, (void*)value, 8);
    assert(val == nval); // check that actual value matches value in cache

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // write-back cache never triggers miss on write
    // value is allocated in cache w/ dirty bit and the cache pushes on eviction
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1I_CACHE, instId);
    }
}

/* ===================================================================== */

VOID LoadSingle(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadSingle addr: 0x%lx, size %u\n", addr, size);
    char* value = (char*)malloc(size);

    UINT64 val = 0;
    PIN_SafeCopy(&val, (void*)addr, size);

    PIN_SafeCopy(value, (void*)addr, size);

    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    const BOOL dl1Hit = dl1Access.hit;

    UINT64 nval = 0;
    PIN_SafeCopy(&nval, (void*)value, size);
    assert(val == nval); // check that actual value matches value in cache

    PIN_SafeCopy((void*)addr, (void*)value, size); // copying value from cache into mem

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // call L2 cache as needed
    if (!dl1Hit) {
        LoadMultiL2(addr, L1D_CACHE, instId);
    }
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1I_CACHE, instId);
    }
}

/* ===================================================================== */

VOID StoreSingle(VOID * address, UINT32 size, UINT32 instId)
{
    // printf("StoreSingle addr: %p, size %u\n", address, size);
    char* value = (char*)malloc(size);

    ADDRINT addr = (ADDRINT)address;

    UINT64 val = 0;
    PIN_SafeCopy(&val, (void*)addr, size);

    PIN_SafeCopy(value, (void*)addr, size);
    // @todo we may access several cache lines for
    // first level D-cache
    const CACHE_ACCESS_STRUCT dl1Access = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    const BOOL dl1Hit = dl1Access.hit;

    UINT64 nval = 0;
    PIN_SafeCopy(&nval, (void*)value, size);
    assert(val == nval); // check that actual value matches value in cache

    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;

    // write-back cache never triggers miss on write
    // value is allocated in cache w/ dirty bit and the cache pushes on eviction
    if (dl1Access.evicted_dirty) {
        StoreMultiL2(dl1Access.evicted_addr, L1I_CACHE, instId);
    }
}

/* ===================================================================== */

VOID LoadSingleInstruction(ADDRINT addr, UINT32 instId)
{
    // assume 64 bit instruction
    UINT32 size = 8;

    const CACHE_ACCESS_STRUCT il1Access = il1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, NULL);
    const BOOL il1Hit= il1Access.hit;

    const COUNTER counter = il1Hit ? COUNTER_HIT : COUNTER_MISS;
    il1_profile[instId][counter]++;

    // call L2 cache as needed
    if (!il1Hit) {
        LoadMultiL2(addr, L1I_CACHE, instId);
    }
    if (il1Access.evicted_dirty) {
        StoreMultiL2(il1Access.evicted_addr, L1I_CACHE, instId);
    }
}

/* ===================================================================== */

VOID Instruction(INS ins, void * v)
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

    // map sparse INS addresses to dense IDs
    instId = dl1_profile.Map(iaddr);

    if (!INS_IsStandardMemop(ins)) return;

    UINT32 memOperands = INS_MemoryOperandCount(ins);
    if (memOperands == 0) return;

    UINT32 readSize=0, writeSize=0;
    UINT32 readOperandCount=0, writeOperandCount=0;

    for (UINT32 opIdx = 0; opIdx < INS_MemoryOperandCount(ins); opIdx++) {
        if (INS_MemoryOperandIsRead(ins, opIdx)) {
            readSize = INS_MemoryOperandSize(ins, opIdx);

            const BOOL single = (readSize <= 8);

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

            const BOOL single = (writeSize <= 8);

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

            }

            writeOperandCount++;
        }
    }
}

/* ===================================================================== */

VOID Fini(int code, VOID * v)
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

    INS_AddInstrumentFunction(Instruction, 0);
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
