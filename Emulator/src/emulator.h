#pragma once

#include "linker.h"
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

class EmulatorException
{
public:
    char* line;
    EmulatorException(char* line)
    {
        this->line = line;
    }
    friend ostream& operator << (ostream& os, EmulatorException &exception)
    {
        //for (int i = 0; line[i] != '\0'; i++) os << line[i];
        //return os << endl;
        return os << exception.line << endl;
    }

};

class Emulator
{
public:
    Emulator(char*, int);
    void emulate();
    ~Emulator();
private:
    char *program;
     int registers[16];
    unsigned int PC, PSW, SP, LR;

    int endOfProgram;
    int programSize;
    char keyboardCharacter;

    int getZ() const;
    int getO() const;
    int getC() const;
    int getN() const;
    int getI() const;


    void setZ() ;
    void setO() ;
    void setC() ;
    void setN() ;
    void setI() ;

    void resetO();
    void resetN();
    void resetZ();
    void resetC();

    void setTimerMask();
    int getTimerMask() const;

    int* IOMemory;

    int signExt(int, int);

    void readAndExecuteInstruction();
    void handleInterrupts();

    void resetInterrupt();
    void timerInterrupt();
    void keyboardInterrupt();
    void periodicTimerInterrupt();
    void illegalInstructionInterrupt();

    void push(int);
    int pop();

    int getInt(int) const;
    void setInt(int, int);

    int readFromIO(int);
    void storeIO(int, int);

    void setRegistry(int, int);
    int getRegistry(int);

    pthread_t timerThread;
    static void *timerThreadRun(void*);

    pthread_t keyboardThread;
    static void *keyboardThreadRun(void*);

    pthread_t periodicTimerThread;
    static void *periodicTimerThreadRun(void*);

    int timerInterruptRequest;
    int keyboardInterruptRequest;
    int resetInterruptRequest;
    int periodicTimerRequest;
    int illegalInstructionInterruptRequest;
};
