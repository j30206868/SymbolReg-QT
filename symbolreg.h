#ifndef SYMBOLREG_H
#define SYMBOLREG_H


#include "symboltype.h"
#include "commonfunc.h"

#include <string>

//常數宣告
static int isSameTypeLimit = 1;

const double TAN10D = 0.176327;
const double TAN20D = 0.36397;
const double TAN25D = 0.4663;
const double TAN65D = 2.144507;
const double TAN70D = 2.747477;
const double TAN80D = 5.671282;

const double TAN15D = 0.2679492;
const double TAN35D = 0.7002075;
const double TAN45D = 1.0;
const double TAN55D = 1.4281480;
const double TAN75D = 3.7320508;

bool isSameType(int tempType, int sampleType);
//IntArray處理
void cleanIntA(intArray &data);
intArray copyIntA(intArray data);
//dualIntArray處理
dualIntArray fillSpaceWithDummy(dualIntArray data);
void cleanDualIntArray(dualIntArray &data);

intArray splitAsInt(std::string str, std::string delimiter);

double trajAxisPeriodABSMean(trajData &data, int axis, int sP, int eP, bool includeZero);
double trajAxisPeriodABSSTD(trajData &data, int axis, double mean, int sP, int eP, bool includeZero);
void cleanNotImportant(trajData &data, int axis, int sP, int cC, int endIdx);
bool cleanCCUnder10(trajData &data, int axis, int cTh, int avgTh, int endIdx, int cC, int zeroC, int sum);
void removeTail(trajData &data);
ctData sumOfPNTrajWithSign(trajData &data);

int newCTDataMergeV(int v1, int v2);
int ctDataMergeV(int v1, int v2);

intArray ctDataMergeXZToIntA(ctData data);
//getSpetialIdx flag
const int SP_FLAG_NOTHING = -1;
const int SP_FLAG_FULL_MATCH = 0;
const int SP_FLAG_WRONG_TYPE = 1;
const int SP_FLAG_FAR_DISTANCE = 2;
int getSpecialIdx(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, int sP, int &flag);
intArray insertDummy(intArray A, int insertPos);
//getZoneBestMatch
const int defaultLenLimit = 8;
const int EXEST_OVERLEN = 1;
const int EXEST_NOSPECIAL = 2;
const int EXEST_FULLMATCH = 100;
int getDisOfXZCT(intArray &tempIntA, intArray &sampleIntA, ctData &temp, ctData &sample, int sP, int eP = -1, bool diffTypeDoubleDis = false);
int* getFourValueInDual(int tP, int sP, intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample);
dualIntArray getZoneBestMatch(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, int sP, int &eP, int &exeState, int lenLimit = defaultLenLimit);
dualIntArray getZoneBestMatchLoop(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, bool debugMode = true);
ctData ctDataRecoverXZFromIntA(ctData &originCT, intArray intA);

//Compare similarity
intArray ctDataXZDistance(ctData data);
intArray* ctDataToIntA(ctData data);
double getCorrOfAxisWithNoZero( intArray temp, intArray sample, int times );

double meanOfABSIntA(intArray A);
double unMatchedPercent(intArray FA, intArray SA);

double showBestMatchResult(ctData Temp, ctData Sample, bool printResult = true);
//刪除沒得merge 但X跟Z軸的值卻又都小於平均值超過th倍的特徵
ctData removeNoisyFeatureByMean(ctData data, int th);

const int SYMREG_RMNOISE_BYMEAN = 1;
const int SYMREG_RMNOISE_BYLARGEST = 2;
const int SYMREG_RMNOISE_BYBOTH = 3;
ctData removeNoisyFeature(ctData data, int th, int method);

//皆有數字為0
//-1 => (x1,y1)為empty
//-2  => (x2,y2)為empty
//-3 => 皆empty
const int EMP_NO_EMPTY   =  0;
const int EMP_X1_EMPTY   = -1;
const int EMP_X2_EMPTY   = -2;
const int EMP_BOTH_EMPTY = -3;
int checkEmptySide(int x1, int y1, int x2, int y2);

void mergeContinuousSimilar(ctData &Temp, ctData &Sample);
void subSampleEigen(ctData &data, double sampleRate);

//一開始擷取完特徵值後 直接將連在一起 type相近的特徵值直接融合
void mergeSimilarType(ctData &data);

//比對
dualCTData compareTwoSymbol(trajData *temp, trajData *sample);

#endif // SYMBOLREG_H

