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
KNOB<BOOL>   KnobTrackLoads(KNOB_MODE_WRITEONCE,    "pintool",
    "tl", "0", "track individual loads -- increases profiling time");
KNOB<BOOL>   KnobTrackStores(KNOB_MODE_WRITEONCE,   "pintool",
   "ts", "0", "track individual stores -- increases profiling time");
KNOB<UINT32> KnobThresholdHit(KNOB_MODE_WRITEONCE , "pintool",
   "rh", "100", "only report memops with hit count above threshold");
KNOB<UINT32> KnobThresholdMiss(KNOB_MODE_WRITEONCE, "pintool",
   "rm","100", "only report memops with miss count above threshold");
KNOB<UINT32> KnobCacheSize(KNOB_MODE_WRITEONCE, "pintool",
    "c","32", "cache size in kilobytes");
KNOB<UINT32> KnobAssociativity(KNOB_MODE_WRITEONCE, "pintool",
    "a","4", "cache associativity (1 for direct mapped)");

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
namespace LCACHE
{
    const UINT32 max_sets = KILO; // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 256; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;
    const UINT32 line_size = 64;
    const bool keep_data = true;

    typedef CACHE_LRU(max_sets, max_associativity, allocation, line_size, keep_data) CACHE;
}

LCACHE::CACHE* dl1 = NULL;
LCACHE::CACHE* il1 = NULL;

typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;



typedef  COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;


// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> il1_profile;
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> dl1_profile;

/* ===================================================================== */

VOID LoadMulti(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadMulti size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const BOOL dl1Hit = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;
}

/* ===================================================================== */

VOID StoreMulti(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("StoreMulti size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    // first level D-cache
    const BOOL dl1Hit = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;
}

/* ===================================================================== */

VOID LoadSingle(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("LoadSingle size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    // @todo we may access several cache lines for
    // first level D-cache
    const BOOL dl1Hit = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;
}
/* ===================================================================== */

VOID StoreSingle(ADDRINT addr, UINT32 size, UINT32 instId)
{
    // printf("StoreSingle size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    // @todo we may access several cache lines for
    // first level D-cache
    const BOOL dl1Hit = dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    free(value);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    dl1_profile[instId][counter]++;
}

/* ===================================================================== */

VOID LoadMultiFast(ADDRINT addr, UINT32 size)
{
    // printf("LoadMultiFast size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    free(value);
}

/* ===================================================================== */

VOID StoreMultiFast(ADDRINT addr, UINT32 size)
{
    // printf("StoreMultiFast size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    free(value);
}

/* ===================================================================== */

VOID LoadSingleFast(ADDRINT addr, UINT32 size)
{
    // printf("LoadSingleFast size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    free(value);
}

/* ===================================================================== */

VOID StoreSingleFast(ADDRINT addr, UINT32 size)
{
    // printf("StoreSingleFast size %u\n", size);
    char* value = (char*)malloc(size);
    PIN_SafeCopy(value, (void*)addr, size);
    dl1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_STORE, value);
    free(value);
}

/* ===================================================================== */

VOID LoadSingleInstruction(ADDRINT addr, UINT32 instId)
{
    // assume 64 bit instruction
    UINT32 size = 8;
    char* value = (char*)malloc(size);
    // cannot copy instruction data, will cause segfault, instead create value
    char instruction[9] = "FFFFFFFF";
    memcpy(value, instruction, 8);
    const BOOL il1Hit = il1->AccessSingleLine(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD, value);
    free(value);
    const COUNTER counter = il1Hit ? COUNTER_HIT : COUNTER_MISS;
    il1_profile[instId][counter]++;
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

    if (!INS_IsStandardMemop(ins)) return;

    if (INS_MemoryOperandCount(ins) == 0) return;

    UINT32 readSize=0, writeSize=0;
    UINT32 readOperandCount=0, writeOperandCount=0;

    for (UINT32 opIdx = 0; opIdx < INS_MemoryOperandCount(ins); opIdx++)
    {
        if (INS_MemoryOperandIsRead(ins, opIdx))
        {
            readSize = INS_MemoryOperandSize(ins, opIdx);
            readOperandCount++;
            break;
        }
        if (INS_MemoryOperandIsWritten(ins, opIdx))
        {
            writeSize = INS_MemoryOperandSize(ins, opIdx);
            writeOperandCount++;
            break;
        }
    }

    // map sparse INS addresses to dense IDs
    instId = dl1_profile.Map(iaddr);

    if (readOperandCount > 0)
    {
        const BOOL single = (readSize <= 8);

        if ( KnobTrackLoads )
        {
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

        }
        else
        {
            if ( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadSingleFast,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
                    IARG_END);

            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) LoadMultiFast,
                    IARG_MEMORYREAD_EA,
                    IARG_MEMORYREAD_SIZE,
                    IARG_END);
            }
        }
    }

    if (writeOperandCount > 0)
    {
        const BOOL single = (writeSize <= 8);

        if ( KnobTrackStores )
        {
            if ( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingle,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_UINT32, instId,
                    IARG_END);
            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreMulti,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_UINT32, instId,
                    IARG_END);
            }

        }
        else
        {
            if ( single )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingleFast,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_END);

            }
            else
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) StoreMultiFast,
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_END);
            }
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

    if( KnobTrackLoads || KnobTrackStores ) {
        outFile <<
            "#\n"
            "# LOAD stats\n"
            "#\n";

        outFile << dl1_profile.StringLong();
    }
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

    il1 = new LCACHE::CACHE("L1 Instruction Cache",
        KnobCacheSize.Value() * KILO,
        KnobAssociativity.Value());

    dl1 = new LCACHE::CACHE("L1 Data Cache",
        KnobCacheSize.Value() * KILO,
        KnobAssociativity.Value());

    il1_profile.SetKeyName("iaddr          ");
    il1_profile.SetCounterName("cache:miss        cache:hit");

    dl1_profile.SetKeyName("iaddr          ");
    dl1_profile.SetCounterName("cache:miss        cache:hit");

    COUNTER_HIT_MISS threshold;

    threshold[COUNTER_HIT] = KnobThresholdHit.Value();
    threshold[COUNTER_MISS] = KnobThresholdMiss.Value();

    il1_profile.SetThreshold( threshold );
    dl1_profile.SetThreshold( threshold );

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns

    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
