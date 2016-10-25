
#pragma once
#include <iostream>
#include <vector>
#include <cstdio>

using namespace std;


class RelTab {
private:
	struct RelEntry {
		int offset;
		int indexInSymTab;
		int type;
		RelEntry(int off, int idx, int _addOrSub) : offset(off), indexInSymTab(idx), type(_addOrSub) {}
	};

public:
	vector<RelEntry> table;
	void addRelocation(int off, int idx, int addOrSub) {
		table.push_back(RelEntry(off, idx, addOrSub));
	}
};
