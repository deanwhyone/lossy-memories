/*
 * Copyright 2002-2020 Intel Corporation.
 *
 * This software is provided to you as Sample Sourc-> Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include "pin.H"
using std::string;
using std::hex;
using std::setw;
using std::cerr;
using std::dec;
using std::endl;

// Holds instruction count for a single procedure
typedef struct RtnCount {
    string _name;
    string _image;
    ADDRINT _address;
    RTN _rtn;
} RTN_COUNT;

// routine info
RTN_COUNT * rc = 0;

FILE * trace;

const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

int IsInteresting(int value) {
    int is_interesting = 0;

    if (value == 15 || value == 31 || value == 46) {
        is_interesting = 1;
    }

    return is_interesting;
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size) {
    size_t value;
    PIN_SafeCopy(&value, addr, size);
    fprintf(trace,"%p: R %p, %zu\n", ip, addr, value);
    if (IsInteresting(value)) {
        value = value * 2;
        PIN_SafeCopy(addr, &value, size);
    }
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size) {
    size_t value;
    PIN_SafeCopy(&value, addr, size);
    fprintf(trace,"%p: W %p, %zu\n", ip, addr, value);
    if (IsInteresting(value)) {
        value = value * 2;
        PIN_SafeCopy(addr, &value, size);
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v) {
    string fn_name = "main";

    IMG image = IMG_FindByAddress(INS_Address(ins));

    if (IMG_Valid(image)) {
        // string image_name = IMG_Name(image);
        // fprintf(trace, "This image is named %s", image_name.c_str());
        if (IMG_IsMainExecutable(image)) {
            // fprintf(trace, " and is the main exe\n");
            // Instruments memory accesses using a predicated call, i.e.
            // the instrumentation is called iff the instruction will actually be executed.
            //
            // On the IA-32 and Intel(R) 64 arc->itectures conditional moves and REP
            // prefixed instructions appear as predicated instructions in Pin.
            UINT32 memOperands = INS_MemoryOperandCount(ins);

            // Iterate over each memory operand of the instruction.
            for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
                if (INS_MemoryOperandIsRead(ins, memOp))
                {
                    INS_InsertPredicatedCall(
                        ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                        IARG_INST_PTR,
                        IARG_MEMORYOP_EA, memOp,
                        IARG_MEMORYREAD_SIZE,
                        IARG_END);
                }
                // Note that in some arc->itectures a single memory operand can be
                // both read and written (for instance incl (%eax) on IA-32)
                // In that case we instrument it once for read and once for write.
                if (INS_MemoryOperandIsWritten(ins, memOp) && INS_IsValidForIpointAfter(ins)) {
                    INS_InsertPredicatedCall(
                        ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite,
                        IARG_INST_PTR,
                        IARG_MEMORYOP_EA, memOp,
                        IARG_MEMORYWRITE_SIZE,
                        IARG_END);
                }
            }
        } else {
            // fprintf(trace, "\n");
        }
    }
}

// Pin calls this function every time a new rtn is executed
VOID Routine(RTN rtn, VOID *v) {
    // The RTN goes away when the image is unloaded, so save it now

    // Allocate a counter for this routine
    rc = new RTN_COUNT;

    rc->_name = RTN_Name(rtn);
    rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
    rc->_address = RTN_Address(rtn);

    fprintf(trace, "Now in routine %s\n", rc->_name.c_str());
}

VOID Fini(INT32 code, VOID *v) {
    fprintf(trace, "#eof\n");
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
    PIN_ERROR( "This is a custom Pintool\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[]) {
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("lossy.out", "w");

    // RTN_AddInstrumentFunction(Routine, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
