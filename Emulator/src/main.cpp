#include <iostream>
#include "linker.h"
#include "emulator.h"

#include <pthread.h>

using namespace std;



int main(int argc, char** argv)
{
    /*
    char* objs[5];
    objs[1] = "skript.txt";
    objs[2] = "treciTest";
    objs[3] = "treciTestFact";
    objs[4] = "prviTestIVT";
    // objs[3] = "obj4";
    //  objs[3] = "obj3";
    */
    if (argc < 3)
    {
        cout << "Wrong number of parameters!" << endl;
        return 0;
    }
    try
    {

        Linker* linker = new Linker(argv, argc - 1);

        linker->linkAndLoad();

        Emulator *emu = new Emulator(linker->getProgram(), linker->getProgramSize());

        emu->emulate();

        delete emu;
        delete linker;
    }
    catch (LinkerException lex)
    {
        cout << lex;
    }
    catch (EmulatorException eex)
    {
        cout << eex;
    }

    return 0;
}
