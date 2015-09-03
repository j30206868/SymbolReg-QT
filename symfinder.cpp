#include "symfinder.h"
#include "commonfunc.h"
#include "symbolreg.h"
#include "symrecorder.h"
#include "sstream"

//private

//public
SymbolMatch::SymbolMatch(std::string fPath, int sAmount){
    filepath = fPath;
    symAmt = sAmount;

    std::stringstream sstm;
    //Build symbol list
    symbols = new trajData*[symAmt];
    int symIdx = 0;	//SYMRECSTARTIDX可能從1開始, 因此symIdx要獨立自己算, 不能用i
    int endIdx = getEndIdx();
    for(int i = SYMRECSTARTIDX ; i <= endIdx ; i++){
        sstm << fPath << i << ".txt";
        symbols[symIdx] = readTrajDataFromFile(sstm.str());
        sstm.str("");
        symIdx++;
    }
}
void SymbolMatch::freeTemplates(int exceptionIdx){
    for(int i = 0 ; i < symAmt ; i++){
        if(i != exceptionIdx){
            freePTraj(symbols[i]);
        }
    }
}

trajData *SymbolMatch::getSymbol(int idx){
    return symbols[idx];
}
//if result idx == -1, means no matched symbol
int SymbolMatch::findBestMatchedSym(trajData *sample, double *simiList){
    double result;
    double maxSimilarity = 0.3;
    int maxSimIdx = -1;
    for(int idx = 0 ; idx < symAmt ; idx++){
        dualCTData bestMatchResult = compareTwoSymbol(symbols[idx], sample);
        result = showBestMatchResult(bestMatchResult.A, bestMatchResult.B, false);
        freeDualCT(bestMatchResult);
        simiList[idx] = result;
        if(maxSimilarity <= result){
            maxSimilarity = result;
            maxSimIdx = idx;
        }
    }

    return maxSimIdx;
}
int SymbolMatch::getEndIdx(){
    return symAmt + SYMRECSTARTIDX - 1;
}
int SymbolMatch::getSymAmt(){
    return this->symAmt;
}
