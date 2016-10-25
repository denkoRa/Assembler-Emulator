#include "emulator.h"
#include <termios.h>
using namespace std;

#define EQ 0
#define NE 1
#define GT 2
#define GE 3
#define LT 4
#define LE 5
#define ALL 7
#define INT 0
#define ADD 1
#define SUB 2
#define MUL 3
#define DIV 4
#define CMP 5
#define AND 6
#define OR 7
#define NOT 8
#define TEST 9
#define LDR_STR 10
#define CALL 12
#define IN_OUT 13
#define MOV_SHR_SHL 14
#define LDCHL 15

#define IOMEMORY_SIZE 0x5000
#define STACK_SIZE 1024

#define OUTPUT_BUFFER 8192
#define INPUT_BUFFER 0x1000
#define INPUT_STATUS 0x1010

#define TIMER_IVT_ENTRY 1
#define KEYBOARD_IVT_ENTRY 3
#define ILLEGAL_IVT_ENTRY 2

static struct termios old, newOne;

void initTermios(int echo)
{
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    newOne = old; /* make new settings same as old settings */
    newOne.c_lflag &= ~ICANON; /* disable buffered i/o */
    newOne.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &newOne); /* use these new terminal i/o settings now */
}

void resetTermios(void)
{
    tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
    char ch;
    initTermios(echo);
    ch = getchar();
    resetTermios();
    return ch;
}

/* Read 1 character without echo */
char getch(void)
{
    return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
    return getch_(1);
}



Emulator::Emulator(char* p, int progSize)
{
    program = p;
    programSize = progSize;
    for (int i = 0; i < 15; i++) registers[i] = 0;
    SP = progSize + STACK_SIZE;
    PC = 0;
    PSW = 0;
    endOfProgram = 0;
    int rc;

    rc = pthread_create(&timerThread, NULL, timerThreadRun, this);
    if (rc)
    {
        cout << "Error:unable to create thread," << rc << endl;
        return;
    }

    rc = pthread_create(&keyboardThread, NULL, keyboardThreadRun, this);
    if (rc)
    {
        cout << "Error:unable to create thread," << rc << endl;
        return;
    }


    timerInterruptRequest = 0;
    keyboardInterruptRequest = 0;
    IOMemory = new int[IOMEMORY_SIZE];
    for (int i = 0; i < IOMEMORY_SIZE; i++) IOMemory[i] = 0;
}

Emulator::~Emulator()
{
    delete  [] IOMemory;
}

void *Emulator::timerThreadRun(void *ptr)
{
    Emulator *emu = (Emulator*) ptr;
    while (!emu->endOfProgram)
    {
        //cout << "Tajmer t " << flush;
        sleep(1);
        emu->timerInterruptRequest = 1;
    }
    pthread_exit(NULL);
}


void *Emulator::keyboardThreadRun(void *ptr)
{
    Emulator *emu = (Emulator*) ptr;

    while (!emu->endOfProgram)
    {
        while (emu->keyboardInterruptRequest);
        char c;
        c = getch();
        if (+c == 27)
        {
            emu->endOfProgram = true;
            break;
        }
        else cout << c;
        emu->keyboardCharacter = c;
        emu->keyboardInterruptRequest = 1;
    }

    pthread_exit(NULL);
}


void Emulator::emulate()
{
    resetInterrupt();
    int i = 0;


    while (!endOfProgram)
    {
        readAndExecuteInstruction();

        handleInterrupts();

        if (IOMemory[OUTPUT_BUFFER])
        {
            cout << (char)IOMemory[OUTPUT_BUFFER] << flush;

            IOMemory[OUTPUT_BUFFER] = 0;
        }

    }
    pthread_join(timerThread, NULL);
    pthread_join(periodicTimerThread, NULL);
    pthread_join(keyboardThread, NULL);

    cout << endl << "Kraj programa.\n"  << flush;

}

void Emulator::readAndExecuteInstruction()
{
    unsigned char firstByte = program[PC];
    unsigned char secondByte = program[PC + 1];
    unsigned char thirdByte = program[PC + 2];
    unsigned char fourthByte = program[PC + 3];

    int condition = firstByte >> 5;
    int setFlags = (firstByte >> 4) & 1;
    int opCode = firstByte & 0xF;
    bool shouldExecute = false;

    //cout << opCode << endl;

    switch(condition)
    {
    case EQ:
    {
        if (getZ()) shouldExecute = true;
        break;
    }
    case NE:
    {
        if (!getZ()) shouldExecute = true;
        break;
    }
    case GT:
    {
        if (!getZ() && !getN()) shouldExecute = true;
        break;
    }
    case GE:
    {
        if (!getN()) shouldExecute = true;
        break;
    }
    case LT:
    {
        if (getN()) shouldExecute = true;
        break;
    }
    case LE:
    {
        if (getN() || getZ()) shouldExecute = true;
        break;
    }
    case ALL:
    {
        shouldExecute = true;
        break;
    }
    default:
        break;
    }


    if (!shouldExecute)
    {
        PC += 4;
        return;
    }

    switch(opCode)
    {
    case INT:
    {
        int src = secondByte >> 4;
        push(LR);
        LR = PC + 4;
        PC = getInt(program[src * 4]);
        push(PSW);
        setI();
        break;
    }
    case ADD:
    {
        int dst = secondByte >> 3;
        int imm = (secondByte >> 2) & 1;
        int src = 0;
        int immVal = 0;
        int firstOperand = getRegistry(dst), secondOperand, result;
        if (!imm)
        {
            src += (secondByte & 0x3) << 3;
            src += thirdByte >> 5;

            setRegistry(getRegistry(dst) + getRegistry(src), dst);
            result = getRegistry(dst);
            secondOperand = getRegistry(src);
        }
        else
        {
            immVal += (secondByte & 0x3) << 16;
            immVal += (thirdByte) << 8;
            immVal += fourthByte;
            immVal = signExt(immVal, 18);

            setRegistry(getRegistry(dst) + immVal, dst);
            result = getRegistry(dst);
            secondOperand = immVal;
        }
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
            if (!imm && firstOperand > (1<<31 - 1) - secondOperand) setC();
            else resetC();
            if (!imm && (firstOperand > 0 && secondOperand > 0 && result < 0) || (firstOperand < 0 && secondOperand < 0 && result > 0)) setO();
            else resetO();
        }
        PC += 4;
        break;
    }
    case SUB:
    {
        int dst = secondByte >> 3;
        int imm = (secondByte >> 2) & 1;
        int src = 0;
        int immVal = 0;
        int firstOperand = getRegistry(dst), secondOperand, result;
        if (!imm)
        {
            src += (secondByte & 0x3) << 3;
            src += thirdByte >> 5;
            setRegistry(getRegistry(dst) - getRegistry(src), dst);
            result = getRegistry(dst);
            secondOperand = getRegistry(src);
        }
        else
        {
            immVal += (secondByte & 0x3) << 16;
            immVal += (thirdByte) << 8;
            immVal += fourthByte;
            immVal = signExt(immVal, 18);

            setRegistry(getRegistry(dst) - immVal, dst);
            result = getRegistry(dst);
            secondOperand = immVal;
        }
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
            if (!imm && firstOperand > (1 << 31 - 1) - secondOperand) setC();
            else resetC();
            if (!imm && (firstOperand > 0 && secondOperand > 0 && result < 0) || (firstOperand < 0 && secondOperand < 0 && result > 0)) setO();
            else resetO();
        }
        PC += 4;
        break;
    }
    case MUL:
    {
        int dst = secondByte >> 3;
        int imm = (secondByte >> 2) & 1;
        int src = 0;
        int immVal = 0;
        int firstOperand = getRegistry(dst), secondOperand, result;
        if (!imm)
        {
            src += (secondByte & 0x3) << 3;
            src += thirdByte >> 5;

            setRegistry(getRegistry(dst) * getRegistry(src), dst);
            result = getRegistry(dst);
            secondOperand = getRegistry(src);
        }
        else
        {
            immVal += (secondByte & 0x3) << 16;
            immVal += (thirdByte) << 8;
            immVal += fourthByte;
            immVal = signExt(immVal, 18);

            setRegistry(getRegistry(dst) * immVal, dst);
            result = getRegistry(dst);
            secondOperand = immVal;
        }
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
            if (!imm && firstOperand > (1 << 31 - 1) - secondOperand) setC();
            else resetC();
            if (!imm && (firstOperand > 0 && secondOperand > 0 && result < 0) || (firstOperand < 0 && secondOperand < 0 && result > 0)) setO();
            else resetO();
        }
        PC += 4;
        break;
    }
    case DIV:
    {
        int dst = secondByte >> 3;
        int imm = (secondByte >> 2) & 1;
        int src = 0;
        int immVal = 0;
        int firstOperand = getRegistry(dst), secondOperand, result;
        if (!imm)
        {
            src += (secondByte & 0x3) << 3;
            src += thirdByte >> 5;

            setRegistry(getRegistry(dst) / getRegistry(src), dst);
            result = getRegistry(dst);
            secondOperand = getRegistry(src);
        }
        else
        {
            immVal += (secondByte & 0x3) << 16;
            immVal += (thirdByte) << 8;
            immVal += fourthByte;
            immVal = signExt(immVal, 18);
            setRegistry(getRegistry(dst) / immVal, dst);
            result = getRegistry(dst);
            secondOperand = immVal;
        }
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
            if (!imm && firstOperand > (1 << 31 - 1) - secondOperand) setC();
            else resetC();
            if (!imm && (firstOperand > 0 && secondOperand > 0 && result < 0) || (firstOperand < 0 && secondOperand < 0 && result > 0)) setO();
            else resetO();
        }
        PC += 4;
        break;
    }
    case CMP:
    {
        int dst = secondByte >> 3;
        int imm = (secondByte >> 2) & 1;
        int src = 0;
        int immVal = 0;
        int firstOperand = getRegistry(dst), secondOperand, result;
        if (!imm)
        {
            src += (secondByte & 0x3) << 3;
            src += thirdByte >> 5;
            result = getRegistry(dst) - getRegistry(src);
            secondOperand = getRegistry(src);
        }
        else
        {
            immVal += (secondByte & 0x3) << 16;
            immVal += (thirdByte) << 8;
            immVal += fourthByte;
            immVal = signExt(immVal, 18);
            result = getRegistry(dst) - immVal;
            secondOperand = immVal;
        }
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
            if (firstOperand > (1 << 31 - 1) - secondOperand) setC();
            else resetC();
            if ((firstOperand > 0 && secondOperand > 0 && result < 0) || (firstOperand < 0 && secondOperand < 0 && result > 0)) setO();
            else resetO();
        }
        PC += 4;
        break;
    }
    case AND:
    {
        int dst = secondByte >> 3;
        int src = (secondByte & 0x7) << 2;
        src += thirdByte >> 6;

        setRegistry(getRegistry(dst) & getRegistry(src), dst);

        if (setFlags)
        {
            if (getRegistry(dst) == 0) setZ();
            if (getRegistry(dst) < 0) setN();
        }

        PC += 4;
        break;
    }
    case OR:
    {
        int dst = secondByte >> 3;
        int src = (secondByte & 0x7) << 2;
        src += thirdByte >> 6;

        setRegistry(getRegistry(dst) | getRegistry(src), dst);

        if (setFlags)
        {
            if (getRegistry(dst) == 0) setZ();
            else resetZ();
            if (getRegistry(dst) < 0) setN();
            else resetN();
        }

        PC += 4;
        break;
    }
    case NOT:
    {
        int dst = secondByte >> 3;
        int src = (secondByte & 0x7) << 2;
        src += thirdByte >> 6;

        setRegistry(~getRegistry(src), dst);

        if (setFlags)
        {
            if (getRegistry(dst) == 0) setZ();
            else resetZ();
            if (getRegistry(dst) < 0) setN();
            else resetN();
        }

        PC += 4;
        break;
    }
    case TEST:
    {
        int dst = secondByte >> 3;
        int src = (secondByte & 0x7) << 2;
        src += thirdByte >> 6;
        int result = getRegistry(dst) & getRegistry(src);
        if (setFlags)
        {
            if (result == 0) setZ();
            else resetZ();
            if (result < 0) setN();
            else resetN();
        }

        PC += 4;
        break;
    }
    case LDR_STR:
    {
        int adr = secondByte >> 3;

        int r = (secondByte & 0x7) << 2;
        r += (thirdByte >> 6);

        int f = (thirdByte >> 3) & 0x7;

        int load = (thirdByte >> 2) & 1;

        int imm = (thirdByte & 0x3) << 8;
        imm += fourthByte;
        imm = signExt(imm, 10);

        if (f == 4) setRegistry(getRegistry(adr) + 4, adr);
        if (f == 5) setRegistry(getRegistry(adr) - 4, adr);

        if (load)
        {
            setRegistry(getInt(getRegistry(adr) + imm), r);
        }
        else
        {
            setInt(getRegistry(adr) + imm, getRegistry(r));
        }

        if (f == 2) setRegistry(getRegistry(adr) + 4, adr);
        if (f == 3) setRegistry(getRegistry(adr) - 4, adr);

        PC += 4;
        break;
    }

    case CALL:
    {
        int dst = secondByte >> 3;

        int imm = (secondByte & 0x7) << 16;
        imm += (thirdByte << 8) + (fourthByte);
        imm = signExt(imm, 19);

        push(LR);
        LR = PC + 4;

        if (dst == 31) PC += imm;
        else PC = getRegistry(dst) + imm;

        break;
    }

    case IN_OUT:
    {
        int dst = secondByte >> 4;
        int src = secondByte & 0xF;

        int I_O = (thirdByte >> 7) & 1;

        if (I_O)
        {
            setRegistry(readFromIO(getRegistry(src)), dst);
        }
        else
        {
            storeIO(getRegistry(src), getRegistry(dst));
        }

        PC += 4;

        break;
    }

    case MOV_SHR_SHL:
    {
        int dst = secondByte >> 3;

        int src = (secondByte & 0x7) << 2;
        src += (thirdByte >> 6);

        int imm = (thirdByte >> 1) & 0x1F;

        int L_R = (thirdByte & 1);


        if (L_R)
        {
            setRegistry(getRegistry(src) << imm, dst);
        }
        else
        {
            setRegistry(getRegistry(src) >> imm, dst);
        }

        bool ret = false;
        if (dst == 16 && src == 17 && setFlags)
        {
            PSW = pop();
            ret = true;
        }
        if (dst == 16 && src == 17)
        {
            LR = pop();
            ret = true;
        }



        if (setFlags)
        {
            if (getRegistry(dst) == 0) setZ();
            else resetZ();
            if (getRegistry(dst) < 0) setN();
            else resetN();
            if ((getRegistry(src) >> (imm - 1)) & 1) setC();
            else resetC();
        }

        if (dst != 16) PC += 4;

        break;
    }

    case LDCHL:
    {
        int dst = secondByte >> 4;
        int H_L = (secondByte >> 3) & 1;

        int constant = (thirdByte << 8) + fourthByte;

        if (H_L)
        {
            setRegistry(constant << 16, dst);
        }
        else
        {
            if (getRegistry(dst) >> 16 > 0) setRegistry(constant + getRegistry(dst), dst);
            else setRegistry(constant, dst);
        }

        PC += 4;
        break;
    }

    default:
        illegalInstructionInterruptRequest = 1;
        break;
    }
}

void Emulator::setRegistry(int val, int i)
{
    switch (i)
    {
    case 16:
        PC = val;
        break;
    case 17:
        LR = val;
        break;
    case 18:
        SP = val;
        break;
    case 19:
        PSW = val;
        break;
    default:
        registers[i] = val;
        break;
    }
}

int Emulator::getRegistry(int i)
{

    switch(i)
    {
    case 16:
    {
        return PC;
        break;
    }
    case 17:
    {
        return LR;
        break;
    }
    case 18:
    {
        return SP;
        break;
    }
    case 19:
    {
        return PSW;
        break;
    }
    default:
        return registers[i];
    }
}

int Emulator::getInt(int pos) const
{
    int ret = 0;
    ret += ( unsigned char) program[pos] << 24;
    ret += ( unsigned char) program[pos + 1] << 16;
    ret += ( unsigned char) program[pos + 2] << 8;
    ret += ( unsigned char) program[pos + 3];
    return ret;
}

void Emulator::setInt(int pos, int val)
{
    program[pos] = (unsigned char) (val >> 24);
    program[pos + 1] = (unsigned char) (val >> 16);
    program[pos + 2] = (unsigned char) (val >> 8);
    program[pos + 3] = (unsigned char) val;
}

int Emulator::readFromIO(int pos)
{
    if (pos == 4096) IOMemory[4112] = 0;
    return IOMemory[pos];
}

void Emulator::storeIO(int pos, int val)
{
    IOMemory[pos] = val;
}

int Emulator::pop()
{
    int ret = getInt(SP);
    SP += 4;
    if (SP > programSize + STACK_SIZE) throw EmulatorException("Stack Underflow!");
    return ret;
}

void Emulator::push(int val)
{
    SP -= 4;
    if (SP < programSize) throw EmulatorException("Stack Overflow!");
    setInt(SP, val);
}

int Emulator::signExt(int val, int size)
{
    int temp = val << (32 - size);
    temp >>= 32 - size;
    return temp;
}

void Emulator::handleInterrupts()
{

    if (timerInterruptRequest)
    {
        timerInterrupt();
    }

    if (keyboardInterruptRequest)
    {
        keyboardInterrupt();
    }

    if (illegalInstructionInterruptRequest)
    {
        illegalInstructionInterrupt();
    }

}

void Emulator::resetInterrupt()
{
    PC = getInt(0);
}

void Emulator::illegalInstructionInterrupt()
{
    if (getI()) return;
    push(LR);
    LR = PC;
    push(PSW);
    setI();
    PC = getInt(ILLEGAL_IVT_ENTRY * sizeof(int));
    illegalInstructionInterruptRequest = 0;
}

void Emulator::timerInterrupt()
{
    if (getI()) return;
    if (!getTimerMask()) return;
    push(LR);
    LR = PC;
    push(PSW);
    setI();
    PC = getInt(TIMER_IVT_ENTRY * sizeof(int));
    timerInterruptRequest = 0;
}

void Emulator::keyboardInterrupt()
{
    if (getI()) return;
    push(LR);
    LR = PC;
    push(PSW);
    setI();
    IOMemory[4096] = keyboardCharacter;
    IOMemory[4112] = 1 << 9;
    PC = getInt(KEYBOARD_IVT_ENTRY * sizeof(int));
    keyboardInterruptRequest = 0;
}

int Emulator::getTimerMask() const
{
    return (PSW >> 30) & 1;
}

void Emulator::setTimerMask()
{
    PSW |= (1 << 30);
}

int Emulator::getI() const
{
    return (PSW >> 31) & 1;
}

void Emulator::setI()
{
    PSW |= (1 << 31);
}

int Emulator::getZ() const
{
    return (PSW >> 3) & 1;
}

void Emulator::setZ()
{
    PSW |= (1 << 3);
}

void Emulator::resetZ()
{
    PSW &= ~(1 << 3);
}

int Emulator::getO() const
{
    return (PSW >> 2) & 1;
}

void Emulator::setO()
{
    PSW |= (1 << 2);
}

void Emulator::resetO()
{
    PSW &= ~(1 << 2);
}

int Emulator::getC() const
{
    return (PSW >> 1) & 1;
}

void Emulator::setC()
{
    PSW |= (1 << 1);
}

void Emulator::resetC()
{
    PSW &= ~(1 << 1);
}

int Emulator::getN() const
{
    return PSW & 1;
}

void Emulator::setN()
{
    PSW |= 1;
}

void Emulator::resetN()
{
    PSW &= ~1;
}
