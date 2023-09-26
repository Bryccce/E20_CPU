/*
CS-UY 2214
Jeff Epstein
Starter code for E20 cache assembler
simcache.cpp
*/

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip>
#include <regex>
#include <map>
#include <list>
using namespace std;

class LRUcache{
public:
    list<int> m_list;
    map<int, vector<uint16_t>> block; 
};

size_t const static NUM_REGS = 8; 
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;
uint16_t isoverflow(uint16_t input){
    while(!(input < MEM_SIZE)){
        input -= MEM_SIZE;
    }
    return input;
}

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
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])

    @param num_rows The number of rows in the given cache.
*/
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
        ", associativity " << assoc << ", blocksize " << blocksize <<
        ", rows " << num_rows << endl;
}

/*
    Prints out a correctly-formatted log entry.

    @param cache_name The name of the cache where the event
        occurred. "L1" or "L2"

    @param status The kind of cache event. "SW", "HIT", or
        "MISS"

    @param pc The program counter of the memory
        access instruction

    @param addr The memory address being accessed.

    @param row The cache row or set number where the data
        is stored.
*/
void print_log_entry(const string &cache_name, const string &status, int pc, int addr, int row) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
        " pc:" << setw(5) << pc <<
        "\taddr:" << setw(5) << addr <<
        "\trow:" << setw(4) << row << endl;
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
    string cache_config;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
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
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl; 
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,associativity,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,associativity,blocksize,size,associativity,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }
    /* parse cache config */
    if (cache_config.size() > 0) {
        vector<int> parts;
        size_t pos;
        size_t lastpos = 0;
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int num_rows = L1size / L1assoc / L1blocksize;
            print_cache_config("L1", L1size, L1assoc, L1blocksize, num_rows);
            vector<LRUcache> cache;
            for(size_t i = 0; i < num_rows; i++){
                LRUcache row;
                cache.push_back(row);
            }

            // TODO: execute E20 program and simulate one cache here
            ifstream f(filename);
            if (!f.is_open()) {
                cerr << "Can't open file "<<filename<<endl;
                return 1;
            }
            uint16_t mem[MEM_SIZE] = {0};
            load_machine_code(f, mem);
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
                        uint16_t regsrcA = regs[regA];
                        uint16_t regsrcB = regs[regB];
                        regs[dst] = dst == 0 ? 0 :(regsrcA < regsrcB ? 1 : 0);
                    }
                    else if(imm == 8){ //opcode = jr
                        pc_next = isoverflow(regs[regA]);
                    }
                }
                else if(opcode == 7){ //opcode = slti
                    uint16_t imm = (num & 0b0000000001111111);
                    uint16_t regSrc = (num & 0b0001110000000000) >> 10;
                    uint16_t regDst = (num & 0b0000001110000000) >> 7;
                    uint16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    uint16_t regsrc = regs[regSrc];
                    regs[regDst] = regDst == 0 ? 0 :(regsrc < sign_extended ? 1 : 0);
                }
                else if(opcode == 4){ //opcode = lw
                    uint16_t imm = (num & 0b0000000001111111);
                    int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    uint16_t Add = (num & 0b0001110000000000) >> 10;
                    uint16_t regDst = (num & 0b0000001110000000) >> 7;
                    // regs[regDst] = (regDst == 0)? 0 : mem[isoverflow(regs[Add] + sign_extended)];
                    int addr = isoverflow(regs[Add] + sign_extended);
                    string status = "";
                    int blockid = addr / L1blocksize;
                    int row_num = blockid % num_rows;
                    int tag = blockid / num_rows;
                    if(cache[row_num].block.find(tag) != cache[row_num].block.end()){
                        status = "HIT";
                        // find the iterator pointing to the value in the list
                        auto it = find(cache[row_num].m_list.begin(), cache[row_num].m_list.end(), tag); 
                        // put it to the front of the list recording implementation of this row
                        cache[row_num].m_list.splice(cache[row_num].m_list.begin(), cache[row_num].m_list, it);
                    }
                    else{
                        status = "MISS";
                        if(cache[row_num].block.size() == L1assoc){
                            int replaced_tag = cache[row_num].m_list.back();
                            cache[row_num].block.erase(replaced_tag);
                            cache[row_num].m_list.pop_back();

                        }
                        vector<uint16_t> load;
                        size_t start = blockid * L1blocksize;
                        size_t end = blockid * L1blocksize + L1blocksize;
                        for(size_t i = start; i < end; i++){
                            load.push_back(mem[i]);
                        }
                        cache[row_num].block.insert({tag, load});
                        cache[row_num].m_list.push_front(tag);
                    }
                    int offset = addr & (L1blocksize - 1);
                    regs[regDst] = (regDst == 0)? 0 : cache[row_num].block.at(tag)[offset];
                    print_log_entry("L1", status, pc, addr, row_num);      
                }
                else if(opcode == 5){ //opcode = sw
                    uint16_t src = (num & 0b0000001110000000)>>7;
                    uint16_t Add = (num & 0b0001110000000000) >> 10;
                    uint16_t imm = (num & 0b0000000001111111);
                    int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    // mem[(isoverflow(sign_extended + regs[Add]))] = regs[src];

                    string status = "SW";
                    int addr = isoverflow(sign_extended + regs[Add]);
                    mem[addr] = regs[src];
                    int blockid = addr / L1blocksize;
                    int row_num = blockid % num_rows;
                    int tag = blockid / num_rows;
                    if(cache[row_num].block.find(tag) != cache[row_num].block.end()){
                        int offset = addr & (L1blocksize - 1);
                        cache[row_num].block.at(tag)[offset] = mem[addr];
                        auto it = find(cache[row_num].m_list.begin(), cache[row_num].m_list.end(), tag); 
                        cache[row_num].m_list.splice(cache[row_num].m_list.begin(), cache[row_num].m_list, it);
                    }
                    else{
                        if(cache[row_num].block.size() == L1assoc){
                            int replaced_tag = cache[row_num].m_list.back();
                            cache[row_num].block.erase(replaced_tag);
                            cache[row_num].m_list.pop_back();
                        }
                        vector<uint16_t> load;
                        size_t start = blockid * L1blocksize;
                        size_t end = blockid * L1blocksize + L1blocksize;
                        for(size_t i = start; i < end; i++){
                            load.push_back(mem[i]);
                        }
                        cache[row_num].block.insert({tag, load});
                        cache[row_num].m_list.push_front(tag);
                    }
                    print_log_entry("L1", status, pc, addr, row_num);
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

        // L1 and L2 caches
        } else if (parts.size() == 6) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int L2size = parts[3];
            int L2assoc = parts[4];
            int L2blocksize = parts[5];
            int num_rows_1 = L1size / L1assoc / L1blocksize;
            int num_rows_2 = L2size / L2assoc / L2blocksize;
            print_cache_config("L1", L1size, L1assoc, L1blocksize, num_rows_1);
            print_cache_config("L2", L2size, L2assoc, L2blocksize, num_rows_2);
            vector<LRUcache> cache1;
            vector<LRUcache> cache2;
            for(size_t i = 0; i < num_rows_1; i++){
                LRUcache row;
                cache1.push_back(row);
            }
            for(size_t i = 0; i < num_rows_2; i++){
                LRUcache row;
                cache2.push_back(row);
            }

            // TODO: execute E20 program and simulate two caches here
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
                        uint16_t regsrcA = regs[regA];
                        uint16_t regsrcB = regs[regB];
                        regs[dst] = dst == 0 ? 0 :(regsrcA < regsrcB ? 1 : 0);
                    }
                    else if(imm == 8){ //opcode = jr
                        pc_next = isoverflow(regs[regA]);
                    }
                }
                else if(opcode == 7){ //opcode = slti
                    uint16_t imm = (num & 0b0000000001111111);
                    uint16_t regSrc = (num & 0b0001110000000000) >> 10;
                    uint16_t regDst = (num & 0b0000001110000000) >> 7;
                    uint16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    uint16_t regsrc = regs[regSrc];
                    regs[regDst] = regDst == 0 ? 0 :(regsrc < sign_extended ? 1 : 0);
                }
                else if(opcode == 4){ //opcode = lw
                    uint16_t imm = (num & 0b0000000001111111);
                    int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    uint16_t Add = (num & 0b0001110000000000) >> 10;
                    uint16_t regDst = (num & 0b0000001110000000) >> 7;
                    // regs[regDst] = (regDst == 0)? 0 : mem[isoverflow(regs[Add] + sign_extended)];
                    int addr = isoverflow(regs[Add] + sign_extended);
                    int blockid_1 = addr / L1blocksize;
                    int row_num_1 = blockid_1 % num_rows_1;
                    int tag_1 = blockid_1 / num_rows_1;
                    string status_1 = "";
                    string status_2 = "";
                    // L1 HIT
                    if(cache1[row_num_1].block.find(tag_1) != cache1[row_num_1].block.end()){
                        status_1 = "HIT";
                        // find the iterator pointing to the value in the list
                        auto it = find(cache1[row_num_1].m_list.begin(), cache1[row_num_1].m_list.end(), tag_1); 
                        // put it to the front of the list recording implementation of this row
                        cache1[row_num_1].m_list.splice(cache1[row_num_1].m_list.begin(), cache1[row_num_1].m_list, it);
                        print_log_entry("L1", status_1, pc, addr, row_num_1);
                    }
                    else{
                        status_1 = "MISS";
                        int blockid_2 = addr / L2blocksize;
                        int row_num_2 = blockid_2 % num_rows_2;
                        int tag_2 = blockid_2 / num_rows_2;
                        // find in L2 cache
                        if(cache2[row_num_2].block.find(tag_2) != cache2[row_num_2].block.end()){
                            status_2 = "HIT";

                        }
                        // L1 & L2 both miss
                        else{
                            status_2 = "MISS";
                            if(cache2[row_num_2].block.size() == L2assoc){
                                int replaced_tag_2 = cache2[row_num_2].m_list.back();
                                cache2[row_num_2].block.erase(replaced_tag_2);
                                cache2[row_num_2].m_list.pop_back();
                            }
                            vector<uint16_t> load_2;
                            size_t start_2 = blockid_2 * L2blocksize;
                            size_t end_2 = blockid_2 * L2blocksize + L2blocksize;
                            for(size_t i = start_2; i < end_2; i++){
                                load_2.push_back(mem[i]);
                            }
                            cache2[row_num_2].block.insert({tag_2, load_2});
                            cache2[row_num_2].m_list.push_front(tag_2);
                        }
                        // store in L1
                        if(cache1[row_num_1].block.size() == L1assoc){
                            int replaced_tag_1 = cache1[row_num_1].m_list.back();
                            cache1[row_num_1].block.erase(replaced_tag_1);
                            cache1[row_num_1].m_list.pop_back();
                        }
                        vector<uint16_t> load_1;
                        size_t start = blockid_1 * L1blocksize;
                        size_t end = blockid_1 * L1blocksize + L1blocksize;
                        for(size_t i = start; i < end; i++){
                            load_1.push_back(mem[i]);
                        }
                        cache1[row_num_1].block.insert({tag_1, load_1});
                        cache1[row_num_1].m_list.push_front(tag_1);
                        print_log_entry("L1", status_1, pc, addr, row_num_1);
                        print_log_entry("L2", status_2, pc, addr, row_num_2);
                    }
                    int offset = addr & (L1blocksize - 1);
                    regs[regDst] = (regDst == 0)? 0 : cache1[row_num_1].block.at(tag_1)[offset];
                }
                else if(opcode == 5){ //opcode = sw
                    uint16_t src = (num & 0b0000001110000000)>>7;
                    uint16_t Add = (num & 0b0001110000000000) >> 10;
                    uint16_t imm = (num & 0b0000000001111111);
                    int16_t sign_extended = (imm & 0x40) ? (imm | 0xFF80) : imm;
                    // mem[(isoverflow(sign_extended + regs[Add]))] = regs[src];

                    int addr = isoverflow(sign_extended + regs[Add]);
                    mem[addr] = regs[src];
                    string status_1 = "SW";
                    string status_2 = "SW";
                    // modify L1
                    int blockid_1 = addr / L1blocksize;
                    int row_num_1 = blockid_1 % num_rows_1;
                    int tag_1 = blockid_1 / num_rows_1;
                    if(cache1[row_num_1].block.find(tag_1) != cache1[row_num_1].block.end()){
                        int offset = addr & (L1blocksize - 1);
                        cache1[row_num_1].block.at(tag_1)[offset] = mem[addr];
                        auto it = find(cache1[row_num_1].m_list.begin(), cache1[row_num_1].m_list.end(), tag_1); 
                        cache1[row_num_1].m_list.splice(cache1[row_num_1].m_list.begin(), cache1[row_num_1].m_list, it);
                    }
                    else{
                        if(cache1[row_num_1].block.size() == L1assoc){
                            int replaced_tag = cache1[row_num_1].m_list.back();
                            cache1[row_num_1].block.erase(replaced_tag);
                            cache1[row_num_1].m_list.pop_back();
                        }
                        vector<uint16_t> load1;
                        size_t start = blockid_1 * L1blocksize;
                        size_t end = blockid_1 * L1blocksize + L1blocksize;
                        for(size_t i = start; i < end; i++){
                            load1.push_back(mem[i]);
                        }
                        cache1[row_num_1].block.insert({tag_1, load1});
                        cache1[row_num_1].m_list.push_front(tag_1);
                    }
                    // modify L2
                    int blockid_2 = addr / L2blocksize;
                    int row_num_2 = blockid_2 % num_rows_2;
                    int tag_2 = blockid_2 / num_rows_2;
                    if(cache2[row_num_2].block.find(tag_2) != cache2[row_num_2].block.end()){
                        int offset = addr & (L2blocksize - 1);
                        cache2[row_num_2].block.at(tag_2)[offset] = mem[addr];
                        auto it = find(cache2[row_num_2].m_list.begin(), cache2[row_num_2].m_list.end(), tag_2); 
                        cache2[row_num_2].m_list.splice(cache2[row_num_2].m_list.begin(), cache2[row_num_2].m_list, it);
                    }
                    else{
                        if(cache2[row_num_2].block.size() == L2assoc){
                            int replaced_tag = cache2[row_num_2].m_list.back();
                            cache2[row_num_2].block.erase(replaced_tag);
                            cache2[row_num_2].m_list.pop_back();
                        }
                        vector<uint16_t> load2;
                        size_t start = blockid_2 * L2blocksize;
                        size_t end = blockid_2 * L2blocksize + L2blocksize;
                        for(size_t i = start; i < end; i++){
                            load2.push_back(mem[i]);
                        }
                        cache2[row_num_2].block.insert({tag_2, load2});
                        cache2[row_num_2].m_list.push_front(tag_2);
                    }
                    print_log_entry("L1", status_1, pc, addr, row_num_1);
                    print_log_entry("L2", status_2, pc, addr, row_num_2);


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

        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }
    }
    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9