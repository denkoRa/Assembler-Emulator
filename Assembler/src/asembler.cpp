#include "asembler.h"
#include "parser.h"
#include <string>
#include <stdint.h>
#include <climits>
#include <cstring>

#define PC 16
#define LR 17
#define SP 18
#define PSW 19
#define ADD 0
#define SUB 1
#define MAX_NAME_LEN 100
#define LDCADD 2
#define LDCSUB 3
#define INT16_MAX ((1<<15) - 1)
#define INT32_MAX ((1<<31) - 1)

using namespace std;

int Asembler::secID = 0;

char Asembler::mnemonics[21][5] =
{
    "int", "add", "sub", "mul", "div", "cmp", "and", "or", "not", "test",
    "ldr", "str", "call", "in", "out", "mov", "shr", "shl", "ldch", "ldcl", "ldc"
};

int Asembler::opCodes[20] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 10, 12, 13, 13, 14, 14, 14, 15, 15
};

int Asembler::mnemLength[21] =
{
    3, 3, 3, 3, 3, 3, 3, 2, 3, 4,
    3, 3, 4, 2, 3, 3, 3, 3, 4, 4, 3
};

char Asembler::conditions[8][3] =
{
    "eq", "ne", "gt", "ge", "lt", "le", "NN", "al"
};

Asembler::Asembler(char* input, char* output)
{
    asmInput.open(input);
    asmInput2.open(input);
    objOutput.open(output, ofstream::binary | ofstream::out);
    curSection = NULL;
    symTab = new SymbolTable();
    locationCounter = 0;

    endOfFirstPass = 0;
    endOfAssembling = 0;
    endOfSecondPass = 0;

}

Asembler::~Asembler()
{

    /*delete symTab;
    delete symTabForLinker;
    for (int i = 0; i < sections.size(); i++) delete relTab[i];
    delete relTab;*/

}

void Asembler::doAssembling()
{

    firstPass();

    secondPass();

    shrinkSymTable();

    generateELF();

}

void Asembler::shrinkSymTable()
{
    symTabForLinker = new SymbolTable();
    int cnt = 1;
    for (int i = 1; i < symTab->table.size(); i++)
        if (symTab->table[i].global || isSection(i))
        {
            if (symTab->table[i].defined == 0 && !symTab->table[i].ext) throw UndefinedSymbolException(symTab->table[i].name);
            symTabForLinker->table.push_back(symTab->table[i]);
            symTab->table[i].idxInLinkingTable = cnt++;
        }
    vector<Section*> textSections = getSections(".text");
    vector<Section*> dataSections = getSections(".data");

    for (int i = 0; i < textSections.size(); i++)
        for (int j = 0; j < relTab[textSections[i]->id]->table.size(); j++)
            if (symTab->table[relTab[textSections[i]->id]->table[j].indexInSymTab].global != 0 || isSection(relTab[textSections[i]->id]->table[j].indexInSymTab))
            {
                for (int k = 1; k < symTabForLinker->table.size(); k++)
                    if (strcmp(symTabForLinker->table[k].name, symTab->table[relTab[textSections[i]->id]->table[j].indexInSymTab].name) == 0)
                        relTab[textSections[i]->id]->table[j].indexInSymTab = k;
            }

    for (int i = 0; i < dataSections.size(); i++)
        for (int j = 0; j < relTab[dataSections[i]->id]->table.size(); j++)
            if (symTab->table[relTab[dataSections[i]->id]->table[j].indexInSymTab].global != 0 || isSection(relTab[dataSections[i]->id]->table[j].indexInSymTab))
            {
                for (int k = 1; k < symTabForLinker->table.size(); k++)
                    if (strcmp(symTabForLinker->table[k].name, symTab->table[relTab[dataSections[i]->id]->table[j].indexInSymTab].name) == 0)
                        relTab[dataSections[i]->id]->table[j].indexInSymTab = k;
            }

    for (int i = 0; i < symTab->table.size(); i++)
    {
        if (symTab->table[i].global || isSection(i))
        {
            char *secName = symTab->table[symTab->table[i].sectionNum].name;
            for (int j = 1; j < symTabForLinker->table.size(); j++)
                if (strcmp(secName, symTabForLinker->table[j].name) == 0)
                    symTabForLinker->table[symTab->table[i].idxInLinkingTable].sectionNum = j;
        }
    }
    /*
    cout << "Tabela simbola:" << endl;
    for (int i = 0; i < symTabForLinker->table.size(); i++)
    {
        cout << i << ": " << symTabForLinker->table[i].name << ", section: " << symTabForLinker->table[i].sectionNum << ", global: " << symTabForLinker->table[i].global << ", value: " << symTabForLinker->table[i].value << endl;
    }
    cout << "--------------------------" << endl;
    for (int i = 0; i < sections.size(); i++)
    {
        cout << "Tabela relokacija za sekciju " << i << endl;
        for (int j = 0; j < relTab[i]->table.size(); j++)
            cout << relTab[i]->table[j].offset << " " << relTab[i]->table[j].indexInSymTab << " " << relTab[i]->table[j].addOrSub << endl;
        cout << "----------------------------------" << endl;
    }
    */
}

bool Asembler::isSection(int i)
{
    return (symTab->table[i].sectionNum == i);
}

void Asembler::generateELF()
{
    objOutput << char('E') << char ('L') << char('F');
    vector<Section*> textSections = getSections(".text");
    vector<Section*> dataSections = getSections(".data");
    vector<Section*> bssSections = getSections(".bss");

    byteByByte(textSections.size());
    byteByByte(dataSections.size());
    byteByByte(bssSections.size());
    byteByByte(symTabForLinker->table.size() - 1);

    for (int i = 0; i < textSections.size(); i++)
    {
        byteByByte(symTabForLinker->getSymbol(textSections[i]->fullname));
        byteByByte(textSections[i]->size);
        byteByByte(relTab[textSections[i]->id]->table.size());
    }
    for (int i = 0; i < dataSections.size(); i++)
    {
        byteByByte(symTabForLinker->getSymbol(dataSections[i]->fullname));
        byteByByte(dataSections[i]->size);
        byteByByte(relTab[dataSections[i]->id]->table.size());
    }

    for (int i = 0; i < bssSections.size(); i++)
    {
        byteByByte(symTabForLinker->getSymbol(bssSections[i]->fullname));
        byteByByte(bssSections[i]->size);
    }

    for (int i = 0; i < textSections.size(); i++)
    {
        for (int j = 0; j < textSections[i]->size; j++)
            objOutput << textSections[i]->content[j];
    }

    for (int i = 0; i < dataSections.size(); i++)
    {
        for (int j = 0; j < dataSections[i]->size; j++)
            objOutput << dataSections[i]->content[j];
    }

    for (int i = 0; i < textSections.size(); i++)
        for (int j = 0; j < relTab[textSections[i]->id]->table.size(); j++)
        {
            byteByByte(relTab[textSections[i]->id]->table[j].offset);
            byteByByte(relTab[textSections[i]->id]->table[j].indexInSymTab);
            byteByByte(relTab[textSections[i]->id]->table[j].addOrSub);
        }

    for (int i = 0; i < dataSections.size(); i++)
        for (int j = 0; j < relTab[dataSections[i]->id]->table.size(); j++)
        {
            byteByByte(relTab[dataSections[i]->id]->table[j].offset);
            byteByByte(relTab[dataSections[i]->id]->table[j].indexInSymTab);
            byteByByte(relTab[dataSections[i]->id]->table[j].addOrSub);
        }

    for (int i = 1; i < symTabForLinker->table.size(); i++)
    {
        byteByByte(length(symTabForLinker->table[i].name) + 1);
        objOutput << symTabForLinker->table[i].name << '\0';
        byteByByte(symTabForLinker->table[i].sectionNum);
        byteByByte(symTabForLinker->table[i].global);
        byteByByte(symTabForLinker->table[i].defined);
        byteByByte(symTabForLinker->table[i].value);
    }

}

int Asembler::length(char* name)
{
    int i = 0;
    while (name[i] != NULL) i++;
    return i;
}

void Asembler::byteByByte(int num)
{
    objOutput << (unsigned char)(num & 0x000000FF);
    objOutput << (unsigned char)((num & 0x0000FF00) >> 8);
    objOutput << (unsigned char)((num & 0x00FF0000) >> 16);
    objOutput << (unsigned char)((num & 0xFF000000) >> 24);
}

vector<Section*> Asembler::getSections(char* name)
{
    vector<Section*> ret;
    for (int i = 0; i < sections.size(); i++)
    {
        if (strcmp(sections[i]->name, name) == 0) ret.push_back(sections[i]);
    }
    return ret;
}


void Asembler::firstPass()
{
    char* line = new char[256];
    bool needNewLine = 1;
    while (!endOfFirstPass)
    {
        if (needNewLine) asmInput.getline(line, 256);
        line = Parser::skipWhite(line);

        if (strchr(line, ':'))
        {
            line = processLabel(line);
            line = Parser::skipWhite(line);
        }
        if (*line == NULL || *line == '\r') continue;
        if (*line == '.')
        {
            getDirective(line);
            needNewLine = true;
            continue;
        }
        int increment = Parser::checkForInstruction(line);
        if (increment == -1) throw AssemblerException(line);
        locationCounter += increment;
    }

    relTab = new RelTab*[sections.size()];
    for (int i = 0; i < sections.size(); i++) relTab[i] = new RelTab;



    /*
    cout << "-------------------------------" << endl;
    cout << "End of firt pass!" << endl << "--------------------------" <<  endl;
    cout << "Tabela simbola:" << endl;
    for (int i = 0; i < symTab->table.size(); i++)
    {
        cout << i << ": "<< symTab->table[i].name << ", section: " << symTab->table[i].sectionNum <<  ", global: " << symTab->table[i].global << ", value: " << symTab->table[i].value << endl;
    }
    cout << "--------------------------" << endl << "Tabela sekcija:" << endl;
    for (int i = 0; i < sections.size(); i++) {
    	cout << sections[i]->name << ", size: " << sections[i]->size << " ";
    	if (sections[i]->subname) cout << sections[i]->subname;
    	cout << endl;
    }
    cout << "--------------------------" << endl;
    */

}

void Asembler::secondPass()
{
    locationCounter = 0;
    char* line = new char[256];
    curSection = NULL;
    while (!endOfAssembling)
    {
        asmInput2.getline(line, 256);
        line = Parser::skipWhite(line);
        if (strchr(line, ':'))
        {
            line = Parser::skipLabel(line);
            line++;
            line = Parser::skipWhite(line);
        }
        if (line[0] == NULL || line[0] == '\r') continue;
        if (line[0] == '.')
        {
            decodeDirective(line);
            continue;
        }
        if (curSection == NULL || (strcmp(curSection->name, "bss") == 0 || strcmp(curSection->name, "data") == 0))
            throw AssemblerException(line);
        else decodeInstruction(line);
    }



    /*
    for (int i = 0; i < sections.size(); i++)
    {
        cout << "Content of section " << sections[i]->name << endl;
        for (int j = 0; j < sections[i]->size; j++)
        {

            cout << +(sections[i]->content[j])<< " ";
            //objOutput << sections[i]->content[j];
        }
        cout << endl << "----------------------------------------" << endl;
    }

    cout << endl << "----------------------------------" << endl;
    for (int i = 0; i < sections.size(); i++)
    {
        cout << "Tabela relokacija za sekciju " << i << endl;
        for (int j = 0; j < relTab[i]->table.size(); j++)
            cout << relTab[i]->table[j].offset << " " << relTab[i]->table[j].indexInSymTab << " " << relTab[i]->table[j].addOrSub << endl;
        cout << "----------------------------------" << endl;
    }

    */
}



void Asembler::decodeInstruction(char* line)
{
    char* lineBegin = line;
    line = Parser::skipWhite(line);
    int mnemIndex = Parser::getMnem(line);
    line += mnemLength[mnemIndex];

    /*cout << mnemonics[mnemIndex] << endl;*/

    int condIndex = 7;

    if (isalpha(*line))
    {
        condIndex = Parser::getCond(line);

        if (condIndex != -1) line += 2;
    }

    int enableFlags = 0;
    if (isalpha(*line))
    {
        if (*line == 's')
        {
            enableFlags = 1;
        }
        line++;
        if (enableFlags == 0) throw AssemblerException(lineBegin);
        if (condIndex == -1 && enableFlags == 0) throw AssemblerException(lineBegin);
        if (condIndex == 6) throw AssemblerException(lineBegin);
    }


    line = Parser::skipWhite(line);

    switch(mnemIndex)
    {

    case 0:		/* int */
    {
        line = Parser::skipWhite(line);
        char num = Parser::getNumber(line);
        if (num == -1) throw new AssemblerException(line);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        char src;
        src = Parser::getNumber(line);
        if (src > 15 || src < 0) throw AssemblerException(lineBegin);
        line = Parser::skipDigits(line);
        if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
        curSection->content[locationCounter + 1] |= src << 4;
        break;
    }

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:		/* add/sub/mul/div */
    {
        char dstReg, srcReg;
        int imm;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 19) throw  AssemblerException(lineBegin);
        if (dstReg > 15 && dstReg < 19 && mnemIndex != 1 && mnemIndex != 2) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(line);
        line++;
        line = Parser::skipWhite(line);
        int immBit;
        if (isalpha(*line))  		//get src reg
        {
            immBit = 0;
            srcReg = Parser::getRegistry(line);
            if (srcReg == -1 || srcReg > 19) throw  AssemblerException(lineBegin);
            if (srcReg > 15 && srcReg < 19 && mnemIndex != 1 && mnemIndex != 2) throw  AssemblerException(lineBegin);
            line = Parser::skipRegistry(line);
            line = Parser::skipWhite(line);
            if (*line != NULL && *line != '\r') throw  AssemblerException(lineBegin);
        }
        else if (isdigit(*line))  		//get Immediate
        {
            immBit = 1;
            imm = Parser::getNumber(line);
            int maxImm = 1 << 19 - 1;
            if (imm > maxImm) throw AssemblerException(lineBegin);
            line = Parser::skipDigits(line);
            line = Parser::skipWhite(line);
            if (*line != NULL && *line != '\r') throw  AssemblerException(lineBegin);
        }
        else throw  AssemblerException(lineBegin);

        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        curSection->content[locationCounter + 1] |= dstReg << 3;
        curSection->content[locationCounter + 1] |= immBit << 2;
        if (immBit)
        {
            curSection->content[locationCounter + 1] |= imm >> 16;
            curSection->content[locationCounter + 2] |= (imm >> 8) & 0xFF;
            curSection->content[locationCounter + 3] |= imm & 0xFF;
        }
        else
        {
            curSection->content[locationCounter + 1] |= srcReg >> 3;
            curSection->content[locationCounter + 2] |= srcReg << 5;
        }
        break;
    }

    case 6:
    case 7:
    case 8:
    case 9:		/* and/or/not/test */
    {
        char dstReg, srcReg;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || (dstReg > 15 && dstReg != 18)) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        srcReg = Parser::getRegistry(line);
        if (srcReg == -1 || (srcReg > 15 && srcReg != 18)) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != NULL && *line != '\r') throw  AssemblerException(lineBegin);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        curSection->content[locationCounter + 1] |= dstReg << 3;
        curSection->content[locationCounter + 1] |= srcReg >> 2;
        curSection->content[locationCounter + 2] |= srcReg << 6;
        break;
    }

    case 10:
    case 11:	/* ldr/str */
    {
        char dstReg, addrReg;
        int imm = 0;
        char fOperand = 0;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 19) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        if (*line == '[')
        {
            line++;
            line = Parser::skipWhite(line);
            if (Parser::increment(line))
            {
                line += 2;
                fOperand = 4;
            }
            else if (Parser::decrement(line))
            {
                line += 2;
                fOperand = 5;
            }
            addrReg = Parser::getRegistry(line);
            if (addrReg >= PSW || addrReg < 0) throw AssemblerException(lineBegin);
            line = Parser::skipRegistry(line);
            if (Parser::increment(line))
            {
                if (fOperand != 0) throw AssemblerException(lineBegin);
                fOperand = 2;
                line += 2;
            }
            else if (Parser::decrement(line))
            {
                if (fOperand != 0) throw AssemblerException(lineBegin);
                fOperand = 3;
                line += 2;
            }
            if (addrReg == PC && fOperand != 0) throw AssemblerException(lineBegin);
            line = Parser::skipWhite(line);
            if (*line == ',')
            {
                line = Parser::skipWhite(line + 1);
                if (isdigit(*line))
                {
                    imm = Parser::getNumber(line);
                    line = Parser::skipDigits(line);
                    line = Parser::skipWhite(line);
                    if (*line != ']') throw AssemblerException(lineBegin);
                    line++;
                    line = Parser::skipWhite(line);
                    if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
                }
            }
            else
            {
                line = Parser::skipWhite(line);
                if (*line != ']') throw AssemblerException(lineBegin);
                line = Parser::skipWhite(line + 1);
                if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
            }
            curSection->content[locationCounter + 1] |= addrReg << 3;
            curSection->content[locationCounter + 1] |= dstReg >> 2;
            curSection->content[locationCounter + 2] |= dstReg << 6;
            curSection->content[locationCounter + 2] |= fOperand << 3;
            curSection->content[locationCounter + 2] |= (mnemIndex == 10) << 2;
            curSection->content[locationCounter + 2] |= imm >> 8;
            curSection->content[locationCounter + 3] |= imm;
        }
        else if (isalpha(*line))
        {
            char* labelStart = line;
            char* labelEnd = Parser::getName(line);
            char* labelName = new char[labelEnd - labelStart];
            strncpy(labelName, line, labelEnd - labelStart);
            labelName[labelEnd - labelStart] = NULL;
            line = Parser::skipWhite(labelEnd);
            if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);

            int sym = symTab->getSymbol(labelName);

            if (symTab->table[sym].sectionNum != symTab->table[symTab->getSymbol(curSection->fullname)].sectionNum) throw AssemblerException(lineBegin);

            int val = symTab->table[sym].value - locationCounter;
            int maxImm = 1 << 11 - 1;
            if (val > maxImm) throw AssemblerException(lineBegin);
            addrReg = 31;
            curSection->content[locationCounter + 1] |= addrReg << 3;
            curSection->content[locationCounter + 1] |= dstReg >> 2;
            curSection->content[locationCounter + 2] |= dstReg << 6;
            curSection->content[locationCounter + 2] |= fOperand << 3;
            curSection->content[locationCounter + 2] |= (mnemIndex == 10) << 2;
            curSection->content[locationCounter + 2] |= val >> 8;
            curSection->content[locationCounter + 3] |= val;
        }

        break;
    }

    case 12: /* call */
    {
        char dstReg;
        int num = 0;
        dstReg = Parser::getRegistry(line);			//get dst reg

        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];

        if (dstReg != -1 && dstReg <= PSW && dstReg > 0)
        {
            line = Parser::skipRegistry(line);
            line = Parser::skipWhite(line);

            if (*line == ',')
            {
                line++;
                line = Parser::skipWhite(line);

                if (isdigit(*line))
                {

                    num = Parser::getNumber(line);
                    int maxNum = 1 << 20 - 1;
                    if (num > maxNum) throw AssemblerException(lineBegin);
                    line = Parser::skipDigits(line);
                    line = Parser::skipWhite(line);
                    if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
                    curSection->content[locationCounter + 1] |= num >> 16;
                    curSection->content[locationCounter + 2] |= num >> 8;
                    curSection->content[locationCounter + 3] |= num;

                }
                else throw AssemblerException(lineBegin);

            }
            else if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);

            curSection->content[locationCounter + 1] |= dstReg << 3;
        }

        else if (isalpha(*line))
        {
            char* labelStart = line;
            char* labelEnd = Parser::getName(line);
            char* labelName = new char[labelEnd - labelStart];
            strncpy(labelName, line, labelEnd - labelStart);
            labelName[labelEnd - labelStart] = NULL;

            int sym = symTab->getSymbol(labelName);
            if (symTab->table[sym].sectionNum != symTab->table[symTab->getSymbol(curSection->fullname)].sectionNum) throw AssemblerException(lineBegin);
            int gap = symTab->table[sym].value - locationCounter;
            int maxImm = (1 << 20) - 1;
            if (gap > maxImm) throw AssemblerException(lineBegin);
            line = Parser::skipWhite(labelEnd);
            if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);

            dstReg = 31;
            curSection->content[locationCounter + 1] |= dstReg << 3;
            curSection->content[locationCounter + 1] |= (gap >> 16) & 0x7;
            curSection->content[locationCounter + 2] |= gap >> 8;
            curSection->content[locationCounter + 3] |= gap;
        }
        else throw AssemblerException(lineBegin);
        break;
    }

    case 13:
    case 14:	/* in/out */
    {
        char dstReg, srcReg;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 15) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        srcReg = Parser::getRegistry(line);
        if (srcReg == -1 || srcReg > 15) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != NULL && *line != '\r') throw  AssemblerException(lineBegin);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        curSection->content[locationCounter + 1] |= dstReg << 4;
        curSection->content[locationCounter + 1] |= srcReg;
        curSection->content[locationCounter + 2] |= (mnemIndex == 13) << 7;
        break;
    }

    case 15:
    case 16:
    case 17:	/* mov/shr/shl */
    {
        char dstReg, srcReg, imm = 0;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 19) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        srcReg = Parser::getRegistry(line);
        if (srcReg == -1 || srcReg > 19) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line == ',' && mnemIndex == 15) throw  AssemblerException(lineBegin);
        if (*line == ',')
        {
            line++;
            line = Parser::skipWhite(line);
            imm = Parser::getNumber(line);
            if (imm < 0 || imm > 31) throw AssemblerException(lineBegin);
            line = Parser::skipDigits(line);
            line = Parser::skipWhite(line);
        }
        if (*line != NULL && *line != '\r') throw  AssemblerException(lineBegin);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        curSection->content[locationCounter + 1] |= dstReg << 3;
        curSection->content[locationCounter + 1] |= srcReg >> 2;
        curSection->content[locationCounter + 2] |= srcReg << 6;
        curSection->content[locationCounter + 2] |= imm << 1;
        curSection->content[locationCounter + 2] |= (mnemIndex == 17);
        break;
    }

    case 18:
    case 19:	/* ldch/ldcl */
    {
        char dstReg, srcReg;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 15) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        int num = Parser::getNumber(line);
        if (num > INT16_MAX) throw AssemblerException(lineBegin);
        line = Parser::skipDigits(line);
        line = Parser::skipWhite(line);
        if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[mnemIndex];
        curSection->content[locationCounter + 1] |= dstReg << 4;
        curSection->content[locationCounter + 1] |= (mnemIndex == 18) << 3;
        curSection->content[locationCounter + 2] |= num >> 8;
        curSection->content[locationCounter + 3] |= num;
        break;
    }

    case 20:	/* ldc */
    {
        char dstReg, srcReg;
        dstReg = Parser::getRegistry(line);			//get dst reg
        if (dstReg == -1 || dstReg > 15) throw  AssemblerException(lineBegin);
        line = Parser::skipRegistry(line);
        line = Parser::skipWhite(line);
        if (*line != ',') throw  AssemblerException(lineBegin);
        line++;
        line = Parser::skipWhite(line);
        /*ldch*/
        curSection->content[locationCounter] |= condIndex << 5;
        curSection->content[locationCounter] |= enableFlags << 4;
        curSection->content[locationCounter] |= opCodes[18];
        curSection->content[locationCounter + 1] |= dstReg << 4;
        curSection->content[locationCounter + 1] |= 1 << 3;
        /*ldcl*/
        curSection->content[locationCounter + 4] |= condIndex << 5;
        curSection->content[locationCounter + 4] |= enableFlags << 4;
        curSection->content[locationCounter + 4] |= opCodes[18];
        curSection->content[locationCounter + 5] |= dstReg << 4;
        curSection->content[locationCounter + 5] |= 0 << 3;
        if (isdigit(*line))
        {
            int num = Parser::getNumber(line);
            if (num > INT32_MAX) throw AssemblerException(lineBegin);
            line = Parser::skipDigits(line);
            line = Parser::skipWhite(line);
            if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
            /*Visa 2 bajta*/
            curSection->content[locationCounter + 2] |= num >> 24;
            curSection->content[locationCounter + 3] |= num >> 16;
            /*Niza 2 bajta*/
            curSection->content[locationCounter + 6] |= num >> 8;
            curSection->content[locationCounter + 7] |= num;
        }
        else if (isalpha(*line))
        {
            char *labelEnd, *labelStart;
            labelStart = line;
            labelEnd = Parser::getName(line);
            char *simbol1 = new char[labelEnd - labelStart];
            strncpy(simbol1, line, labelEnd - labelStart);
            simbol1[labelEnd - labelStart] = NULL;
            line = Parser::skipWhite(labelEnd);


            char *simbol2 = NULL;
            line = Parser::skipWhite(labelEnd);

            if (*line == '-')
            {
                line = Parser::skipWhite(line + 1);
                labelStart = line;
                labelEnd = Parser::getName(line);
                simbol2 = new char[labelEnd - labelStart];
                strncpy(simbol2, line, labelEnd - labelStart);
                simbol2[labelEnd - labelStart] = NULL;
                line = Parser::skipWhite(labelEnd);
                if (*line != NULL && *line != '\r') throw AssemblerException(lineBegin);
            }

            int sym = symTab->getSymbol(simbol1);
            if (sym == -1) throw AssemblerException(lineBegin);

            if (simbol2 != NULL)
            {
                bool found1 = false, found2 = false;
                int sym1 = symTab->getSymbol(simbol1);
                int sym2 = symTab->getSymbol(simbol2);

                if (sym1 == -1 || sym2 == -1) throw AssemblerException(lineBegin);

                if (!(symTab->table[sym1].defined && symTab->table[sym1].defined))
                {
                    relTab[curSection->id]->addRelocation(locationCounter, sym1, LDCADD);
                    relTab[curSection->id]->addRelocation(locationCounter, sym2, LDCSUB);
                }
                else if (symTab->table[sym1].sectionNum == symTab->table[sym2].sectionNum)
                {
                    int val = symTab->table[sym1].value - symTab->table[sym2].value;
                    curSection->content[locationCounter + 2] |= val >> 24;
                    curSection->content[locationCounter + 3] |= val >> 16;
                    curSection->content[locationCounter + 6] |= val >> 8;
                    curSection->content[locationCounter + 7] |= val;
                }
                else
                {

                    if (symTab->table[sym1].global)
                        relTab[curSection->id]->addRelocation(locationCounter, sym1, LDCADD);
                    else
                    {
                        relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym1].sectionNum, LDCADD);
                        curSection->content[locationCounter + 2] |= (symTab->table[sym1].value >> 24);
                        curSection->content[locationCounter + 3] |= (symTab->table[sym1].value >> 16);
                        curSection->content[locationCounter + 6] |= (symTab->table[sym1].value >> 8);
                        curSection->content[locationCounter + 7] |= (symTab->table[sym1].value);
                    }
                    if (symTab->table[sym2].global)
                        relTab[curSection->id]->addRelocation(locationCounter, sym2, LDCSUB);
                    else
                    {
                        relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym2].sectionNum, LDCSUB);
                        curSection->content[locationCounter + 2] -= (symTab->table[sym2].value >> 24);
                        curSection->content[locationCounter + 3] -= (symTab->table[sym2].value >> 16);
                        curSection->content[locationCounter + 6] -= (symTab->table[sym2].value >> 8);
                        curSection->content[locationCounter + 7] -= (symTab->table[sym2].value);
                    }
                }
            }

            else
            {
                if (symTab->table[sym].global)
                    relTab[curSection->id]->addRelocation(locationCounter, sym, LDCADD);
                else
                {
                    relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym].sectionNum, LDCADD);
                    /*Visa 2 bajta simbola*/
                    curSection->content[locationCounter + 2] |= (symTab->table[sym].value >> 24);
                    curSection->content[locationCounter + 3] |= (symTab->table[sym].value >> 16);
                    /*Niza 2 bajta simbola*/
                    curSection->content[locationCounter + 6] |= (symTab->table[sym].value >> 8);
                    curSection->content[locationCounter + 7] |= (symTab->table[sym].value);
                }
            }
        }
        else throw AssemblerException(lineBegin);
        break;
    }

    default:
    {
        break;
    }

    }

    if (mnemIndex == 20) locationCounter += 4;
    locationCounter += 4;
}


void Asembler::decodeDirective(char* line)
{
    char* lineBegin = line;
    char* start = line;
    char* end = Parser::getName(line + 1);
    char* directiveName = new char[end - start];
    strncpy(directiveName, line, end - start);
    directiveName[end - start] = '\0';
    bool invalid = true;
    line = end;

    if (strcmp(directiveName, ".text") == 0 || strcmp(directiveName, ".data") == 0 || strcmp(directiveName, ".bss") == 0)
    {
        char* subname = NULL;
        if (*line == '.')
        {
            char* subnameStart = line;
            char* subnameEnd = Parser::getSectionSubname(line + 1);
            if (subnameEnd == NULL) throw AssemblerException(lineBegin);
            subname = new char[subnameEnd - subnameStart];
            strncpy(subname, subnameStart, subnameEnd - subnameStart);
            subname[subnameEnd - subnameStart] = '\0';
        }
        for (int i = 0; i < sections.size(); i++)
        {

            if (sections[i]->subname == NULL && subname != NULL) continue;
            if (sections[i]->subname != NULL && subname == NULL) continue;
            if (sections[i]->subname == NULL && subname == NULL && strcmp(sections[i]->name, directiveName) != 0) continue;

            if (sections[i]->subname == NULL && subname == NULL && strcmp(sections[i]->name, directiveName) == 0)
            {
                curSection = sections[i];
                locationCounter = 0;
                curSection->allocateContent(curSection->size);
                break;
            }
            if (strcmp(sections[i]->subname, subname) == 0 && strcmp(sections[i]->name, directiveName) == 0)
            {
                curSection = sections[i];
                locationCounter = 0;
                curSection->allocateContent(curSection->size);
                break;
            }

        }
        invalid = false;
    }

    if (strcmp(directiveName, ".extern") == 0 || strcmp(directiveName, ".global") == 0)
    {
        line = Parser::skipWhite(line);
        vector<char*> labels = Parser::getLabels(line);
        if (labels.empty()) throw AssemblerException(lineBegin);
        invalid = false;
    }

    if (strcmp(directiveName, ".char") == 0)
    {
        if (curSection == NULL) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(end);
        if (*line == NULL || *line == '\r') throw  AssemblerException(lineBegin);
        while (*line != NULL && *line != '\r')
        {
            char num = 0;
            if (*line == ',') line++;
            line = Parser::skipWhite(line);
            if (isdigit(*line))
            {
                num = (char)Parser::getNumber(line);
                if (num > CHAR_MAX) throw  AssemblerException(lineBegin);
                curSection->content[locationCounter] = num;
                line = Parser::skipDigits(line);
                line = Parser::skipWhite(line);
            }
            else if (*line == NULL || *line == '\r') break;
            else throw  AssemblerException(lineBegin);
            locationCounter++;
        }
        invalid = false;
    }

    if (strcmp(directiveName, ".word") == 0)
    {
        if (curSection == NULL) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(end);

        if (*line == NULL || *line == '\r') throw  AssemblerException(lineBegin);

        while (*line != NULL && *line != '\r')
        {
            char num = 0;
            if (*line == ',') line++;
            line = Parser::skipWhite(line);
            if (isdigit(*line))
            {
                int num = Parser::getNumber(line);
                if (num > INT16_MAX) throw  AssemblerException(lineBegin);
                curSection->content[locationCounter] = num & 0xFF;
                curSection->content[locationCounter + 1] = (num >> 8) & 0xFF;
                line = Parser::skipDigits(line);
                line = Parser::skipWhite(line);
            }
            else if (*line == NULL || *line == '\r') break;
            else throw  AssemblerException(lineBegin);

            locationCounter += 2;
        }
        invalid = false;
    }

    if (strcmp(directiveName, ".long") == 0)
    {
        if (curSection == NULL) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(end);
        if (*line == NULL || *line == '\r') throw AssemblerException(lineBegin);

        while (*line != NULL && *line != '\r')
        {
            char num = 0;

            char *labelStart = line;
            char *labelEnd, *labelName;
            if (isdigit(*line))
            {
                num = Parser::getNumber(line);
                if (num > INT32_MAX) throw  AssemblerException(lineBegin);
                curSection->content[locationCounter + 3] = num & 0xFF;
                curSection->content[locationCounter + 2] = (num >> 8) & 0xFF;
                curSection->content[locationCounter + 1] = (num >> 16) & 0xFF;
                curSection->content[locationCounter] = (num >> 24) & 0xFF;
                line = Parser::skipDigits(line);
                line = Parser::skipWhite(line);
            }
            else if (isalpha(*line))
            {
                labelEnd = Parser::getName(line);
                char *simbol1 = new char[labelEnd - labelStart];
                strncpy(simbol1, line, labelEnd - labelStart);
                simbol1[labelEnd - labelStart] = NULL;
                char *simbol2 = NULL;
                line = Parser::skipWhite(labelEnd);

                if (*line == '-')
                {
                    line = Parser::skipWhite(line + 1);
                    labelStart = line;
                    labelEnd = Parser::getName(line);
                    simbol2 = new char[labelEnd - labelStart];
                    strncpy(simbol2, line, labelEnd - labelStart);
                    simbol2[labelEnd - labelStart] = NULL;
                    line = Parser::skipWhite(labelEnd);
                }


                if (simbol2 != NULL)
                {
                    bool found1 = false, found2 = false;
                    int sym1 = symTab->getSymbol(simbol1);
                    int sym2 = symTab->getSymbol(simbol2);

                    if (sym1 == -1 || sym2 == -1) throw AssemblerException(lineBegin);

                    if (!(symTab->table[sym1].defined && symTab->table[sym1].defined))
                    {
                        relTab[curSection->id]->addRelocation(locationCounter, sym1, ADD);
                        relTab[curSection->id]->addRelocation(locationCounter, sym2, SUB);
                    }
                    else if (symTab->table[sym1].sectionNum == symTab->table[sym2].sectionNum)
                    {
                        int val = symTab->table[sym1].value - symTab->table[sym2].value;
                        curSection->content[locationCounter] |= val >> 24;
                        curSection->content[locationCounter + 1] |= val >> 16;
                        curSection->content[locationCounter + 2] |= val >> 8;
                        curSection->content[locationCounter + 3] |= val;
                    }
                    else
                    {

                        if (symTab->table[sym1].global)
                            relTab[curSection->id]->addRelocation(locationCounter, sym1, ADD);
                        else
                        {
                            relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym1].sectionNum, ADD);
                            curSection->content[locationCounter] |= (symTab->table[sym1].value >> 24);
                            curSection->content[locationCounter + 1] |= (symTab->table[sym1].value >> 16);
                            curSection->content[locationCounter + 2] |= (symTab->table[sym1].value >> 8);
                            curSection->content[locationCounter + 3] |= (symTab->table[sym1].value);
                        }
                        if (symTab->table[sym2].global)
                            relTab[curSection->id]->addRelocation(locationCounter, sym2, SUB);
                        else
                        {
                            relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym2].sectionNum, SUB);
                            curSection->content[locationCounter] -= (symTab->table[sym2].value >> 24);
                            curSection->content[locationCounter + 1] -= (symTab->table[sym2].value >> 16);
                            curSection->content[locationCounter + 2] -= (symTab->table[sym2].value >> 8);
                            curSection->content[locationCounter + 3] -= (symTab->table[sym2].value);
                        }
                    }
                }
                else
                {
                    int sym = symTab->getSymbol(simbol1);
                    if (sym == -1) throw AssemblerException(lineBegin);
                    line = Parser::skipWhite(labelEnd);

                    if (symTab->table[sym].global)
                        relTab[curSection->id]->addRelocation(locationCounter,sym, ADD);
                    else
                    {
                        relTab[curSection->id]->addRelocation(locationCounter, symTab->table[sym].sectionNum, ADD);
                        curSection->content[locationCounter] |= (symTab->table[sym].value >> 24);
                        curSection->content[locationCounter + 1] |= (symTab->table[sym].value >> 16);
                        curSection->content[locationCounter + 2] |= (symTab->table[sym].value >> 8);
                        curSection->content[locationCounter + 3] |= (symTab->table[sym].value);
                    }
                }
            }
            else throw AssemblerException(lineBegin);
            locationCounter += 4;
            if (*line == ',') line++;
            line = Parser::skipWhite(line);
        }
        invalid = false;
    }

    if (strcmp(directiveName, ".align") == 0)
    {
        int bytes = Parser::getNumber(end);
        int skipping = bytes - (locationCounter % bytes);
        for (int i = 0; i < skipping; i++) curSection->content[locationCounter++] = (char)0x00;
        invalid = false;
    }

    if (strcmp(directiveName, ".skip") == 0)
    {
        int bytes = Parser::getNumber(end);
        for (int i = 0; i < bytes; i++) curSection->content[locationCounter++] = (char) 0x00;
        invalid = false;
    }

    if (strcmp(directiveName, ".end") == 0)
    {
        endOfAssembling = 1;
        invalid = false;
    }

    if (invalid) throw  AssemblerException(lineBegin);


}

void Asembler::getDirective(char* line)
{
    char* lineBegin = line;
    bool invalid = true;


    char* nameStart = line;
    char* nameEnd = Parser::getName(line + 1);
    char* name = new char[nameEnd - nameStart + 1];
    strncpy(name, nameStart, nameEnd - nameStart);
    name[nameEnd - nameStart] = '\0';

    line = nameEnd;

    /*cout << name << endl;*/

    if (strcmp(name, ".text") == 0 || strcmp(name, ".data") == 0 || strcmp(name, ".bss") == 0)
    {
        char* subname = NULL;
        if (*line == '.')
        {
            char* subnameStart = line;
            char* subnameEnd = Parser::getSectionSubname(line + 1);
            if (subnameEnd == NULL) throw AssemblerException(lineBegin);
            subname = new char[subnameEnd - subnameStart];
            strncpy(subname, subnameStart, subnameEnd - subnameStart);
            subname[subnameEnd - subnameStart] = '\0';
        }

        if (curSection == NULL)
        {
            char* fullname = new char[MAX_NAME_LEN];
            *fullname = NULL;
            strcat(fullname, name);
            if (subname != NULL) strcat(fullname, subname);
            curSection = new Section(name, subname, symTab, secID++, fullname);

            symTab->addSymbol(fullname, symTab->table.size(), 0, 1, 0);
            locationCounter = 0;
        }
        else
        {
            curSection->size = locationCounter;
            sections.push_back(curSection);
            char* fullname = new char[MAX_NAME_LEN];
            *fullname = NULL;
            strcat(fullname, name);
            if (subname != NULL) strcat(fullname, subname);
            curSection = new Section(name, subname, symTab, secID++, fullname);

            symTab->addSymbol(fullname, symTab->table.size(), 0, 1, 0);
            locationCounter = 0;
        }
        invalid = false;
    }

    if (strcmp(name, ".extern") == 0 || strcmp(name, ".global") == 0)
    {
        line = Parser::skipWhite(line);
        vector<char*> labels = Parser::getLabels(line);
        if (labels.empty()) throw AssemblerException(lineBegin);

        int ext = 0;
        if (strcmp(name, ".extern") == 0) ext = 1;

        for (int i = 0; i < labels.size(); i++)
        {
            int sym = symTab->getSymbol(labels[i]);
            if (sym != -1) {
                if (symTab->table[sym].global == 0) symTab->table[sym].global = 1;
                if (ext) symTab->table[sym].ext = 1;
            }
            if (sym == -1) {
                symTab->addSymbol(labels[i], 0, 1, 0, 0);
                if (ext) symTab->setExternForLastSymbol();
            }
        }
        invalid = false;
    }


    if (strcmp(name, ".char") == 0)
    {
        if (strcmp(curSection->name, ".bss") == 0) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(line);
        vector<char*> labels = Parser::getLabels(line);
        if (labels.empty()) throw AssemblerException(lineBegin);
        locationCounter += labels.size();
        invalid = false;
    }

    if (strcmp(name, ".word") == 0)
    {
        if (strcmp(curSection->name, ".bss") == 0) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(line);
        vector<char*> labels = Parser::getLabels(line);
        if (labels.empty()) throw AssemblerException(lineBegin);
        locationCounter += labels.size() * 2;
        invalid = false;
    }

    if (strcmp(name, ".long") == 0)
    {
        if (strcmp(curSection->name, ".bss") == 0) throw AssemblerException(lineBegin);
        line = Parser::skipWhite(line);
        int size = Parser::getNumOfLabels(line);
        locationCounter += size * 4;
        invalid = false;
    }

    if (strcmp(name, ".align") == 0)
    {
        int bytes = Parser::getNumber(line);
        int skipping = bytes - (locationCounter % bytes);
        locationCounter += skipping;
        invalid = false;
    }

    if (strcmp(name, ".skip") == 0)
    {
        int bytes = Parser::getNumber(line);
        if (bytes == -1) throw new AssemblerException(lineBegin);
        locationCounter += bytes;
        invalid = false;
    }

    if (strcmp(name, ".end") == 0)
    {
        curSection->size = locationCounter;
        locationCounter = 0;
        sections.push_back(curSection);
        curSection = NULL;
        endOfAssembling = endOfFirstPass ? 1 : 0;
        endOfFirstPass ^= 1;
        invalid = false;
    }


    if (invalid == true) throw AssemblerException(lineBegin);
}

char* Asembler::processLabel(char* line)
{
    if (curSection == NULL) throw AssemblerException(line);

    char* start = line;
    char* end = strchr(line, ':');
    char* labelName = new char[end - start + 1];
    strncpy(labelName, start, end - start);
    labelName[end - start] = '\0';
    int sym = symTab->getSymbol(labelName);

    if (sym != -1)
        if (symTab->table[sym].defined) throw AssemblerException(line);
        else
        {
            symTab->table[sym].defined = 1;
            symTab->table[sym].sectionNum = symTab->table[symTab->getSymbol(curSection->fullname)].sectionNum;
            symTab->table[sym].value = locationCounter;
        }

    if (sym == -1) 	symTab->addSymbol(labelName, symTab->table[symTab->getSymbol(curSection->fullname)].sectionNum, 0, 1, locationCounter);
    return end + 1;

}
