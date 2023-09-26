/*
CS-UY 2214
Jeff Epstein
Starter code for E20 simulator
sim.cpp
*/

#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8; 
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

/*
    isoverflow helps check if 16-bit number is overflowed.
    If it is, the function will decrease the input by
    MEM_SIZE in the while loop till it is in the valid range.

    @param input the 16-bit binary number stored in uint16_t
*/
uint16_t isoverflow(uint16_t input){
    while(!(input < MEM_SIZE)){
        input -= MEM_SIZE;
    }
    return input;
}

/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;
//uint16_t
    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl; 
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // TODO: your code here. Load f and parse using load_machine_code
    uint16_t mem[MEM_SIZE] = {0};
    load_machine_code(f, mem);

    // TODO: your code here. Do simulation.
    uint16_t regs[NUM_REGS] = {0};
    uint16_t pc = 0;
    bool goahead = true;
    while(goahead){
        uint16_t num = mem[pc];
        uint16_t pc_next = pc + 1;
        uint16_t msb_mask = 0b1110000000000000;
        uint16_t opcode = (num & msb_mask) >> 13;
        if(opcode == 0){
            uint16_t imm = (num & 0b0000000000001111);
            uint16_t regA = (num & 0b0001110000000000) >> 10;
            uint16_t regB = (num & 0b0000001110000000) >> 7;
            uint16_t dst = (num & 0b0000000001110000) >> 4;
            if(imm == 0){ // opcode = add
                regs[dst] = (dst == 0 ? 0 : regs[regA] + regs[regB]);
            }
            else if(imm == 1){ //opcode = sub
                regs[dst] = (dst == 0 ? 0 : regs[regA] - regs[regB]);
            }
            else if(imm == 2){ //opcode = or
                regs[dst] = (dst == 0 ? 0 : regs[regA] | regs[regB]);
            }
            else if(imm == 3){ //opcode = and
                regs[dst] = (dst == 0 ? 0 : regs[regA] & regs[regB]);
            }
            else if (imm  == 4){ //opcode = slt
                int16_t regA4 = static_cast<int16_t>(regs[regA]);
                int16_t regB4 = static_cast<int16_t>(regs[regB]);
                regs[dst] = dst == 0 ? 0 :(regA4 < regB4 ? 1 : 0);
            }
            else if(imm == 8){ //opcode = jr
                pc_next = isoverflow(regs[regA]);
            }
        }
        else if(opcode == 7){ //opcode = slti
            uint16_t imm = (num & 0b0000000001111111);
            uint16_t regSrc = (num & 0b0001110000000000) >> 10;
            uint16_t regDst = (num & 0b0000001110000000) >> 7;
            int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
            int16_t regSrcs = static_cast<int16_t>(regs[regSrc]);
            regs[regDst] = regDst == 0 ? 0 :(regSrcs < sign_extended ? 1 : 0);
        }
        else if(opcode == 4){ //opcode = lw
            uint16_t imm = (num & 0b0000000001111111);
            int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
            uint16_t Add = (num & 0b0001110000000000) >> 10;
            uint16_t regDst = (num & 0b0000001110000000) >> 7;
            regs[regDst] = (regDst == 0)? 0 : mem[isoverflow(regs[Add] + sign_extended)];
        }
        else if(opcode == 5){ //opcode = sw
            uint16_t src = (num & 0b0000001110000000)>>7;
            uint16_t Add = (num & 0b0001110000000000) >> 10;
            uint16_t imm = (num & 0b0000000001111111);
            int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
            mem[(isoverflow(sign_extended + regs[Add]))] = regs[src];
        }
        else if(opcode == 1){ // opcode = addi
            uint16_t src = (num & 0b0001110000000000) >> 10;
            uint16_t dst = (num & 0b0000001110000000) >> 7;
            uint16_t imm = (num & 0b0000000001111111);
            int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
            regs[dst] = dst == 0 ? 0 : regs[src] + sign_extended;
        }
        else if (opcode == 2){ // opcode = j
            uint16_t address = isoverflow(num & 0b0001111111111111);
            pc_next = (address & 0b0001111111111111);
        }
        else if (opcode == 3) { //opcode = jal
            regs[7] = pc_next;
            pc_next = isoverflow(num & 0b0001111111111111);
        }
        else if (opcode == 6) { //opcode = jeq
            uint16_t regA = (num & 0b0001110000000000) >> 10;
            uint16_t regB = (num & 0b0000001110000000) >> 7;
            if(regs[regA] == regs[regB]){
                uint16_t rel_imm = (num & 0b0000000001111111);
                int16_t sign_extended = (rel_imm & 0x40) ? (rel_imm | 0xFF80) : rel_imm;
                pc_next += sign_extended;
            }
        }

        if(pc_next == pc){
            goahead = false;
        }
        else {
            pc = isoverflow(pc_next);
        }
    }
    // TODO: your code here. print the final state of the simulator before ending, using print_state
    print_state(pc, regs, mem, 128);

    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9