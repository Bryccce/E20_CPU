/*
CS-UY 2214
Qiyuan Yin
Starter code for E20 assembler
asm.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
#include <map>
#include <algorithm>

using namespace std;

/*Check if this line is a label or not based on the colon sign*/
bool islabel(const string& line){
    return line.find(':') != string::npos;
}

/*Check if this line is a "fill" instruction or not.*/
bool hasfill(const string& line){
    string fill = ".fill";
    return line.find(fill) != string::npos;
}

/*Check if the parameter imm is a label or not.
The function will go through the map labels. If there is a same label name stored, 
it will return the corrresponding value. If there is no same label name, the program will
convert characters in imm to integer and check its sign.*/
unsigned checklabel(const string& imm, map<string, unsigned>& labels){
    for(map<string,unsigned>::iterator pair = labels.begin(); pair != labels.end(); ++pair){
        if(pair->first == imm){
            return pair->second;
        }
    }
    int temp = stoi(imm);
    if(temp >= 0){
        return (unsigned)temp;
    }
        return (unsigned) 127 + temp + 1;
        /*After this modification, it would be converted into 7-bit signed binary correctly*/
}


/**
    print_line(address, num)
    Print a line of machine code in the required format.
    Parameters:
        address = RAM address of the instructions
        num = numeric value of machine instruction 
    */
void print_machine_code(unsigned address, unsigned num) {
    bitset<16> instruction_in_binary(num);
    cout << "ram[" << address << "] = 16'b" << instruction_in_binary <<";"<<endl;
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
        cerr << "Assemble E20 files into machine code" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing assembly language, typically with .s suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    /* iterate through the line in the file, construct a list
       of numeric values representing machine code */
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }

    /* our final output is a list of ints values representing
       machine code instructions */
    vector<unsigned> instructions;
    string line;

    unsigned machine;
    unsigned location = 0;
    vector<string> lines;
    map <string, unsigned> labels; //map to store detected labels
    map <string, unsigned> assembly; //map to store all opcode with corresponding machine language
    assembly.insert(pair <string, unsigned> ("add", 0));
    assembly.insert(pair <string, unsigned> ("sub", 0));
    assembly.insert(pair <string, unsigned> ("or", 0));
    assembly.insert(pair <string, unsigned> ("and", 0));
    assembly.insert(pair <string, unsigned> ("slt", 0));
    assembly.insert(pair <string, unsigned> ("jr", 0));
    assembly.insert(pair <string, unsigned> ("slti", 7));
    assembly.insert(pair <string, unsigned> ("lw", 4));
    assembly.insert(pair <string, unsigned> ("sw", 5));
    assembly.insert(pair <string, unsigned> ("jeq", 6));
    assembly.insert(pair <string, unsigned> ("addi", 1));
    assembly.insert(pair <string, unsigned> ("movi", 1));
    assembly.insert(pair <string, unsigned> ("j", 2));
    assembly.insert(pair <string, unsigned> ("jal", 3));

    while (getline(f, line)) {
        size_t pos = line.find("#");
        if (pos != string::npos)
            line = line.substr(0, pos);

        lines.push_back(line);
        /*The while-loop can help store all labels pointing to the same address.
        For example, label1: label2: label3: halt*/
        while(islabel(line)){
            string label = line.substr(0, line.find(':'));
            size_t firstChar = label.find_first_not_of(" ");
            label.erase(0, firstChar);
            labels.insert(pair <string, unsigned> (label, location));
            size_t position = line.find(':');
            line = line.substr(position + 1);
        }
        //After erasing labels and comments, it is the instruction if there is something left.
        if(!line.empty())
            location++;
    }

    location = 0; //remake location
    for(size_t i = 0; i < lines.size(); i++){
        line = lines[i];
        while(islabel(line)){
            size_t position = line.find(':');
            line = line.substr(position + 1);
        }
        size_t pos2 = line.find_first_not_of(" \t\n\r\f\v");
        if (pos2 != string::npos) {
            line = line.substr(pos2);
            size_t endpos = line.find_last_not_of(" \t\n");
            if (endpos != std::string::npos) {
                line.erase(endpos+1);
            }
            /*halt instruction*/
            if(line == "halt"){
                machine = (2<<13) | location;
            }
            /*instore .fill instructions*/
            else if (hasfill(line)){
                machine = stoi(line.substr(line.find('.') + 6, line.back()));
            }
            /*convert instructions*/
            else{
                string opcode = line.substr(0, line.find(' '));
                for(map<string,unsigned>::iterator pair = assembly.begin(); pair != assembly.end(); ++pair){
                    if(pair->first == opcode){
                        machine = pair->second << 13;
                        if(opcode == "sub"){
                            machine = machine | 1;
                        }
                        else if(opcode == "or"){
                            machine = machine | 2;
                        }
                        else if(opcode == "and"){
                            machine = machine | 3;
                        }
                        else if(opcode == "slt"){
                            machine = machine | 4;
                        }
                        else if(opcode == "jr"){
                            machine = machine | 8;
                            unsigned reg = (unsigned)line[line.find('$') + 1] - 48;
                            /*convert character into unsigned num based on ascii table*/
                            machine = machine | (reg << 10);
                        }
                        if(opcode == "add" || opcode == "sub" || opcode == "or" || opcode == "and" || opcode == "slt"){
                            size_t firstDollarPos = line.find_first_of("$");
                            size_t secondDollarPos = line.find_first_of("$", firstDollarPos + 1);
                            size_t thirdDollarPos = line.find_first_of("$", secondDollarPos + 1);
                            unsigned dst = (unsigned)line[firstDollarPos + 1] - 48;
                            unsigned srcA = (unsigned)line[secondDollarPos + 1] - 48;
                            unsigned srcB = (unsigned)line[thirdDollarPos + 1] - 48;
                            machine = machine | (dst << 4) | (srcA << 10) | (srcB << 7);
                        }
                    }

                }
                if(opcode == "slti" || opcode == "addi"){
                    size_t firstDollarPos = line.find_first_of("$");
                    size_t secondDollarPos = line.find_first_of("$", firstDollarPos + 1);
                    unsigned dst = (unsigned)line[firstDollarPos + 1] - 48;
                    unsigned src = (unsigned)line[secondDollarPos + 1] - 48;
                    machine = machine | (dst << 7) | (src << 10);
                    size_t comma = line.find_last_of(",");
                    string imm = line.substr(comma + 1); // locate immediate value
                    size_t firstCharPos = imm.find_first_not_of(" ");
                    imm.erase(0, firstCharPos);
                    machine = machine | checklabel(imm, labels);
                }
                if(opcode == "movi"){
                    unsigned dst = (unsigned)line[line.find('$') + 1] - 48;
                    unsigned src = 0;
                    size_t comma = line.find_last_of(",");
                    string imm = line.substr(comma + 1);
                    size_t firstCharPos = imm.find_first_not_of(" ");
                    imm.erase(0, firstCharPos);
                    machine = machine | (dst << 7) | (src << 10) | checklabel(imm, labels);
                }
                if(opcode == "lw" || opcode == "sw"){
                    unsigned src = (unsigned)line[line.find('$') + 1] - 48;
                    unsigned add = (unsigned)line[line.find('(') + 2] - 48;
                    size_t comma = line.find_last_of(",");
                    string imm = line.substr(comma + 1);
                    size_t firstCharPos = imm.find_first_not_of(" ");
                    size_t paren = imm.find_last_of("(");
                    imm.erase(paren);
                    imm.erase(0, firstCharPos);
                    machine = machine | (src << 7) | (add << 10) | checklabel(imm, labels);
                }
                if(opcode == "j" || opcode == "jal"){
                    size_t start = line.find(" ")+1;
                    size_t end = line.find(" ", start);
                    string imm = line.substr(start, end - start);
                    machine = machine | checklabel(imm, labels);
                }
                if(opcode == "jeq"){
                    size_t firstDollarPos = line.find_first_of("$");
                    size_t secondDollarPos = line.find_first_of("$", firstDollarPos + 1);
                    unsigned regA = (unsigned)line[firstDollarPos + 1] - 48;
                    unsigned regB = (unsigned)line[secondDollarPos + 1] - 48;
                    size_t comma = line.find_last_of(",");
                    string imm = line.substr(comma + 1);
                    size_t firstCharPos = imm.find_first_not_of(" ");
                    imm.erase(0, firstCharPos);
                    int temp_rel = checklabel(imm, labels) - location - 1;
                    /*Get the relative value in int*/
                    unsigned rel_imm;
                    if(temp_rel >= 0)
                        rel_imm =  (unsigned)temp_rel;
                        //If it is not negative, we can use it directly.
                    else
                        rel_imm = (unsigned) 127 + temp_rel + 1;
                        // If it is negaive, we should change it to make its signed 7-bit binary correctly.
                    machine = machine | (regA << 10) | (regB << 7) | rel_imm;
                }
            }
            instructions.push_back(machine);   // TODO change this. generate the machine code
            machine = 0;
            location ++;
            
        }

    }

    /* print out each instruction in the required format */
    unsigned address = 0;
    for (unsigned instruction : instructions) {
        print_machine_code(address, instruction); 
        address ++;
    }
 
    return 0;
}

//ra0Eequ6ucie6Jei0koh6phishohm9