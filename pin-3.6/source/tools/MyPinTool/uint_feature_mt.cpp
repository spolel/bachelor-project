/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2017 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <thread>
#include <chrono>
#include "pin.H"

ofstream outFile;
unsigned int *feature_table;
clock_t *time_table;
int table_index;
int table_size;
int table_allocated_size = 100000;

VOID init_tables(){
        table_index = 0;
        table_size = 0;
        feature_table = (unsigned int *) malloc(table_allocated_size * sizeof(unsigned int));
        time_table = (clock_t *) malloc(table_allocated_size * sizeof(clock_t));
}

VOID free_tables(){
        free(feature_table);
        free(time_table);
}

//==============================================================
//  Analysis Routines
//==============================================================
// Note:  threadid+1 is used as an argument to the PIN_GetLock()
//        routine as a debugging aid.  This is the value that
//        the lock is set to, so it must be non-zero.

// lock serializes access to the output file.
PIN_LOCK pinLock;

// Note that opening a file in a callback is only supported on Linux systems.
// See buffer-win.cpp for how to work around this issue on Windows.
//
// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, threadid+1);
    outFile << "thread begin " << threadid << endl;
    PIN_ReleaseLock(&pinLock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&pinLock, threadid+1);
    outFile << "thread end " << threadid << endl;
    PIN_ReleaseLock(&pinLock);
}

//====================================================================
// Instrumentation Routines
//====================================================================

VOID measure_feature(unsigned int feature_value, THREADID threadid){
        PIN_GetLock(&pinLock, threadid+1);
	
	outFile << "entered thread " << threadid << endl;
	
	if (table_size < table_allocated_size)
                table_size += 1;

        feature_table[table_index] = feature_value;
        time_table[table_index] = clock();

	PIN_ReleaseLock(&pinLock);
}

VOID measure_time() {
        time_table[table_index] = clock() - time_table[table_index];
        table_index = (table_index + 1) % table_allocated_size;
}


const char * target = 0;
int feature_pos = -1;


// This routine is executed for each image.
VOID ImageLoad(IMG img, VOID *)
{
//  Find the function.
    RTN featureRtn = RTN_FindByName(img, target);
    if (RTN_Valid(featureRtn))
    {   
        RTN_Open(featureRtn);
        
        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(featureRtn, IPOINT_BEFORE, (AFUNPTR)measure_feature,                   
                       IARG_FUNCARG_ENTRYPOINT_VALUE, feature_pos,
                       IARG_THREAD_ID, IARG_END);
        RTN_InsertCall(featureRtn, IPOINT_AFTER, (AFUNPTR)measure_time, IARG_END);    
        RTN_Close(featureRtn);
    }

}

// This routine is executed once at the end.
VOID Fini(INT32 code, VOID *v)
{
    outFile << "Feature;Time(s)" << endl;

    for (int i = 0; i < table_size; i++)
    {
        outFile << feature_table[i] << ";" << ((double)time_table[i])/CLOCKS_PER_SEC << endl;
    }

    free_tables();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of malloc calls in the guest application\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(INT32 argc, CHAR **argv)
{
   
    const char * output_filename = 0;

    for (int i = 1; i < argc; ++i)
        if (strncmp(argv[i], "target=", 7) == 0) {
            target = argv[i] + 7;
        } else if (strncmp(argv[i], "out=", 4) == 0) {
            output_filename = argv[i] + 4;
        } else if (sscanf(argv[i], "feature=%d", &feature_pos) == 1) {
            continue;
        } else {
            std::cerr << "Warning: unknown command-line option: " << argv[i] << std::endl;
        }

    if (target == 0 || feature_pos < 0 || output_filename == 0) {
            std::cerr << "Usage: " << argv[0] << "target=<target-function> out=<filename> feature=<feature-par-pos>" << std::endl;
            return 1;
    }


    // Initialize the pin lock
    PIN_InitLock(&pinLock);
    
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();
    
    init_tables();


    outFile.open(output_filename);

    // Register ImageLoad to be called when each image is loaded.
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Analysis routines to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
