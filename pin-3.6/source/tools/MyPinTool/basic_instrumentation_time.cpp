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

#include <time.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <stack>
#include "pin.H"

#define MAX_THREADS 100

ofstream outFile;

std::vector<clock_t> clock_table[MAX_THREADS];
std::vector<struct timeval> wall_time_table[MAX_THREADS];
std::vector<unsigned int> feature_table[MAX_THREADS];
std::stack<unsigned int> executing[MAX_THREADS];
int table_index[MAX_THREADS];

unsigned int max_tid = 0;

//const char *target_name = "_Z1fij";
#define TARGET_NAME "_ZN12Field_string4packEPhPKhj"
#define FEATURE_POS 2
#define OUT_FILE_NAME "basic_instrumentation_time.out"

//int feature_pos = 1;

VOID init_tables(){
	for (int t = 0; t < MAX_THREADS; t++) {
		table_index[t] = -1;
	}
}

VOID free_tables(){
	for (int i = 0; i < MAX_THREADS; i++) {
		clock_table[i].clear();
		wall_time_table[i].clear();
		feature_table[i].clear();
	}
}

VOID measure_feature(UINT32 feature_value, THREADID tid){
	//tid--;
	//outFile << tid << endl;
	//if (tid >= MAX_THREADS)
	//	return;
	if(tid > max_tid){
		max_tid = tid;	
	}
	table_index[tid]++;
	executing[tid].push(table_index[tid]);
	feature_table[tid].push_back(feature_value);
	clock_table[tid].push_back(clock());
	struct timeval now;
	gettimeofday(&now, 0);
	wall_time_table[tid].push_back(now);
	//outFile << "Entering f at " << clock_table[tid][table_index[tid]] << " with feature " << feature_value << " and t_index " << table_index[tid] << "; thread " <<  tid  << std::endl;
}

VOID measure_time(INT32 ret_value, THREADID tid) {
	//tid--;
	//if (tid >= MAX_THREADS)
	//	return;
	if(tid > max_tid){
                max_tid = tid;
        }
	unsigned int index = executing[tid].top();
	clock_table[tid][index] = clock() - clock_table[tid][index];
	struct timeval now;
	gettimeofday(&now, 0);
	wall_time_table[tid][index].tv_sec = now.tv_sec - wall_time_table[tid][index].tv_sec; 
	if (now.tv_usec < wall_time_table[tid][index].tv_usec) {
		wall_time_table[tid][index].tv_sec -= 1;
		wall_time_table[tid][index].tv_usec = 1000000 + now.tv_usec - wall_time_table[tid][index].tv_usec;
	} else {
		wall_time_table[tid][index].tv_usec = now.tv_usec - wall_time_table[tid][index].tv_usec;
	}
	//std::cout << "Exiting f at " << clock_table[tid][index] << " which returns at t_index " << index << std::endl;
	executing[tid].pop();
}

VOID Image(IMG img, VOID *v)
{   
    //  Find the function.
    RTN featureRtn = RTN_FindByName(img, TARGET_NAME);
    if (RTN_Valid(featureRtn))
    {   

   	RTN_Open(featureRtn);
        
        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(featureRtn, IPOINT_BEFORE, (AFUNPTR)measure_feature,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, FEATURE_POS,
                       IARG_THREAD_ID,
		       IARG_END);
        
        RTN_InsertCall(featureRtn, IPOINT_AFTER, (AFUNPTR)measure_time,
                       IARG_FUNCRET_EXITPOINT_VALUE, 
                       IARG_THREAD_ID,
		       IARG_END);
        RTN_Close(featureRtn);
    }
}
       
// This function is called when the application exits
VOID Fini(INT32 code, VOID *v){
   
    outFile << "Feature;Time(s)" << endl;   

    for (unsigned int t = 0; t <= max_tid; t++) {
	outFile << std::endl << "## THREAD " << t << " ##" << std::endl;
        for (unsigned int i = 0; i < feature_table[t].size(); i++) {   
		outFile << "Feature: " << feature_table[t][i] << "; Clock: " << ((double)clock_table[t][i]) / CLOCKS_PER_SEC << "; Wall-time: " << (double)wall_time_table[t][i].tv_sec + (double)wall_time_table[t][i].tv_usec / 1000000 << endl;
	}
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
    

    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    outFile.open(OUT_FILE_NAME);

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    
    init_tables();
    
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

