#pragma once
#include <iostream>
#include <vector>

using namespace std;

class RelTab {
private:
	struct RelEntry {
		int offset;
		int indexInSymTab;
		int addOrSub;
		RelEntry(int off, int idx, int _addOrSub) : offset(off), indexInSymTab(idx), addOrSub(_addOrSub) {}
	};
	
public:
	vector<RelEntry> table;
	void addRelocation(int off, int idx, int addOrSub) {
		table.push_back(RelEntry(off, idx, addOrSub));
	}
};