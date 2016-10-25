
#include "symtab.h"
#include <cstring>

using namespace std;

int SymbolTable::getSymbol(char * name)
{

	for (int i = 0; i < table.size(); i++)
		if (strcmp(name, table[i].name) == 0) return i;
	return -1;

}

void SymbolTable::addSymbol(char * label, int secNum, int glob, int defined, int value)
{
	table.push_back(Symbol(label, secNum, glob, defined, value));
}

void SymbolTable::addSymbol(Symbol s)
{
	table.push_back(s);
}
