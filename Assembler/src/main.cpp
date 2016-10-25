#include <cstdio>
#include <iostream>
#include "asembler.h"
using namespace std;

int main(int argNum, char* args[])
{
    if (argNum != 3) {
        cout << "Wrong number of arguments!" << endl;
        return 0;
    }
    try
    {

            Asembler *asmb = new Asembler(args[1], args[2]);

            asmb->doAssembling();

            delete asmb;

    }

    catch (AssemblerException aex)
    {
        cout << aex << endl;
    }
    catch (UndefinedSymbolException use) {
        cout << use << endl;
    }
    return 0;
}
