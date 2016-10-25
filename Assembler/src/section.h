#include "symtab.h"

using namespace std;


class Section {
public:
	int id;
	int size;
	SymbolTable* st;
	char* name;
	char* subname;
	char* fullname;
	char* content;
	int hasSubname;
	Section(char* name, char* _subname, SymbolTable *st, int _id, char* full) {
		(*this).st = st;
		(*this).name = name;
		if (_subname != NULL) (*this).subname = _subname;
		else subname = NULL;
		size = 0;
		id = _id;
		fullname = full;
	}

	void allocateContent(int size) {
		content = new char[size];
		for (int i = 0; i < size; i++) content[i] = 0;
	}

	~Section() {
		delete [] name;
		delete [] name;
		delete [] subname;
	}
};
