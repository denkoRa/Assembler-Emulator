#pragma once
#include <iostream>
#include <string>

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

    static char* getNameWithDot(char* line)
    {
        int i = 0;
        while (isalpha(line[i]) || line[i] == '.') i++;
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



    static char* skipDigits(char* line)
    {
        int i = 0;
        line = skipWhite(line);
        while (isdigit(line[i])) i++;
        return line + i;
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
