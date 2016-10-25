#pragma once
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include "section.h"
#include "symtab.h"
using namespace std;

class LinkerException
{
public:
    char* line;
    LinkerException(char* line)
    {
        this->line = line;
    }
    friend ostream& operator << (ostream& os, LinkerException &exception)
    {
        //for (int i = 0; line[i] != '\0'; i++) os << line[i];
        //return os << endl;
        return os << exception.line << endl;
    }

};

class Linker
{
private:
    FILE** objFiles;
    ifstream skript;
    int cnt;
    int memorySize;
    vector<Section*> *textSections;
    vector<Section*> *dataSections;
    vector<Section*> *bssSections;
    SymbolTable** symTab;
    SymbolTable* additionalSimbols;
    char* memory;
public:
    Linker(char**, int);
    ~Linker();
    void linkAndLoad();
    void readObjFiles();
    void processScript();
    void calculateSymbolValues();
    void manageRellocations();
    void load();
    char* getProgram() const;
    int getProgramSize() const;
private:
    int locationCounter;
    int evalExpr(char*);
    Section* findSectionByName(char*);
    void storeAllSections(char*);
    int isSection(int, int);
    int externVar(int, int);
    int findExternVar(int, char*);
    int align(int, int);
};
