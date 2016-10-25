#pragma once
#include <iostream>
#include <string>
#include "asembler.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
using namespace std;



class Parser
{
private:

public:
    static char* skipWhite(char* line)
    {
        int i = 0;
        while (line[i] == ' ' || line[i] == '\t') i++;
        return line + i;
    }

    static void strncpy(char* label, char* line, int i)
    {
        for(int j = 0; j < i; j++) label[j] = line[j];
    }

    static char* getName(char* line)
    {
        int i = 0;
        while (isalpha(line[i])) i++;
        return line + i;
    }

    static int getNumOfLabels(char* line)
    {
        int ret = 1;

        while (*line != NULL && *line != '\r')
        {
            if (*line == ',') ret++;
            line++;
        }
        return ret;
    }

    static vector<char*> getLabels(char *line)
    {
        vector<char*> ret;
        ret.clear();
        while (1)
        {
            line = skipWhite(line);
            int i = 0;
            while (isalnum(line[i]) || line[i] == '_') i++;
            if (i == 0) return ret;
            if (line[i] == ',' || line[i] == '\0' || line[i] == '\r')
            {
                char* newLabel = new char[i + 1];
                strncpy(newLabel, line, i);
                newLabel[i] = '\0';
                ret.push_back(newLabel);
                line += i;
                if (*line == '\0' || *line == '\r') return ret;
                else
                {
                    line++;
                    continue;
                }
            }
            ret.clear();
            return ret;
        }

        return ret;
    }

    static int getNumber(char* line)
    {
        line = skipWhite(line);

        int i = 0;
        while (isdigit(line[i])) i++;

        char* strNum = new char[i];
        strNum[i] = '\0';
        strncpy(strNum, line, i);

        return atoi(strNum);
    }

    static int getRegistry(char* line)
    {
        if (*line == 'r')
        {
            line++;
            return getNumber(line);
        }
        else
        {
            char *specReg = new char[3];
            if (line[2] == 'W')
            {
                strncpy(specReg, line, 3);
                specReg[3] = '\0';
                if (strcmp("PSW", specReg) == 0) return 19;
            }
            else
            {
                strncpy(specReg, line, 2);
                specReg[2] = '\0';
                if (strcmp("PC", specReg) == 0) return 16;
                if (strcmp("LR", specReg) == 0) return 17;
                if (strcmp("SP", specReg) == 0) return 18;
            }
        }
        return -1;
    }

    static char* skipRegistry(char* line)
    {
        int i = 0;
        while (isalnum(line[i])) i++;
        return line + i;
    }


    static int getMnem(char* line)
    {
        int i = 0;
        int ret = -1;
        while (isalpha(line[i]))
        {
            char* checkName = new char[5];
            strncpy(checkName, line, i + 1);
            checkName[i + 1] = '\0';
            int found = check(checkName);
            if (found != -1)
            {
                ret = found;
            }
            i++;
        }
        return ret;
    }

    static int check(char* name)
    {
        for (int i = 0; i < 21; i++)
        {
            if (strcmp(name, Asembler::mnemonics[i]) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    static int getCond(char* line)
    {
        int i = 0;
        int ret = -1;
        while (isalpha(line[i]))
        {
            char* checkName = new char[3];
            strncpy(checkName, line, i + 1);
            checkName[i + 1] = '\0';
            int found = checkForCond(checkName);
            if (found != -1)
            {
                ret = found;
            }
            i++;
        }
        return ret;
    }

    static int checkForCond(char* name)
    {
        for (int i = 0; i < 8; i++)
        {
            if (strcmp(name, Asembler::conditions[i]) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    static int checkForDirective(char* line)
    {
        line = skipWhite(line);
        if (*line == '.')
        {
            line++;
            char* start = line;
            char* end = getName(line);
            char* directiveName = new char[end - start];
            strncpy(directiveName, line, end - start);
            directiveName[end - start] = '\0';

            if (strcmp(directiveName, "char") == 0)
            {
                return 1;
            }

            if (strcmp(directiveName, "word") == 0)
            {
                return 2;
            }

            if (strcmp(directiveName, "long") == 0)
            {
                return 4;
            }

            if (strcmp(directiveName, "skip") == 0)
            {
                int bytes = Parser::getNumber(end);
                if (bytes == -1) return -2;
                return bytes;
            }
        }
        else return -1;
    }

    static char* skipDigits(char* line)
    {
        int i = 0;
        line = skipWhite(line);
        while (isdigit(line[i])) i++;
        return line + i;
    }

    static int checkForInstruction(char* line)
    {
        line = skipWhite(line);
        int mnemIndex = getMnem(line);
        if (mnemIndex == 20) return 8;
        if (mnemIndex != -1) return 4;
        return -1;
    }

    static bool increment(char* line)
    {
        if (*line == '+' && *(line + 1) == '+') return true;
        return false;
    }

    static bool decrement(char* line)
    {
        if (*line == '-' && *(line + 1) == '-') return true;
        return false;
    }

    static char* getSectionSubname(char* line)
    {
        int i = 0;
        if (!isalpha(line[i])) return NULL;
        while (isalnum(line[i]) || line[i] == '_') i++;
        return line + i;
    }

    static char* skipLabel(char* line)
    {
        int i = 0;
        while (line[i] == '_' || isalpha(line[i])) i++;
        if (line[i] != ':') return NULL;
        return line + i;
    }
};
