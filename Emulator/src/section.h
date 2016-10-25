#pragma once

#include <iostream>
#include <cstdio>
#include "reltab.h"
using namespace std;

class Section
{
public:

    int size, relTabSize, indexInSymbolTable;
    RelTab* relTab;
    char* content;

    int loadAddress;
    int loaded;

    Section()
    {
        loaded = 0;
        relTab = new RelTab();
    }

    void allocateContent()
    {
        content = new char[size];
    }

};
