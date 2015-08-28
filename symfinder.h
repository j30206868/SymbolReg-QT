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
        trajData *getSymbol(int idx);
        int findBestMatchedSym(trajData *sample, double *simiList);
        int getSymAmt();
};


#endif // SYMFINDER_H

