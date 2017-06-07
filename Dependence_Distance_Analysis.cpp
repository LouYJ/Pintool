#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include "pin.H"

ofstream OutFile;
/*Record the distance from the instruction whose destination 
operand is current instruction's source operand to current instruction.*/
map<REG, UINT64> DistFromRegDes;
map<UINT64, UINT64> DistToNum;
struct regRecord
{
    vector<REG> src;
    vector<REG> dst;
    struct RegRecord *_next;
};

/*This function is used to add the distance when a new instruction is executed.*/
VOID distanceAdd()
{
    for(map<REG, UINT64>::iterator it=DistFromRegDes.begin(); it!=DistFromRegDes.end(); ++it) 
    {
        it->second += 1;
    }
}

/*This function is used to get all distance to current instruction.*/
VOID dist_docount(vector<REG> *ptr_regSource)
{
    vector<REG> regSrc = *ptr_regSource;
    for (vector<REG>::iterator it = regSrc.begin(); it != regSrc.end(); ++it)
    {
        /*When current source register is a destination registor before,
        record the dependence distance.*/
        if (DistFromRegDes.count(*it) != 0)
        {
            UINT32 dist = DistFromRegDes[*it];
            if (DistToNum.count(dist) > 0)
            {
                DistToNum[dist]++;
            }
            else
            {
                DistToNum[dist] = 1;
            }
        }
    }
}

/*This function is used to make the distance to current destination register zero.*/
VOID reg_as_dst(vector<REG> *ptr_regDestination)
{
    vector<REG> regDst = *ptr_regDestination;
    for(vector<REG>::iterator it = regDst.begin(); it != regDst.end(); ++it) 
    {
        DistFromRegDes[*it] = 0;
    }
}

VOID Instruction(INS ins, VOID *v)
{
    vector<REG> vec1;
    vector<REG> vec2;
    //Insert the function distanceAdd.
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)distanceAdd, IARG_END);

    regRecord *RegRecord = new regRecord;
    RegRecord->src = vec1;
    RegRecord->dst = vec2;

    UINT32 n = INS_OperandCount(ins);
    // It's used to see what the ins it is.
    // OutFile << INS_Disassemble(ins) << endl;

    for (UINT32 i = 0; i < n; i++) 
    {
        // OutFile << INS_OperandReg(ins, i) << endl;
        // OutFile << INS_OperandRead(ins, i) << endl;
        // Judge the operand is a register and a source operand.
        if (INS_OperandIsReg(ins, i) && INS_OperandRead(ins, i))
        {
            RegRecord->src.push_back(INS_OperandReg(ins, i));
        } 
        // It's a destination operand.
        else if(INS_OperandIsReg(ins, i) && INS_OperandWritten(ins, i)) 
        {
            RegRecord->dst.push_back(INS_OperandReg(ins, i));
        }
    }

    //Insert the function dist_count and reg_as_dst.
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)dist_docount, IARG_PTR, &(RegRecord->src), IARG_END);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)reg_as_dst, IARG_PTR, &(RegRecord->dst), IARG_END);
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "Dependence_Distance_Analysis.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);

    UINT64 sum = 0;

    for(map<UINT64, UINT64>::iterator it=DistToNum.begin(); it != DistToNum.end(); ++it) 
    {
        sum += it->second;
    }

    OutFile << "The dependence distance analysis:" << endl;

    UINT64 cnt = 0;
    for(map<UINT64, UINT64>::iterator it=DistToNum.begin(); it != DistToNum.end(); ++it) 
    {
        cnt++;
        OutFile << "Dependence distance: " << it->first << " --> " << it->second << " --> " << it->second / (0.01*sum) << "%" << endl;
        if(cnt == 30) 
        {
            break;
        }
    }

    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}   