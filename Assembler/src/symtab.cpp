#include "symtab.h"

void SymbolTable::addSymbol(char* label, int secNum, int glob, int defined, int value)
{
	bool found = false;
	for (int i = 0; i < table.size(); i++)
		if (strcmp(table[i].name, label) == 0) {
			found = true;
			if (table[i].sectionNum == 0) table[i].sectionNum = secNum;
			if (table[i].value == 0) table[i].value = value;
			table[i].defined = defined;
		}
	if (!found) {
		Symbol newEntry(label, secNum, glob, defined, value);
		table.push_back(newEntry);
	}
}


int SymbolTable::getSymbol(char* name) {
	for (int i = 0; i < table.size(); i++)
		if (strcmp(name, table[i].name) == 0) return i;
	return -1;
}

void SymbolTable::setExternForLastSymbol() {
    table[table.size() - 1].ext = 1;
}
