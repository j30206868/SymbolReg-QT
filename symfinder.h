#ifndef SYMFINDER_H
#define SYMFINDER_H

#include "symboltype.h"
#include <string>

class SymbolMatch{
    private:
        std::string filepath;
        trajData **symbols;
        int symAmt;				//symbol amount

    public:
        SymbolMatch(std::string fPath, int sAmount);
        void freeTemplates(int exception);
        trajData *getSymbol(int idx);
        int findBestMatchedSym(trajData *sample, double *simiList);
        int getEndIdx();
        int getSymAmt();
};


#endif // SYMFINDER_H

