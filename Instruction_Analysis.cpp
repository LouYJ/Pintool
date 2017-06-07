#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include "pin.H"
extern "C" {
    #include "xed-interface.h"
}
ofstream OutFile;

map<string, UINT64> InsToNum;
map<UINT64, UINT64> ImmbitToNum;
map<UINT64, UINT64> EncodeMap;

//This data structure is used to record instruction.
typedef struct _instruction
{
    string ins_name;//Record the name of instruction
    UINT64 bit_cnt;//Record the bit number
    UINT64 symbol;//Record the way of seeking for address
    struct _instruction * _next;
} _INS;

typedef pair<string, UINT64> PAIR;  

bool cmp_by_value(const PAIR& lhs, const PAIR& rhs) 
{  
    return lhs.second > rhs.second;  
} 

/*This function is used to output the first part of result.*/
VOID part1_output(map<string, UINT64> mp)
{   
    OutFile << "----------------Part1----------------" << endl;
    //Sort the InsToNum map.
    vector<PAIR> vec_mp(mp.begin(), mp.end());  
    sort(vec_mp.begin(), vec_mp.end(), cmp_by_value); 

    UINT64 sum = 0; 

    for(vector<PAIR>::iterator it=vec_mp.begin(); it != vec_mp.end(); ++it) 
    {
        sum += it->second;
    }

    OutFile << "The total number of instruction: " << sum << endl << 
    endl << "Here is ten of the most frequent used instructions:" << endl;


    UINT num = 0;
    OutFile << "Instruction --> Number --> Proportion:" << endl;
    for(vector<PAIR>::iterator it=vec_mp.begin(); it != vec_mp.end(); ++it) 
    {
        num++;
        OutFile << it->first << " --> " << it->second << " --> " << it->second / (0.01*sum) << "%" << endl;

        //Just output 10 results.
        // if (num == 10)
        // {
        //     break;
        // }
    }
    OutFile << endl;
}

/*This function is used to output the second part of result.*/
VOID part2_output(map<UINT64, UINT64> mp)
{
    OutFile << "----------------Part2----------------" << endl;
    UINT64 sum = 0;
    OutFile << "The usage of immediate: " << endl;
    for(map<UINT64, UINT64>::iterator it=mp.begin(); it != mp.end(); ++it) 
    {
        sum += it->second;
    }
    OutFile << "Number of immediate: " << sum << endl;

    for(map<UINT64, UINT64>::iterator it= mp.begin(); it != mp.end(); ++it) 
    {
        OutFile << "The effective bit number: " << it->first << " --> " << it->second << " --> " << it->second / (0.01*sum) << "%" << endl;
    }
    OutFile << endl;
}

/*This function is used to output the third part of result.*/
VOID part3_output(map<UINT64, UINT64> mp)
{
    map<UINT64, string> IntToName;
    IntToName[0] = "Immediate Addressing";
    IntToName[1] = "Register Addressing"; 
    IntToName[2] = "Direct Addressing";
    IntToName[7] = "Scale Factor Indexed Addressing";
    IntToName[8] = "Register Indirect Addressing";
    IntToName[10]= "Register Relative Addressing";
    IntToName[12]= "Based Indexed Addressing";
    IntToName[13]= "Based Scaled Indexed Addressing";
    IntToName[14]= "Relative Based Indexed Addressing";
    IntToName[15]= "Relative Based Scaled Indexed Addressing";

    OutFile << "----------------Part3----------------" << endl;
    UINT64 sum = 0;
    for(map<UINT64, UINT64>::iterator it=mp.begin(); it != mp.end(); ++it) 
    {
        if (it->first == 5) 
        {
            mp[7] += it->second;
            continue;
        }
        sum += it->second;
    }
    OutFile << "Different addressing:" << endl;
    for(map<UINT64, UINT64>::iterator it= mp.begin(); it != mp.end(); ++it) 
    {
        if (it->first == 5) continue;
        OutFile << IntToName[it->first] << " --> " << it->second << " --> " << it->second / (0.01*sum) << "%" << endl;
    }
}

/*This function is used to calculate the number
of different types of instructions.*/
VOID type_docount(string* insStr_ptr)
{
    string insStr = *insStr_ptr;
    if(InsToNum.count(insStr)>0) 
    {
        InsToNum[insStr]++;
    } 
    else 
    {
        InsToNum[insStr] = 1;
    }  
}

/*This function is used to calculate the number of
immediate addressing operands.*/
VOID immediate_docount() 
{
    if(EncodeMap.count((UINT64)0)>0) 
    {
        EncodeMap[(UINT64)0]++;
    } 
    else 
    {
        EncodeMap[(UINT64)0] = 1;
    }    
}

/*This function is used to calculate the number of
register addressing operands.*/
VOID register_docount() 
{
    if(EncodeMap.count((UINT64)1)>0) 
    {
        EncodeMap[(UINT64)1]++;
    } 
    else 
    {
        EncodeMap[(UINT64)1] = 1;
    }    
}

/*This function is uesd to count the number of different addressing operands.*/
VOID addressing_docount(UINT64 * symbol_ptr)
{
    UINT64 symbol = *symbol_ptr;
    if(symbol != 0) {
        if(EncodeMap.count(symbol)>0) 
        {
            EncodeMap[symbol]++;
        } 
        else 
        {
            EncodeMap[symbol] = 1;
        }               
    }
}

/*This function is used to count numbers of different 
length of immediate bit number.*/
VOID immbit_docount(UINT32 *bitcount_ptr)
{
    UINT32 bitcount = *bitcount_ptr;
    if(ImmbitToNum.count(bitcount)>0) 
    {
        ImmbitToNum[bitcount]++;
    } 
    else 
    {
        ImmbitToNum[bitcount] = 1;
    }   
}

static UINT32 GetBitNum(ADDRINT unsigned_value, long signed_value, bool is_signed, INT32 length_bits)
{
    UINT32 bitcount = 0;
    //Calculate the times of right move until the value is 0.
    while(unsigned_value != 0) {
        unsigned_value = unsigned_value >> 1;
        bitcount++;
    }  
    return bitcount;
}

VOID GetLenAndSigned(INS ins, INT32 i, INT32& length_bits, bool& is_signed)
{
    xed_decoded_inst_t* xedd = INS_XedDec(ins);
    length_bits = 8*xed_decoded_inst_operand_length(xedd, i);
    is_signed = xed_decoded_inst_get_immediate_is_signed(xedd);
}

VOID Instruction(INS ins, VOID *v)
{   

    // It's used to see what the ins it is.
    // OutFile << INS_Disassemble(ins) << endl;
    
    _INS *_instruction = new _INS;

    /*******Part1: Calculate the number of different instructions.******/
    //Get the name of the instruction.
    _instruction->ins_name = INS_Mnemonic(ins);
    //Insert the function type_docount
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)type_docount, IARG_PTR, &(_instruction->ins_name), IARG_END);
    /*****************************************************************/

    UINT32 n = INS_OperandCount(ins);
    for(UINT32 i=0; i<n; i++) 
    {
        //Calculate the number of immediate addressing
        //Encode: 0000
        if(INS_OperandIsImmediate(ins, i)) 
        {
            if(INS_OperandRead(ins, i)) 
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)immediate_docount, IARG_END);
            }
        }
        //Calculate the number of register addressing
        //Encode: 0001
        if(INS_OperandIsReg(ins, i)) 
        {
            if(INS_OperandRead(ins, i)) 
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)register_docount, IARG_END);
            }
        }
    } 

    /*******Part2: Calculate the number of different lengths of immdiate number.******/
    /*******Part3: Calculate the number of different ways of addressing.*******/
    for(UINT32 i=0; i<n; i++) 
    {
        UINT64 encode = 0;
        UINT32 cnt = 0;
        //Judge the immediate operand and calculate the number of its bits.
        if(INS_OperandIsImmediate(ins, i)) 
        {
            // Get the value of the immediate.
            ADDRINT value = INS_OperandImmediate(ins, i);
            long immediate_value = (long)value;

            INT32 length_bits = -1;
            bool is_signed = false;

            GetLenAndSigned(ins, i, length_bits, is_signed);
            cnt = GetBitNum(value, immediate_value, is_signed, length_bits);

            _instruction->bit_cnt = cnt;
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)immbit_docount, IARG_PTR, &(_instruction->bit_cnt), IARG_END);
        }

        //Judge insturction's way of addressing, and count it.
        if(INS_OperandIsMemory(ins, i)) 
        {
            /*Here used encode as a symbol to jude the instrucition whether include 
            basement register, index register, displacement and scale.
            For example:
            ------------------------------------------------
            0000: Immediate Addressing
            0001: Register Addressing
            0010: Direct Addressing
            0111: Scale Factor Indexed Addressing
            1000: Register Indirect Addressing
            1010: Register Relative Addressing
            1100: Based Indexed Addressing
            1101: Based Scaled Indexed Addressing
            1110: Relative Based Indexed Addressing
            1111: Relative Based Scaled Indexed Addressing
            ------------------------------------------------
            */
            if(INS_OperandMemoryBaseReg(ins, i) != REG_INVALID()) 
            {
                encode += 8;
            } 
            if(INS_OperandMemoryIndexReg(ins, i) != REG_INVALID()) 
            {
                encode += 4;
            }
            if(INS_OperandMemoryDisplacement(ins, i) != 0) 
            {
                encode += 2;
            } 
            if(INS_OperandMemoryScale(ins, i) != 1) 
            {
                encode += 1;
            } 
            _instruction->symbol = encode;

            if(INS_OperandRead(ins, i)) 
            {
                //Insert the function to calculate the addressing.
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)addressing_docount, IARG_PTR, &(_instruction->symbol), IARG_END);
            }
        }
    }
    /******************************************************************/
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "Instruction_Analysis.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);
    
    //Part1
    part1_output(InsToNum);

    //Part2
    part2_output(ImmbitToNum);

    //Part3
    part3_output(EncodeMap);

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