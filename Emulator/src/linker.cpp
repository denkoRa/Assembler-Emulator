#include "linker.h"
#include "parser.h"

using namespace std;

#define SIZE_OF_RELTAB_ENTRY 3*sizeof(int)
#define DWORD_SIZE 4
#define STACK_SIZE 1024
#define SIMPLE_ADD 0
#define SIMPLE_SUB 1
#define LDC_ADD 2
#define LDC_SUB 3


Linker::Linker(char** objs, int _cnt)
{
    cnt = _cnt - 1;
    objFiles = new FILE* [cnt];
    symTab = new SymbolTable*[cnt];
    additionalSimbols = new SymbolTable();
    skript.open(objs[1]);
    for (int i = 0; i < cnt; i++) objFiles[i] = fopen(objs[i + 2], "rb");
    for (int i = 0; i < cnt; i++) symTab[i] = new SymbolTable();
    textSections = new vector<Section*>[cnt];
    dataSections = new vector<Section*>[cnt];
    bssSections = new vector<Section*>[cnt];
}

Linker::~Linker()
{
    delete [] memory;
}

char* Linker::getProgram() const
{
    return memory;
}

int Linker::getProgramSize() const
{
    return memorySize;
}

void Linker::linkAndLoad()
{
    readObjFiles();
    processScript();
    calculateSymbolValues();
    /*
    for (int k = 0; k < cnt; k++)
    {
        cout << "Tabela simbola:" << endl;
        for (int j = 0; j < symTab[k]->table.size(); j++)
        {
            cout << j << ": " << symTab[k]->table[j].name << ", section: " << symTab[k]->table[j].sectionNum <<
                 ", value: " << symTab[k]->table[j].value << endl;
        }
        cout << "---------------------" << endl;
    }
    cout << "Dodatni simboli iz skripte" << endl;
    for (int i = 0; i < additionalSimbols->table.size(); i++)
    {
        cout << i << ": " << additionalSimbols->table[i].name << ", value: " << additionalSimbols->table[i].value << endl;
    }
    */
    manageRellocations();
    load();
    /*
    for (int i = 0; i < memorySize; i++)
        cout << +memory[i] << " ";
    */
}

void Linker::readObjFiles()
{
    for (int i = 0; i < cnt; i++)
    {
        char elf[4];
        fread(elf, sizeof(char), 3, objFiles[i]);
        elf[3] = '\0';
        if (strcmp(elf, "ELF") != 0) throw LinkerException("Unrecognizable file!");
        int numOfTextSections, numOfDataSections, numOfbssSections, symTabSize;

        fread(&numOfTextSections, sizeof(int), 1, objFiles[i]);
        fread(&numOfDataSections, sizeof(int), 1, objFiles[i]);
        fread(&numOfbssSections, sizeof(int), 1, objFiles[i]);
        fread(&symTabSize, sizeof(int), 1, objFiles[i]);

        for (int j = 0; j < numOfTextSections; j++)
        {
            Section* newSec = new Section();
            int indexInSymTable;
            int size;
            int sizeOfRelTable;
            fread(&indexInSymTable, sizeof(int), 1, objFiles[i]);
            fread(&size, sizeof(int), 1, objFiles[i]);
            fread(&sizeOfRelTable, sizeof(int), 1, objFiles[i]);
            newSec->size = size;
            newSec->relTabSize = sizeOfRelTable;
            newSec->indexInSymbolTable = indexInSymTable;
            textSections[i].push_back(newSec);
        }
        for (int j = 0; j < numOfDataSections; j++)
        {
            Section* newSec = new Section();
            int indexInSymTable;
            int size;
            int sizeOfRelTable;
            fread(&indexInSymTable, sizeof(int), 1, objFiles[i]);
            fread(&size, sizeof(int), 1, objFiles[i]);
            fread(&sizeOfRelTable, sizeof(int), 1, objFiles[i]);
            newSec->size = size;
            newSec->relTabSize = sizeOfRelTable;
            newSec->indexInSymbolTable = indexInSymTable;
            dataSections[i].push_back(newSec);
        }
        for (int j = 0; j < numOfbssSections; j++)
        {
            Section* newSec = new Section();
            int size;
            int indexInSymTable;
            fread(&indexInSymTable, sizeof(int), 1, objFiles[i]);
            fread(&size, sizeof(int), 1, objFiles[i]);
            newSec->size = size;
            newSec->indexInSymbolTable = indexInSymTable;
            newSec->allocateContent();
            bssSections[i].push_back(newSec);
        }
        for (int j = 0; j < numOfTextSections; j++)
        {
            textSections[i][j]->allocateContent();
            fread(textSections[i][j]->content, sizeof(char), textSections[i][j]->size, objFiles[i]);
        }
        for (int j = 0; j < numOfDataSections; j++)
        {
            dataSections[i][j]->allocateContent();
            fread(dataSections[i][j]->content, sizeof(char), dataSections[i][j]->size, objFiles[i]);
        }
        for (int j = 0; j < numOfTextSections; j++)
        {
            for (int k = 0; k < textSections[i][j]->relTabSize; k++)
            {
                int offset, idxSymTab, addOrSub;
                fread(&offset, sizeof(int), 1, objFiles[i]);
                fread(&idxSymTab, sizeof(int), 1, objFiles[i]);
                fread(&addOrSub, sizeof(int), 1, objFiles[i]);
                textSections[i][j]->relTab->addRelocation(offset, idxSymTab, addOrSub);
            }
        }
        for (int j = 0; j < numOfDataSections; j++)
        {
            for (int k = 0; k < dataSections[i][j]->relTabSize; k++)
            {
                int offset, idxSymTab, addOrSub;
                fread(&offset, sizeof(int), 1, objFiles[i]);
                fread(&idxSymTab, sizeof(int), 1, objFiles[i]);
                fread(&addOrSub, sizeof(int), 1, objFiles[i]);
                dataSections[i][j]->relTab->addRelocation(offset, idxSymTab, addOrSub);
            }
        }
        for (int j = 0; j < symTabSize; j++)
        {
            int nameSize;
            fread(&nameSize, sizeof(int), 1, objFiles[i]);
            char* name = new char[nameSize];
            fread(name, sizeof(char), nameSize, objFiles[i]);
            int secNum, global, defined, value;
            fread(&secNum, sizeof(int), 1, objFiles[i]);
            fread(&global, sizeof(int), 1, objFiles[i]);
            fread(&defined, sizeof(int), 1, objFiles[i]);
            fread(&value, sizeof(int), 1, objFiles[i]);
            symTab[i]->addSymbol(name, secNum, global, defined, value);
        }
    /*
        cout << "Tabela simbola:" << endl;
        for (int j = 0; j < symTab[i]->table.size(); j++)
        {
            cout << j << ": " << symTab[i]->table[j].name << ", section: " << symTab[i]->table[j].sectionNum <<
                 ", value: " << symTab[i]->table[j].value << endl;
        }
        cout << "---------------------" << endl;



        for (int j = 0; j < textSections[i].size(); j++)
        {
            cout << "Podaci i Tabela relokacija za text sekciju br. " << symTab[i]->table[textSections[i][j]->indexInSymbolTable].name << endl;

            for (int k = 0; k < textSections[i][j]->relTab->table.size(); k++)
                cout << textSections[i][j]->relTab->table[k].offset << " " << textSections[i][j]->relTab->table[k].indexInSymTab << " " <<
                     textSections[i][j]->relTab->table[k].type << endl;
            cout << "------------------------------------" << endl;
        }

        for (int j = 0; j < dataSections[i].size(); j++)
        {
            cout << "Tabela relokacija za data sekciju br. " << symTab[i]->table[dataSections[i][j]->indexInSymbolTable].name << endl;
            for (int k = 0; k < dataSections[i][j]->relTab->table.size(); k++)
                cout << dataSections[i][j]->relTab->table[k].offset << " " << dataSections[i][j]->relTab->table[k].indexInSymTab << " " <<
                     dataSections[i][j]->relTab->table[k].type << endl;
            cout << "------------------------------------" << endl;
        }

        for (int j = 0; j < textSections[i].size(); j++)
        {
            cout << "Sadrzaj text sekcije " << symTab[i]->table[textSections[i][j]->indexInSymbolTable].name << endl;
            for (int k = 0; k < textSections[i][j]->size; k++)
                cout << +textSections[i][j]->content[k]  << " " ;
            cout << endl << "--------------------" << endl;
        }

        for (int j = 0; j < dataSections[i].size(); j++)
        {
            cout << "Sadrzaj data sekcije " << symTab[i]->table[dataSections[i][j]->indexInSymbolTable].name << endl;
            for (int k = 0; k < dataSections[i][j]->size; k++)
                cout << +dataSections[i][j]->content[k] << " " ;
            cout << endl << "--------------------" << endl;
        }
        */
    }


}


void Linker::processScript()
{
    char * line = new char [256];
    locationCounter = 0;
    while (skript.getline(line, 256))
    {
        line = Parser::skipWhite(line);
        if (*line == NULL) continue;
        if (*line == '.')
        {
            if (strchr(line, '*'))   // Vise sekcija u pitanju
            {
                char* nameEnd = Parser::getName(line + 1);
                char* name = new char[nameEnd - line];
                strncpy(name, line, nameEnd - line);
                name[nameEnd - line] = '\0';
                line = nameEnd;

                storeAllSections(name);

            }
            else if (isalpha(*(line + 1)))  	// Sekcija u pitanju
            {
                char *nameEnd = Parser::getNameWithDot(line + 1);
                char *name = new char[nameEnd - line];
                strncpy(name, line, nameEnd - line);
                name[nameEnd - line] = '\0';
                bool found = false;
                Section* section = findSectionByName(name);

                line = Parser::skipWhite(nameEnd);

                if (*line == '=')
                {
                    int address = evalExpr(line + 1);
                    locationCounter =  address;
                }

                line = Parser::skipWhite(line);
                if (section != NULL)
                {
                    section->loadAddress = locationCounter;
                    section->loaded = 1;
                    //for (int i = 0; i < section->size; i++) memory[locationCounter++] = section->content[i];
                    locationCounter += section->size;
                }

            }

            else   /*Postavljanje vrednosti tekuce adrese*/
            {
                line = Parser::skipWhite(line + 1);
                if (*line != '=') throw LinkerException(line);
                if (*line == '=')
                {
                    int address = evalExpr(line + 1);

                    if (address < locationCounter) throw LinkerException("Going backwards with address - not possible!");
                    locationCounter = address;
                }
            }
        }

        else  if (isalpha(*line)) /*Simbol u pitanju*/
        {
            char *nameEnd = Parser::getName(line);
            char *name = new char[nameEnd - line];
            strncpy(name, line, nameEnd - line);
            name[nameEnd - line] = '\0';

            int val;

            line = Parser::skipWhite(nameEnd);
            if (*line == '=')
            {
                val = evalExpr(line + 1);
            }
            for (int i = 0; i < additionalSimbols->table.size(); i++)
            {
                if (strcmp(additionalSimbols->table[i].name, name) == 0) throw LinkerException("Double definition!");
            }
            for (int k = 0; k < cnt; k++)
            {
                for (int i = 0; i < symTab[k]->table.size(); i++)
                    if (strcmp(symTab[k]->table[i].name, name) == 0 && !externVar(k, i)) throw LinkerException("Double definition!");
            }
            additionalSimbols->addSymbol(name, 0, 1, 1, val);
        }



    }

    for (int k = 0; k < cnt; k++)
    {
        for (int i = 0; i < textSections[k].size(); i++)
            if (!textSections[k][i]->loaded)
            {
                Section *section = textSections[k][i];
                section->loadAddress = locationCounter;
                section->loaded = 1;
                locationCounter += section->size;
            }
        for (int i = 0; i < dataSections[k].size(); i++)
            if (!dataSections[k][i]->loaded)
            {
                Section *section = dataSections[k][i];
                section->loadAddress = locationCounter;
                section->loaded = 1;
                locationCounter += section->size;
            }
        for (int i = 0; i < bssSections[k].size(); i++)
            if (!bssSections[k][i]->loaded)
            {
                Section *section = bssSections[k][i];
                section->loadAddress = locationCounter;
                section->loaded = 1;
                locationCounter += section->size;
            }
    }

/*
    for (int k = 0; k < cnt; k++)
    {
        for (int i = 0; i < textSections[k].size(); i++)
            cout << "Sekcija " << symTab[k]->table[textSections[k][i]->indexInSymbolTable].name  << " u modulu " << k << " pocinje na adresi " << textSections[k][i]->loadAddress << endl;
        for (int i = 0; i < dataSections[k].size(); i++)
            cout << "Sekcija " << symTab[k]->table[dataSections[k][i]->indexInSymbolTable].name << " u modulu " << k << " pocinje na adresi " << dataSections[k][i]->loadAddress << endl;
        for (int i = 0; i < bssSections[k].size(); i++)
            cout << "Sekcija " << symTab[k]->table[bssSections[k][i]->indexInSymbolTable].name << " u modulu " << k << " pocinje na adresi " << bssSections[k][i]->loadAddress << endl;

    }
    */
}

void Linker::storeAllSections(char *name)
{

    if (strcmp(name, ".text") == 0)
    {
        for (int k = 0; k < cnt; k++)
            for (int i = 0; i < textSections[k].size(); i++)
            {
                if (textSections[k][i]->loaded == 0)
                {
                    textSections[k][i]->loadAddress = locationCounter;
                    textSections[k][i]->loaded = 1;
                    symTab[k]->table[textSections[k][i]->indexInSymbolTable].value = locationCounter;
                    locationCounter += textSections[k][i]->size;
                }
            }
    }
    if (strcmp(name, ".data") == 0)
    {
        for (int k = 0; k < cnt; k++)
            for (int i = 0; i < dataSections[k].size(); i++)
            {
                if (dataSections[k][i]->loaded == 0)
                {
                    dataSections[k][i]->loadAddress = locationCounter;
                    dataSections[k][i]->loaded = 1;
                    symTab[k]->table[dataSections[k][i]->indexInSymbolTable].value = locationCounter;
                    locationCounter += dataSections[k][i]->size;
                }
            }
    }
    if (strcmp(name, ".bss") == 0)
    {
        for (int k = 0; k < cnt; k++)
            for (int i = 0; i < bssSections[k].size(); i++)
            {
                if (bssSections[k][i]->loaded == 0)
                {
                    bssSections[k][i]->loadAddress = locationCounter;
                    bssSections[k][i]->loaded = 1;
                    symTab[k]->table[bssSections[k][i]->indexInSymbolTable].value = locationCounter;
                    locationCounter += bssSections[k][i]->size;
                }
            }
    }
}


int Linker::evalExpr(char * line)
{

    line = Parser::skipWhite(line);
    int num = 0;
    int ret = 0;
    int sign = 1;

    while (*line != NULL)
    {

        if (isdigit(*line))
        {
            num = Parser::getNumber(line);
            line = Parser::skipDigits(line);
            line = Parser::skipWhite(line);
            ret += sign * num;

            if (*line == '-') sign = -1;
            else if (*line != '+' && *line != NULL) throw LinkerException("Bad script!");
            else sign = 1;
            if (*line == NULL) break;
            line = Parser::skipWhite(line + 1);
        }

        else if (isalpha(*line))
        {
            char *nameEnd = Parser::getName(line);
            char *name = new char[nameEnd - line];
            strncpy(name, line, nameEnd - line);
            name[nameEnd - line] = '\0';

            if (strcmp(name, "align") == 0)
            {
                line = nameEnd;
                if (*line != '(') throw LinkerException("Expected ( after align!");
                line = Parser::skipWhite(line + 1);
                int adr;
                if (*line == '.')
                {
                    adr = locationCounter;
                    line++;
                }
                else if (isdigit(*line))
                {
                    adr = Parser::getNumber(line);
                    line = Parser::skipDigits(line);
                }
                else throw LinkerException("Bad format of align!");

                line = Parser::skipWhite(line);

                if (*line != ',') throw LinkerException("Expected , after first argument of align!");
                line = Parser::skipWhite(line + 1);
                int alignment = Parser::getNumber(line);
                line = Parser::skipDigits(line);
                line = Parser::skipWhite(line);
                if (*line != ')') throw LinkerException("Bad format of align!");
                line = Parser::skipWhite(line + 1);

                ret += sign * align(adr, alignment);

                if (*line == '-') sign = -1;
                else if (*line != '+' && *line != NULL) throw LinkerException("Bad script!");
                else sign = 1;
                if (*line == NULL) break;
                line = Parser::skipWhite(line + 1);
            }
            else
            {
                line = Parser::skipWhite(nameEnd);

                for (int i = 0; i < additionalSimbols->table.size(); i++)
                    if (strcmp(additionalSimbols->table[i].name, name) == 0)
                    {
                        ret += sign * additionalSimbols->table[i].value;
                        break;
                    }


                if (*line == '-') sign = -1;
                else if (*line != '+' && *line != NULL) throw LinkerException("Bad script!");
                else sign = 1;
                if (*line == NULL) break;
                line = Parser::skipWhite(line + 1);
            }
        }
        else if (*line = '.')
        {
            ret += sign * locationCounter;
            line = Parser::skipWhite(line + 1);

            if (*line == '-') sign = -1;
            else if (*line != '+' && *line != NULL) throw LinkerException("Bad script!");
            else sign = 1;

            if (*line == NULL) break;
            line = Parser::skipWhite(line + 1);

        }


    }

    return ret;

}

int Linker::align(int adr, int alignment)
{
    if (alignment > adr) return alignment;
    if (adr % alignment == 0) return adr;
    return adr + (alignment - adr % alignment);
}

Section* Linker::findSectionByName(char *name)
{

    for (int k = 0; k < cnt; k++)
    {
        for (int i = 0; i < textSections[k].size(); i++)
            if (strcmp(name, symTab[k]->table[textSections[k][i]->indexInSymbolTable].name) == 0)
            {
                return textSections[k][i];
                break;
            }
        for (int i = 0; i < dataSections[k].size(); i++)
            if (strcmp(name, symTab[k]->table[dataSections[k][i]->indexInSymbolTable].name) == 0)
            {
                return dataSections[k][i];
                break;
            }
        for (int i = 0; i < bssSections[k].size(); i++)
            if (strcmp(name, symTab[k]->table[bssSections[k][i]->indexInSymbolTable].name) == 0)
            {
                return bssSections[k][i];
                break;
            }
    }
}

void Linker::calculateSymbolValues()
{

    for (int k = 0; k < cnt; k++)
        for (int i = 1; i < symTab[k]->table.size(); i++)
        {
            if (isSection(k, i))
            {
                Section *section = findSectionByName(symTab[k]->table[i].name);
                symTab[k]->table[i].value = section->loadAddress;
                if (section->loadAddress + section->size > memorySize) memorySize = section->loadAddress + section->size;
            }

        }

    for (int k = 0; k < cnt; k++)
        for (int i = 1; i < symTab[k]->table.size(); i++)
        {
            if (!isSection(k, i) && !externVar(k, i))
            {
                Section *section = findSectionByName(symTab[k]->table[symTab[k]->table[i].sectionNum].name);
                symTab[k]->table[i].value += section->loadAddress;
            }
        }
    for (int k = 0; k < cnt; k++)
    {
        for (int i = 1; i < symTab[k]->table.size(); i++)
        {
            if (externVar(k, i))
            {
                symTab[k]->table[i].value = findExternVar(k, symTab[k]->table[i].name);
            }
        }
    }

    memory = new char [memorySize + STACK_SIZE];
}

int Linker::findExternVar(int notThat, char* name)
{
    for (int k = 0; k < cnt; k++)
    {
        if (k == notThat) continue;
        for (int i = 1; i < symTab[k]->table.size(); i++)
            if (strcmp(name, symTab[k]->table[i].name) == 0 && !externVar(k, i))
            {
                return symTab[k]->table[i].value;
            }
    }
    for (int i = 0; i < additionalSimbols->table.size(); i++)
    {
        if (strcmp(name, additionalSimbols->table[i].name) == 0)
        {
            return additionalSimbols->table[i].value;
        }
    }
    //cout << name << endl;
    throw LinkerException("Non-existing extern variable");
}

void Linker::manageRellocations()
{
    for (int k = 0; k < cnt; k++)
    {


        for (int i = 0; i < textSections[k].size(); i++)
        {
            RelTab *curRelTable = textSections[k][i]->relTab;
            Section *curSection = textSections[k][i];
            for (int j = 0; j < curRelTable->table.size(); j++)
            {
                int val = symTab[k]->table[curRelTable->table[j].indexInSymTab].value;
                int offset = curRelTable->table[j].offset;
                switch (curRelTable->table[j].type)
                {
                case SIMPLE_ADD :
                {
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 3] ;
                    if (d + val3 > 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 3] = curSection->content[offset+3] + val3;

                    int c = (unsigned char) curSection->content[offset + 2];
                    if (c + val2 + carry1 > 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 2] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 1];
                    if (b + val1 + carry2 > 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 1] += val1 + carry2;

                    curSection->content[offset] += val0 + carry3;
                    break;
                }
                case SIMPLE_SUB :
                {
                    val = val * (-1);
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 3] ;
                    if (d + val3 > 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 3] = curSection->content[offset+3] + val3;

                    int c = (unsigned char) curSection->content[offset + 2];
                    if (c + val2 + carry1 > 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 2] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 1];
                    if (b + val1 + carry2 > 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 1] += val1 + carry2;

                    curSection->content[offset] += val0 + carry3;
                    break;
                }
                case LDC_ADD :
                {
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 7];
                    if (d + val3 >= 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 7] += val3;


                    int c = (unsigned char) curSection->content[offset + 6];
                    if (c + val2 + carry1 >= 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 6] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 3];
                    if (b + val1 + carry2 >= 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 3] += val1 + carry2;

                    curSection->content[offset + 2] += val0 + carry3 ;


                    break;
                }
                case LDC_SUB :
                {
                    val = val * (-1);
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 7];
                    if (d + val3 >= 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 7] += val3;


                    int c = (unsigned char) curSection->content[offset + 6];
                    if (c + val2 + carry1 >= 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 6] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 3];
                    if (b + val1 + carry2 >= 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 3] += val1 + carry2;

                    curSection->content[offset + 2] += val0 + carry3 ;


                    break;
                }
                default:
                {
                    break;
                }
                }
            }

        }
        for (int i = 0; i < dataSections[k].size(); i++)
        {
            RelTab *curRelTable = dataSections[k][i]->relTab;
            Section *curSection = dataSections[k][i];
            for (int j = 0; j < curRelTable->table.size(); j++)
            {
                int val = symTab[k]->table[curRelTable->table[j].indexInSymTab].value;
                int offset = curRelTable->table[j].offset;
                switch (curRelTable->table[j].type)
                {
                case SIMPLE_ADD :
                {
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 3] ;
                    if (d + val3 > 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 3] = curSection->content[offset+3] + val3;

                    int c = (unsigned char) curSection->content[offset + 2];
                    if (c + val2 + carry1 > 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 2] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 1];
                    if (b + val1 + carry2 > 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 1] += val1 + carry2;

                    curSection->content[offset] += val0 + carry3;
                    break;
                }
                case SIMPLE_SUB :
                {
                    val = val * (-1);
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 3] ;
                    if (d + val3 > 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 3] = curSection->content[offset+3] + val3;

                    int c = (unsigned char) curSection->content[offset + 2];
                    if (c + val2 + carry1 > 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 2] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 1];
                    if (b + val1 + carry2 > 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 1] += val1 + carry2;

                    curSection->content[offset] += val0 + carry3;
                    break;
                }
                case LDC_ADD :
                {
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 7];
                    if (d + val3 >= 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 7] += val3;


                    int c = (unsigned char) curSection->content[offset + 6];
                    if (c + val2 + carry1 >= 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 6] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 3];
                    if (b + val1 + carry2 >= 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 3] += val1 + carry2;

                    curSection->content[offset + 2] += val0 + carry3 ;


                    break;
                }
                case LDC_SUB :
                {
                    val = val * (-1);
                    int carry1 = 0, carry2 = 0, carry3 = 0;
                    int val3 = val & 0xFF;
                    int val2 = (val >> 8) & 0xFF;
                    int val1 = (val >> 16) & 0xFF;
                    int val0 = (val >> 24) & 0xFF;

                    int d = (unsigned char) curSection->content[offset + 7];
                    if (d + val3 >= 256) carry1 = (d + val3) / 256;
                    curSection->content[offset + 7] += val3;


                    int c = (unsigned char) curSection->content[offset + 6];
                    if (c + val2 + carry1 >= 256) carry2 = (c + val2 + carry1) / 256;
                    curSection->content[offset + 6] += val2 + carry1;

                    int b = (unsigned char) curSection->content[offset + 3];
                    if (b + val1 + carry2 >= 256) carry3 = (b + val1 + carry2) / 256;
                    curSection->content[offset + 3] += val1 + carry2;

                    curSection->content[offset + 2] += val0 + carry3 ;


                    break;
                }
                default:
                {
                    break;
                }
                }
            }
        }
    }
}

void Linker::load()
{
    /* First sections from the script file */
    for (int k = 0; k < cnt; k++)
    {
        for (int i = 0; i < textSections[k].size(); i++)
        {
            Section *curSection = textSections[k][i];
            if (curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }
        for (int i = 0; i < dataSections[k].size(); i++)
        {
            Section *curSection = dataSections[k][i];
            if (curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }
        for (int i = 0; i < bssSections[k].size(); i++)
        {
            Section *curSection = bssSections[k][i];
            if (curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }

    }
    /* Then the remaining sections */
    for (int k  = 0; k < cnt; k++)
    {
        for (int i = 0; i < textSections[k].size(); i++)
        {
            Section *curSection = textSections[k][i];
            if (!curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }
        for (int i = 0; i < dataSections[k].size(); i++)
        {
            Section *curSection = dataSections[k][i];
            if (!curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }
        for (int i = 0; i < bssSections[k].size(); i++)
        {
            Section *curSection = bssSections[k][i];
            if (!curSection->loaded)
                for (int j = 0; j < curSection->size; j++)
                    memory[curSection->loadAddress + j] = curSection->content[j];
        }
    }

}

int Linker::isSection(int k, int i)
{
    return symTab[k]->table[i].sectionNum == i;
}

int Linker::externVar(int k, int i)
{
    return symTab[k]->table[i].sectionNum == 0;
}


