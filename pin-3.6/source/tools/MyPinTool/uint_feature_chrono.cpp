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
//
// This tool counts the number of times a routine is executed and 
// the number of instructions executed in a routine
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pin.H"

#include <chrono>

using clock = std::chrono::high_resolution_clock;

ofstream outFile;
clock::duration * time_table;
unsigned int *feature_table;
int table_index;
int table_size;
int table_allocated_size = 100000;

VOID init_tables(){
	table_index = 0;
	table_size = 0;
	feature_table = (unsigned int *) malloc(table_allocated_size * sizeof(unsigned int));
	time_table = new clock::duration[table_allocated_size];
}

VOID free_tables(){
	free(feature_table);
	delete[](time_table);	
}

VOID measure_feature(unsigned int feature_value){
	if (table_size < table_allocated_size)
		table_size += 1;

	feature_table[table_index] = feature_value;
	entry_time = clock::now();

}

VOID measure_time() {
        time_table[table_index] = clock::now() - entry_time;
	table_index = (table_index + 1) % table_allocated_size;
}

const char * target = 0;
int feature_pos = -1;

VOID Image(IMG img, VOID *v)
{   
    //  Find the function.
    RTN featureRtn = RTN_FindByName(img, target);
    if (RTN_Valid(featureRtn))
    {   
        RTN_Open(featureRtn);
        
        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(featureRtn, IPOINT_BEFORE, (AFUNPTR)measure_feature,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, feature_pos,
                       IARG_END);
        
        RTN_Close(featureRtn);
        measure_time();
    }
}
       
// This function is called when the application exits
VOID Fini(INT32 code, VOID *v){
   
    outFile << "Table size: " << table_size << endl;
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
    cerr << "This Pintool counts the number of times a routine is executed" << endl;
    cerr << "and the number of instructions executed in a routine" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
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

   // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    outFile.open(output_filename);

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    
    init_tables();
    
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
