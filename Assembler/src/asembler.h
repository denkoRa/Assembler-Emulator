#pragma once
#include <cstdio>
#include <iostream>
#include <fstream>
#include "section.h"
#include "symtab.h"
#include "reltab.h"
using namespace std;

class AssemblerException {
public:
	char* line;
	AssemblerException(char* line) {
		this->line = line;
	}
	friend ostream& operator << (ostream& os, AssemblerException &exception) {
		os << "Error in processing line: ";
		return os << exception.line << endl;
	}

};

class UndefinedSymbolException {
public:
	char* line;
	UndefinedSymbolException(char* line) {
		this->line = line;
	}
	friend ostream& operator << (ostream& os, UndefinedSymbolException &exception) {
		os << "Undefined symbol ";
		return os << exception.line << endl;
	}

};


class Asembler {
private:
	ifstream asmInput;
	ifstream asmInput2;
	ofstream objOutput;
	Section* curSection ;
	SymbolTable* symTab;
	SymbolTable* symTabForLinker;
	int locationCounter;
	vector<Section*> sections;
	RelTab** relTab;

	int endOfFirstPass;
	int endOfAssembling;
	int endOfSecondPass;

public:
	static char mnemonics[21][5];
	static char conditions[8][3];
	static int mnemLength[21];
	static int opCodes[20];
	static int secID;
	Asembler(char* input, char* output);
	~Asembler();
	char* processLabel(char*);
	void doAssembling();
private:
	void getDirective(char*);
	void decodeDirective(char*);
	void decodeInstruction(char*);
	void firstPass();
	void secondPass();
	void generateELF();
	vector<Section*> getSections(char*);
	void byteByByte(int);
	void shrinkSymTable();
	bool isSection(int i);
	int length(char*);
};
