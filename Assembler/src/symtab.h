#pragma once
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <cstring>
using namespace std;


struct Symbol {
	char* name;
	int sectionNum;
	int global;
	int value;
	int defined;
	int idxInLinkingTable;
	int ext;
	Symbol(char* _name, int _sectionNum, int _global, int _defined, int _value) {
		name = _name;
		defined = _defined;
		sectionNum = _sectionNum;
		global = _global;
		value = _value;
	}
};


class SymbolTable {
public:

	vector<Symbol> table;

	SymbolTable() {
		addSymbol("", 0, 0, 0, 0);
	}

	int getSymbol(char* name);

	void addSymbol(char* label, int secNum, int glob, int defined, int value);

    void setExternForLastSymbol();

};
