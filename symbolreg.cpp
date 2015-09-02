#include "SymbolReg.h"
#include <math.h>
#include <cstdlib>

#include "iostream"

#include <QObject>

using namespace std;

bool isSameType(int tempType, int sampleType)
{//用於ctData merge x跟z軸後的data
    if(tempType==0 || sampleType==0){
        return true;//type 0 是dummy 等於其他任何type
    }

    if(tempType == sampleType){
        return true;
    }

    int bigOne = max(tempType, sampleType);
    int smallOne = min(tempType, sampleType);
    int dis = abs(bigOne - smallOne);

    /*  dis <= N 若N大於1 要調整後面的部分 因為整個圓圈的設計14 15 16 後面是接 1 2 3 4*/
    if( isSameTypeLimit == 0 ){
        if(tempType == sampleType){
            return true;
        }else{
            return false;
        }
    }else if( isSameTypeLimit == 1 ){
        if( dis <= 1){
            return true;
        }else if(bigOne == 16 && smallOne == 1){
            return true;
        }
    }else if( isSameTypeLimit == 2 ){
        if( dis <= 2 ){
            return true;
        }else if(((bigOne  == 16 || bigOne   == 15) &&
                  (smallOne == 1 || smallOne == 2 ))){

            bigOne -= 16;

            dis = abs(bigOne - smallOne);
            if(dis <= 2){
                return true;
            }
        }
    }else if( isSameTypeLimit == 3 ){
        if( dis <= 3 ){
            return true;
        }else if(((bigOne  == 16 || bigOne   == 15 || bigOne == 14) &&
                (  smallOne == 1 || smallOne == 2  || smallOne == 3 ))){
            bigOne -= 16;

            dis = abs(bigOne - smallOne);
            if(dis <= 3){
                return true;
            }
        }
    }
    return false;
}

double trajAxisPeriodABSMean(trajData &data, int axis, int sP, int eP, bool includeZero){
    if(data.length==0)
        return 0;

    double mean = 0;
    int zeroC = 0;

    for(int i=sP ; i<eP ; i++){
        if(data.level[i][axis] == 0){
            zeroC++;
            continue;
        }
        mean += abs(data.level[i][axis]);
    }

    if(includeZero){
        zeroC=0;
    }
    if((eP-sP-zeroC)!=0)
        mean /= (eP-sP-zeroC);

    return mean;
}
double trajAxisPeriodABSSTD(trajData &data, int axis, double mean, int sP, int eP, bool includeZero){
    if(data.length==0)
        return 0;

    double std = 0;
    int zeroC = 0;

    for(int i=sP ; i<eP ; i++){
        if(data.level[i][axis] == 0 && includeZero){
            zeroC++;
            continue;
        }
        std += abs( getRound(mean - abs(data.level[i][axis])) );
    }
    if((eP-sP-zeroC)!=0)
        std /= (eP-sP-zeroC);

    return std;
}
void cleanNotImportant(trajData &data, int axis, int sP, int cC, int endIdx){
    double mean = trajAxisPeriodABSMean(data, axis, sP, (sP+cC), false);
    double std = abs( getRound(trajAxisPeriodABSSTD(data, axis, mean, sP, (sP+cC), false)) );
    double th = mean - (std/2);
    for(int inIdx = endIdx-1; inIdx >= (endIdx-cC) ; inIdx--){
        if(th > abs(data.level[inIdx][axis])){
            data.level[inIdx][axis] = 0;
            //printf( "[%d].[%d]=>0 ", axis, inIdx+1);
        }
    }
}
bool cleanCCUnder10(trajData &data, int axis, int cTh, int avgTh, int endIdx, int cC, int zeroC, int sum){
    double avg = 0;
    bool isCleaned = false;
    if(cC < cTh){
        cC = cC - zeroC;
        if(sum!=0 && cC>0)
            avg = abs(sum) / (double)cC;//不考慮正負

        if(avg < avgTh || cC <= getRound(cTh/2.0))
        {//將資料區段歸0 (這個值之前的舊資料區段)
            isCleaned = true;
            for(int inIdx = endIdx-1; inIdx >= endIdx-(cC+zeroC) ; inIdx--){
                data.level[inIdx][axis] = 0;
            }
        }
    }
    return isCleaned;
}
void removeTail(trajData &data){
    int cTh = 8;
    int avgTh = 8;
    int x=0, z=2;

    for(int axis=0 ; axis<=2; axis+=2){
        int sP = 0;
        int cC = 0;
        int sign = 0;
        int sum = 0;
        int zeroC = 0;

        for(int i=0 ; i<data.length ; i++){
            if( data.level[i][axis] == 0){
                zeroC++;
            }

            bool isCleaned = false;

            if( !isEqualSign(data.level[i][axis], sign) ){
                //碰到斷點, 統計整個連續資料區段
                //資料區段太小則歸0
                //printf("%d %d %d %d %d %d %d %d\n", data.level[i][axis], axis, cTh, avgTh, i, cC, zeroC, sum);
                isCleaned = cleanCCUnder10(data, axis, cTh, avgTh, i, cC, zeroC, sum);

                isCleaned = true;

                //將資料區段中, 小於門檻的值歸0
                //printf("%d to %d mean=%.2f std=%.2f", i, (i-cC)+1, mean, std);
                if(!isCleaned)
                {//若資料區段大小超過門檻被保留, 則消除不重要的值
                    cleanNotImportant(data, axis, sP, cC, i);
                }
                //printf("\n");
                cC = 0;
                sign = 0;
                zeroC = 0;
                sum = 0;
            }
            if(sign == 0)
                sign = getSign(data.level[i][axis]);

            sum += data.level[i][axis];

            if(cC == 0)
                sP = i;
            cC++;
        }
        //處理最後一筆
        //bool isCleaned = false;
        bool isCleaned = cleanCCUnder10(data, axis, cTh, avgTh, data.length, cC, zeroC, sum);

        isCleaned = true;

        if(!isCleaned)
        {//若資料區段大小超過門檻被保留, 則消除不重要的值
            cleanNotImportant(data, axis, sP, cC, data.length);
        }
    }

}
ctData sumOfPNTrajWithSign(trajData &data)
{//將正負波各自加總
    int buffer[2000][3] = {0};
    int cCount[2000] = {0};
    int idx = 0;
    int cC = 0; //continue count
    int sign[3] = {0};
    int x=0, y=1, z=2;

    //累加各軸 連正或負的值(考慮軸與軸之間關係)
    for(int i=0 ; i<data.length ; i++){
        //前處理
        if(cC > 0)
        {//累加中, 確定是否為連續正 或連續負的相加 (不管0)
            /*
                忽略Y軸
            */
            if( !isEqualSign(data.level[i][x], sign[x]) ||
                !isEqualSign(data.level[i][z], sign[z]) ){
                //x或z只要有不連續 就中斷 加到下一組
                //printf("sign (%d, %d) != (%d, %d)\n", data.level[i][x], data.level[i][z], sign[x], sign[z]);
                cCount[idx] = cC;
                cC = 0;
                idx++;
            }
        }

        //計算
        if(cC == 0){
            //新一組的初始
            if(abs(data.level[i][x]) != 1)
                buffer[idx][x] = data.level[i][x];
            else
                buffer[idx][x] = 0;

            if(abs(data.level[i][y]) != 1)
                buffer[idx][y] = data.level[i][y];
            else
                buffer[idx][y] = 0;

            if(abs(data.level[i][z]) != 1)
                buffer[idx][z] = data.level[i][z];
            else
                buffer[idx][z] = 0;
            //buffer[idx][x] = data.level[i][x];
            //buffer[idx][y] = data.level[i][y];
            //buffer[idx][z] = data.level[i][z];
            cC = 1;
            //printf("新一組的初始 ");
            //取得各軸的sign
            for(int axis=0; axis<3; axis++){
                if( buffer[idx][axis] > 0 ){
                    sign[axis] = 1;
                }else if( buffer[idx][axis] < 0 ){
                    sign[axis] = -1;
                }else{
                    sign[axis] = 0;
                }
            }
            //printf("sign=%d %d\n", sign[0], sign[2]);
        }else
        {//繼續累加
            if(abs(data.level[i][x]) != 1)
                buffer[idx][x] += data.level[i][x];
            if(abs(data.level[i][y]) != 1)
                buffer[idx][y] += data.level[i][y];
            if(abs(data.level[i][z]) != 1)
                buffer[idx][z] += data.level[i][z];
            cC++;

            if(sign[x] == 0)
                sign[x] = getSign(data.level[i][x]);
            if(sign[z] == 0)
                sign[z] = getSign(data.level[i][z]);
        }
    }
    //記錄最後一組的cC
    cCount[idx] = cC;

    //將buffer跟cCount的資料放入ctData
    int length = idx+1;

    int noVC = 0;
    for(int i=0 ; i<length ; i++){
        //若XZ都是0 則捨棄 否則會造成後面的問題
        if(buffer[i][0]==0 && buffer[i][2]==0)
            noVC++;
    }
    //printf("noVC:%d\n", noVC);

    int newLen = length - noVC;

    ctData newData;
    newData.level = new int*[newLen];
    newData.count = new int[newLen];
    newData.length = newLen;

    int newIdx = 0;
    //printf("ctData XZ:\n");
    for(int i=0 ; i<length ; i++){
        if(buffer[i][0]==0 && buffer[i][2]==0){
            //printf("%3d| (%d %d) count:%d\n", i+1, buffer[i][0], buffer[i][2], cCount[i]);
            //被消掉的count要加給前面的, 否則以後算SP EP會出錯
            if(newIdx > 0){
                //printf("oldcount = %d + %d\n", newData.count[newIdx-1],  cCount[i]);
                newData.count[newIdx-1] += cCount[i];
            }else
                newData.count[newIdx] += cCount[i];
            continue;
        }
        newData.level[newIdx] = new int[3];
        newData.count[newIdx] = cCount[i];
        for(int axis=0; axis<3; axis++){
            newData.level[newIdx][axis] = buffer[i][axis];
        }
        //printf("%3d| %d %d count:%d type:%d\n", i+1, buffer[i][0], buffer[i][2], newData.count[newIdx], ctDataMergeV(buffer[i][0], buffer[i][2]));
        newIdx++;
        if(newIdx < newLen){
            newData.count[newIdx] = 0;
        }
    }
    //system("PAUSE");
    return newData;
}
int ctDataMergeV(int v1, int v2){
    int type = 0;
    /*
        double TAN15D = 0.2679492;
        double TAN35D = 0.7002075;
        double TAN45D = 1.0;
        double TAN55D = 1.4281480;
        double TAN75D = 3.7320508;
    */
    //get ratio
    double ratio;

    if(v1 == 0 && v2 == 0){
        return 0;
    }

    if(v1 == 0){
        ratio = 0;
    }else if(v2 == 0){
        ratio = 1000;//無限大
    }else{
        ratio = abs(v1) / (double)abs(v2);
    }

    //get type
    if( v1 > 0 )
    {//右半球
        if( ratio > TAN75D ){
            type = 5;
        }else if( ratio > TAN55D ){
            if(v2 > 0){
                type = 4;
            }else{
                type = 6;
            }
        }else if( ratio >= TAN35D ){// tan 35
            if(v2 > 0){
                type = 3;
            }else{
                type = 7;
            }
        }else if( ratio > TAN15D ){
            if(v2 > 0){
                type = 2;
            }else{
                type = 8;
            }
        }else{// 0 ~ tan(15)
            if(v2 > 0){
                type = 1;
            }else{
                type = 9;
            }
        }
    }else
    {//左半球
        if( ratio > TAN75D ){
            type = 13;
        }else if( ratio > TAN55D ){
            if(v2 > 0){
                type = 14;
            }else{
                type = 12;
            }
        }else if( ratio >= TAN35D ){
            if(v2 > 0){
                type = 15;
            }else{
                type = 11;
            }
        }else if( ratio > TAN15D ){
            if(v2 > 0){
                type = 16;
            }else{
                type = 10;
            }
        }else{
            if(v2 > 0){
                type = 1;
            }else{
                type = 9;
            }
        }
    }

    return type;
}
intArray ctDataMergeXZToIntA(ctData data){
    intArray newData;
    newData.length = data.length;
    newData.values = new int[data.length];

    for(int idx=0 ; idx<newData.length ; idx++){
        newData.values[idx] = ctDataMergeV(data.level[idx][0], data.level[idx][2]);
    }
    return newData;
}

// get best match
int getSpecialIdx(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, int sP, int &flag)
{//flag存放 回傳的idx是屬於哪一種特徵點的idx -1:沒有spetial point 1:type不相容 2:差距過大可能有問題 0:full match point
    int minLen = min(tempIntA.length, sampleIntA.length);
    int sPoint = -1;
    flag = SP_FLAG_NOTHING;

    int tempZC = 0, sampleZC = 0;
    for(int i=0 ; i<minLen ; i++){
        if(tempIntA.values[i] == 0 || sampleIntA.values[i] == 0){
            if(tempIntA.values[i] == 0){
                tempZC++;
            }
            if(sampleIntA.values[i] == 0){
                sampleZC++;
            }
            continue;
        }

        if(i < sP)//還沒道sP
            continue;

        if(!isSameType(tempIntA.values[i], sampleIntA.values[i])){
            flag = SP_FLAG_WRONG_TYPE;
            sPoint = i;
            //printf("Temp: %d|%d %d Sample: %d|%d %d Type Diff\n", tempIntA.values[i], temp.level[ i-tempZC ][0], temp.level[ i-tempZC ][2], sampleIntA.values[i], sample.level[ i-sampleZC ][0], sample.level[ i-sampleZC ][2]);
            break;
        }else if(tempIntA.values[i]!=0 && sampleIntA.values[i]!=0)
        {//同type 看看距離是否過大
            //計算dis
            int disT, disS;
            int sumT, sumS;
            //根據temp的正負來計算 不管sample是否一樣

            disT = abs(temp.level[ i-tempZC ][0] - sample.level[ i-sampleZC ][0]);
            disS = abs(temp.level[ i-tempZC ][2] - sample.level[ i-sampleZC ][2]);

            sumT = abs(temp.level[ i-tempZC ][0]) + abs(temp.level[ i-tempZC ][2]);
            sumS = abs(sample.level[ i-sampleZC ][0]) + abs(sample.level[ i-sampleZC ][2]);

            //若無過大就是full match point
            //if( (disT + disS) > min(sumT, sumS)*3 )
            if( (disT + disS) > min(sumT, sumS)*2 )
            {//距離差距過大
                flag = SP_FLAG_FAR_DISTANCE;
                sPoint = i;
                //printf("Temp: %d|%d %d Sample: %d|%d %d 距離差距過大\n", tempIntA.values[i], temp.level[ i-tempZC ][0], temp.level[ i-tempZC ][2], sampleIntA.values[i], sample.level[ i-sampleZC ][0], sample.level[ i-sampleZC ][2]);
                break;
            }else{
            //else if((disT + disS) < min(sumT, sumS) * 2){
            //else if(((disT + disS) / max(sumT, sumS)) <= 0.5){
                //int oIsST = isSameTypeLimit;
                //isSameTypeLimit = 1;
                //if(isSameType(tempIntA.values[i], sampleIntA.values[i]) || (((double)disT + disS) / max(sumT, sumS)) <= 0.5){
                    flag = SP_FLAG_FULL_MATCH;
                    sPoint = i;
                    //printf("Temp: %d|%d %d Sample: %d|%d %d Full Match Point\n", tempIntA.values[i], temp.level[ i-tempZC ][0], temp.level[ i-tempZC ][2], sampleIntA.values[i], sample.level[ i-sampleZC ][0], sample.level[ i-sampleZC ][2]);
                    break;
                //}
                //isSameTypeLimit = oIsST;
            }
        }
    }
    return sPoint;
}
intArray insertZero(intArray A, int insertPos){
    intArray newArray;
    newArray.length = A.length + 1;
    newArray.values = new int[newArray.length];
    int oldIdx = 0;

    for(int i=0 ; i<newArray.length ; i++){
        if(i == insertPos){
            newArray.values[i] = 0;
        }else{
            newArray.values[i] = A.values[oldIdx];
            oldIdx++;
        }
    }

    return newArray;
}
int getDisOfXZCT(intArray &tempIntA, intArray &sampleIntA, ctData &temp, ctData &sample, int sP, int eP, bool diffTypeDoubleDis)
{//temp跟sample長度不同
    int maxL = max(tempIntA.length, sampleIntA.length);
    int distance = 0;
    int tempZC = 0;
    int sampleZC = 0;
    for(int idx=0 ; idx < maxL ; idx++)
    {
        //還沒道start point只要計算zero count就好
        if(idx < sP){
            if(tempIntA.values[idx] == 0)
                tempZC++;
            if(sampleIntA.values[idx] == 0)
                sampleZC++;
            continue;
        }else if(eP != -1 && idx > eP){
            break;
        }

        int valueTX = 0;
        int valueTZ = 0;
        int valueSX = 0;
        int valueSZ = 0;

        if(idx < tempIntA.length)
        {
            if(tempIntA.values[idx] == 0){
                tempZC++;
            }else{
                valueTX = temp.level[ idx-tempZC ][0];
                valueTZ = temp.level[ idx-tempZC ][2];
            }
        }

        if(idx < sampleIntA.length)
        {
            if(sampleIntA.values[idx] == 0){
                sampleZC++;
            }else{
                valueSX = sample.level[ idx-sampleZC ][0];
                valueSZ = sample.level[ idx-sampleZC ][2];
            }
        }
        //printf("(%d %d) (%d-%d)+(%d-%d)\n", tempIntA.values[idx], sampleIntA.values[idx], valueTX, valueSX, valueTZ, valueSZ);
        if(diffTypeDoubleDis && (!isSameType(tempIntA.values[idx], sampleIntA.values[idx]))){
            //printf("double \n");
            distance += ((abs(valueTX - valueSX) + abs(valueTZ - valueSZ)) * 3);
        }else{
            distance += abs(valueTX - valueSX) + abs(valueTZ - valueSZ);
        }
    }

    return distance;
}
int* getFourValueInDual(int tP, int sP, intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample){
    int minLen = max(tempIntA.length, sampleIntA.length);
    int tempZC = 0, sampleZC = 0;
    int tXV = 0, tZV = 0; bool isTFound = false;
    int sXV = 0, sZV = 0; bool isSFound = false;

    //確定tP再TempIntA範圍內
    //sP再SampleIntA範圍內
    if( (tP < 0 || sP < 0) || (tP >= tempIntA.length || sP >= sampleIntA.length) ){
        printf("getFourValueInDual:tP %d len:%d || sP %d len:%d value out of border\n", tP, tempIntA.length, sP, sampleIntA.length);
        system("PAUSE");
    }

    for(int i=0 ; i<minLen ; i++){
        if(i < tempIntA.length && tempIntA.values[i] == 0){
            tempZC++;
        }else{//非0找到才assign值 否則直接照預設值給0
            if(i < tempIntA.length &&
               tP == i)
            {//找到t的point了
                tXV = temp.level[ i-tempZC ][0];
                tZV = temp.level[ i-tempZC ][2];
            }
        }

        if(i < sampleIntA.length && sampleIntA.values[i] == 0){
            sampleZC++;
        }else{
            if(i < sampleIntA.length &&
               sP == i)
            {//找到s的point了
                sXV = sample.level[ i-sampleZC ][0];
                sZV = sample.level[ i-sampleZC ][2];
            }
        }

        if(isTFound && isSFound)
            break;
    }
    int *result = new int[4];
    result[0] = tXV;
    result[1] = tZV;
    result[2] = sXV;
    result[3] = sZV;
    return result;
}
dualIntArray getZoneBestMatch(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, int sP, int &eP, int &exeState, int lenLimit){
    //eP = 檢查過的最後一個點
    if(lenLimit <= 0){
        dualIntArray OM;
        OM.A = tempIntA;
        OM.B = sampleIntA;
        int intAMaxIdx = min(tempIntA.length, sampleIntA.length) - 1;
        if(sP <= intAMaxIdx){
            eP = sP;//使用時會-1
        }else{
            eP = intAMaxIdx;
        }
        exeState = EXEST_OVERLEN;
        //printf("getZoneBestMatch: 超過lenLimit\n");
        //system("PAUSE");
        return OM;
    }

    int spFlag = 0;
    int spIdx = getSpecialIdx(tempIntA, sampleIntA, temp, sample, sP, spFlag);
    if(spFlag == SP_FLAG_FULL_MATCH){
        dualIntArray OM;
        OM.A = tempIntA;
        OM.B = sampleIntA;
        eP = spIdx;
        exeState = EXEST_FULLMATCH;
        return OM;
    }else if(spFlag == SP_FLAG_WRONG_TYPE){
        intArray newTemp = insertZero(tempIntA, spIdx);
        intArray newSample = insertZero(sampleIntA, spIdx);

        int fEp = -1;
        int sEp = -1;
        int fMEState = 0;
        int sMEState = 0;
        dualIntArray firstM = getZoneBestMatch(newTemp, sampleIntA, temp, sample, spIdx+1, fEp, fMEState, lenLimit-1);
        dualIntArray secondM = getZoneBestMatch(tempIntA, newSample, temp, sample, spIdx+1, sEp, sMEState, lenLimit-1);

        //printf("getFirstDis\n");
        int firstDis = getDisOfXZCT(firstM.A, firstM.B, temp, sample, spIdx, fEp, true);
        //printf("getSecondDis\n");
        int secondDis = getDisOfXZCT(secondM.A, secondM.B, temp, sample, spIdx, sEp, true);// 最後一點不算的話 若是overlen則會區分不出與full match差別

        /*if(lenLimit == defaultLenLimit){
            printf("\nfirstDis:%d state:%d\nsecondDis:%d state:%d\n", firstDis, fMEState, secondDis, sMEState);
            showCTWithTYpe("First Matrix", firstM.A, firstM.B, temp, sample, spIdx-1, fEp);
            showCTWithTYpe("Second Matrix", secondM.A, secondM.B, temp, sample, spIdx-1, sEp);
        }*/

        double disPercent = abs(firstDis-secondDis) / (double)max(firstDis, secondDis);
        //lenLimit == defaultLenLimit 一定要; 在每個區塊的總結部分差距過近 才需要進到下一輪, 否則 錯誤的分支很可能也會近到下一輪 沒完沒了
        if( (disPercent <= 0.1) && lenLimit == defaultLenLimit )
        {//若差距很微小, 且eP點的值不一樣, 則各自以此eP點為基礎 往下作一輪的Match看cost的差距
            //看看結尾是否一樣 若一樣則不需要比 隨便挑一個


            int *fVOfFirstM = getFourValueInDual(fEp, fEp, firstM.A, firstM.B, temp, sample);
            int *fVOfSecondM = getFourValueInDual(sEp, sEp, secondM.A, secondM.B, temp, sample);
            if( (fVOfFirstM[0] != fVOfSecondM[0] || fVOfFirstM[1] != fVOfSecondM[1]) ||
                (fVOfFirstM[2] != fVOfSecondM[2] || fVOfFirstM[3] != fVOfSecondM[3]))
            {//從eP的下一點 往下做下一輪比較
                //長度直接給Default值
                //printf("FirstM:%d %d %d %d\n", fVOfFirstM[0], fVOfFirstM[1], fVOfFirstM[2], fVOfFirstM[3]);
                //printf("SecondM:%d %d %d %d\n", fVOfSecondM[0], fVOfSecondM[1], fVOfSecondM[2], fVOfSecondM[3]);
                /*printf("差距過小, 進行進階比較\n");
                system("PAUSE");*/
                fMEState = 0;
                firstM = getZoneBestMatch(firstM.A, firstM.B, temp, sample, fEp+1, fEp, fMEState);
                sMEState = 0;
                secondM = getZoneBestMatch(secondM.A, secondM.B, temp, sample, sEp+1, sEp, sMEState);

                firstDis = getDisOfXZCT(firstM.A, firstM.B, temp, sample, spIdx, fEp, true);
                secondDis = getDisOfXZCT(secondM.A, secondM.B, temp, sample, spIdx, sEp, true);

                /*printf("進階比較後 firstDis:%d secondDis:%d\n", firstDis, secondDis);
                showCTWithTYpe("First Matrix", firstM.A, firstM.B, temp, sample, spIdx-1, fEp);
                showCTWithTYpe("Second Matrix", secondM.A, secondM.B, temp, sample, spIdx-1, sEp);
                system("PAUSE");*/
            }
            /*else{
                printf("差距過小 但結尾一樣, 不需進行進階比較\n");
                system("PAUSE");
            }*/
        }

        if(firstDis <= secondDis){
            //printf("fEP:%d\n", fEp);
            cleanIntA(newSample);
            eP = fEp;
            exeState = fMEState;
            return firstM;
        }else{
            //printf("sEp:%d\n", sEp);
            cleanIntA(newTemp);
            eP = sEp;
            exeState = sMEState;
            return secondM;
        }
    }else if(spFlag == SP_FLAG_FAR_DISTANCE){
        intArray newTemp = insertZero(tempIntA, spIdx);
        intArray newSample = insertZero(sampleIntA, spIdx);

        int fEp = -1;
        int sEp = -1;
        int oEp = -1;

        dualIntArray OM;
        OM.A = copyIntA(tempIntA);
        OM.B = copyIntA(sampleIntA);
        int fMEState = 0;
        int sMEState = 0;
        int oMEState = 0;
        dualIntArray firstM = getZoneBestMatch(newTemp, sampleIntA, temp, sample, spIdx+1, fEp, fMEState, lenLimit-1);
        dualIntArray secondM = getZoneBestMatch(tempIntA, newSample, temp, sample, spIdx+1, sEp, sMEState, lenLimit-1);
        OM = getZoneBestMatch(OM.A, OM.B, temp, sample, spIdx+1, oEp, oMEState, lenLimit-1);

        //printf("getFirstDis\n");
        int firstDis = getDisOfXZCT(firstM.A, firstM.B, temp, sample, spIdx, fEp, true); // full match的點不用算入
        //printf("getSecondDis\n");
        int secondDis = getDisOfXZCT(secondM.A, secondM.B, temp, sample, spIdx, sEp, true);
        //printf("getOriginDis\n");
        int originDis = getDisOfXZCT(OM.A, OM.B, temp, sample, spIdx, oEp, true);

        /*if(lenLimit == defaultLenLimit){
            printf("firstDis:%d secondDis:%d originDis:%d\n", firstDis, secondDis, originDis);
            showCTWithTYpe("First Matrix", firstM.A, firstM.B, temp, sample, spIdx-1, fEp);
            showCTWithTYpe("Second Matrix", secondM.A, secondM.B, temp, sample, spIdx-1, sEp);
            showCTWithTYpe("Original Matrix",OM.A, OM.B, temp, sample, spIdx-1, oEp);
        }*/

        double foPercent = (originDis - firstDis) / (double)max(firstDis, originDis);
        double soPercent = (originDis - secondDis) / (double)max(secondDis, originDis);

        if( firstDis <= secondDis && foPercent > 0.1){
            //printf("fEp:%d\n", fEp);
            cleanIntA(newSample);
            eP = fEp;
            exeState = fMEState;
            return firstM;
        }else if( secondDis < firstDis && soPercent > 0.1){
            //printf("sEp:%d\n", sEp);
            cleanIntA(newTemp);
            eP = sEp;
            exeState = sMEState;
            return secondM;
        }else{
            //printf("oEp:%d\n", oEp);
            cleanIntA(newTemp);
            cleanIntA(newSample);
            eP = oEp;
            exeState = oMEState;
            return OM;
        }
    }else{//(spFlag == SP_FLAG_NOTHING)
        dualIntArray OM;
        OM.A = tempIntA;
        OM.B = sampleIntA;
        eP = min(tempIntA.length, sampleIntA.length) - 1; //後面的每個pair都是沒有match的(如9 0), 如果在比較實計算distance就全部都算進去
        exeState = EXEST_NOSPECIAL;
        return OM;
    }

    //傳回的bestmatch
    //要筆dis還是相似度要權衡一下 否則會選錯

    //若是超過maxLen而被打回票的 要特別改變eP? 還是中間那些真的就全部不管?
}
dualIntArray getZoneBestMatchLoop(intArray tempIntA, intArray sampleIntA, ctData &temp, ctData &sample, bool debugMode){
    dualIntArray bestDual;
    bestDual.A = tempIntA;
    bestDual.B = sampleIntA;

    int sP = 0;
    while( sP < min(bestDual.A.length, bestDual.B.length) )
    {
        int eP = -1;
        int exeState = 0;
        bestDual = getZoneBestMatch(bestDual.A, bestDual.B, temp, sample, sP, eP, exeState);
        /*showCTWithTYpe("----Final Decision----", bestDual.A, bestDual.B, temp, sample, sP, eP);
        if(debugMode || exeState == EXEST_OVERLEN){
            printf("getZoneBestMatchLoop:區段調整結果:\n");
            showCTWithTYpe("----Final Decision----", bestDual.A, bestDual.B, temp, sample, sP, eP);
            printf("sp:%d ",sP);
            printf("eP:%d\n",eP);
            system("PAUSE");
            system("cls");
        }*/
        sP = eP+1;//若不+1會無窮迴圈
    }
    return bestDual;
}

//會產生memory leak(舊的CT沒被刪除)
ctData ctDataRecoverXZFromIntA(ctData &originCT, intArray intA){
    ctData newData;
    newData.length = intA.length;
    newData.level = new int*[intA.length];
    newData.count = new int[intA.length];

    int oC = 0;
    for(int idx=0 ; idx<intA.length; idx++){
        newData.level[idx] = new int[3];
        if(intA.values[idx] == 0){
            newData.level[idx][0] = 0;
            newData.level[idx][1] = 0;
            newData.level[idx][2] = 0;
            newData.count[idx] = 0;
        }else
        {//not zero
            newData.level[idx][0] = originCT.level[oC][0];
            newData.level[idx][1] = originCT.level[oC][1];
            newData.level[idx][2] = originCT.level[oC][2];
            newData.count[idx] = originCT.count[oC];
            oC++;
        }
    }
    return newData;
}

//Compare similarity
intArray ctDataXZDistance(ctData data){
    intArray newData;

    newData.length = data.length;
    newData.values = new int[newData.length];
    for(int i=0 ; i<newData.length ; i++){
        newData.values[i] = data.level[i][0] - data.level[i][2];
    }

    return newData;
}
intArray* ctDataToIntA(ctData data){
    intArray *newData = new intArray[3];

    for(int axis=0 ; axis<3 ; axis++){
        newData[axis].values = new int[data.length];
        newData[axis].length = data.length;

        for(int idx=0 ; idx<data.length ; idx++){
            newData[axis].values[idx] = data.level[idx][axis];
        }
    }
    return newData;
}
double getCorrOfAxisWithNoZero( intArray temp, intArray sample, int times )
{
    int length = temp.length;
    double tMean=0, sMean=0;
    double tSTD=0, sSTD=0;
    double coVariance = 0;
    double corr = 0;

    int zeroC = 0;

    //求mean
    //printf("\ntemp ");
    for(int i=0 ; i<length ; i++){
        if(temp.values[i] == 0 || sample.values[i] == 0){
            zeroC++;
            continue;
        }

        tMean += temp.values[i] * times;
        sMean += sample.values[i] * times;
        //printf(" %d ",temp.values[i]);
    }
    tMean = tMean / length;
    sMean = sMean / length;

    //求STD
    //求covariance
    //printf("\nsample ");
    double tDistance=0, sDistance=0, coDistance=0;
    for(int i=0 ; i<length ; i++){
        //printf(" %d ",sample.values[i]);
        if(temp.values[i] == 0 || sample.values[i] == 0){
            continue;
        }

        int tD = temp.values[i] * times - tMean;
        int sD = sample.values[i] * times - sMean;

        /*if(tD == 0 || sD == 0){
            tD = 0;
            sD = 0;
        }*/

        tDistance += pow(tD, 2);
        sDistance += pow(sD, 2);

        coDistance += (tD * sD);
    }
    length -= zeroC;
    //printf("\n");
    if( length > 1){
        tSTD = sqrt( (double)tDistance / (length-1) );
        sSTD = sqrt( (double)sDistance / (length-1) );
        coVariance = (double)coDistance / (length-1);
    }else{
        //取得的樣本數長度只有1
        tSTD = sqrt( (double)tDistance / (length) );
        sSTD = sqrt( (double)sDistance / (length) );
        coVariance = (double)coDistance / (length );
    }
    //printf("coDistance:%.2f tDistance:%.2f sDistance:%.2f\n",coDistance, tDistance, sDistance );
    //printf("Length:%d tMean:%.2f sMean:%.2f\n",length, tMean, sMean );
    //printf("coVariance:%.2f tSTD:%.2f sSTD:%.2f\n",coVariance, tSTD, sSTD );

    //取得correlation
    corr = coVariance / (tSTD * sSTD);

    return corr;
}

void sumOfABSIntArrayABWithNoZero(intArray A, intArray B, int &aABSSum, int &bABSSum){
    aABSSum = 0;
    bABSSum = 0;

    for(int i=0; i<A.length; i++){
        if(A.values[i]!=0 && B.values[i]!=0)
        {//該點AB皆不為0才算入sum理面
            aABSSum += std::abs(A.values[i]);
            bABSSum += std::abs(B.values[i]);
        }
    }
}

double meanOfABSIntA(intArray A){
    double mean = 0;
    for(int i=0; i<A.length; i++){
        mean+=abs(A.values[i]);
    }
    mean = mean / (double)A.length;
    return mean;
}
double unMatchedPercent(intArray FA, intArray SA){
    if(FA.length != SA.length){
        printf("unMatchedPercent: 兩陣列長度不同, 無法處理, 請先補零\n");
        return 0;
    }
    double percent = 0;
    double FAM = meanOfABSIntA(FA);
    double SAM = meanOfABSIntA(SA);
    int len = FA.length;
    for(int i=0; i<len ; i++){
        if(FA.values[i] == 0 && SA.values[i] != 0)
        {//FA的值為0
            //printf("V:%d SAM:%.2f len:%d, percent: %.2f\n",SA.values[i], SAM, len, ((abs(SA.values[i])) / SAM) / len);
            percent += ((abs(SA.values[i])) / SAM) / len;

        }else if(FA.values[i] != 0 && SA.values[i] == 0)
        {//SA的值為0
            //printf("V:%d FAM:%.2f len:%d, percent: %.2f\n",FA.values[i], FAM, len, ((abs(FA.values[i])) / FAM) / len);
            percent += ((abs(FA.values[i])) / FAM) / len;
        }
    }
    return percent;
}
double calcDiffCostRatioWithNotZero(intArray A, intArray B){
    if(A.length == B.length){
        int aABSSum = 0.0;
        int bABSSum = 0.0;

        sumOfABSIntArrayABWithNoZero(A, B, aABSSum, bABSSum);

        double totalCostRatio = 0;
        for(int i=0 ; i < A.length ; i++){
            if(A.values[i]!=0 && B.values[i]!=0)
            {//AB都不是0才做
                int absDiff = std::abs(A.values[i] - B.values[i]);
                int absMaxAB   = std::max(abs(A.values[i]),abs(B.values[i]));
                double singleDiffRatio = absDiff / (double)absMaxAB;//maximum value can be 2 and minimum is 0
                double aWeight = abs(A.values[i]) / (double)aABSSum;
                double bWeight = abs(B.values[i]) / (double)bABSSum;
                double singleWeight = (aWeight + bWeight) / 2.0;
                double singleCostRatio = singleDiffRatio * singleWeight;
                totalCostRatio += singleCostRatio;
            }
        }
        totalCostRatio /= 2.0;//因為單筆diff ratio值介於0-2所以除以2
        return totalCostRatio;
    }else{
        std::cout << "calcDiffRatioWithNotZero: A.length != B.lenght invalid, return 1 (1 means totally different)" << std::endl;
        return 1;
    }
}
dualIntArray fillSpaceWithZero(dualIntArray data){
    int ALen = data.A.length;
    int BLen = data.B.length;

    if(ALen == BLen){
        return data;
    }else{
        int maxLen = max(ALen, BLen);
        intArray newIntA;
        newIntA.length = maxLen;
        newIntA.values = new int[maxLen];

        if(ALen > BLen){
        //換掉B
            for(int i=0 ; i< maxLen ; i++){
                if(i<BLen){
                    newIntA.values[i] = data.B.values[i];
                }else{
                    newIntA.values[i] = 0;
                }
            }

            data.B = newIntA;
        }else{
            //換掉A
            for(int i=0 ; i< maxLen ; i++){
                if(i<ALen){
                    newIntA.values[i] = data.A.values[i];
                }else{
                    newIntA.values[i] = 0;
                }
            }

            data.A = newIntA;
        }
    }
    return data;
}

void enhanceUnmatchPercent(double &cvalue, double &unmatchPercent){
    if(unmatchPercent > 1){
        cvalue = -1;
        unmatchPercent = 0;
    }else if(unmatchPercent > 0.5){
        //介於1~0.5 該軸的correlation值 變成負影響
        if(cvalue > 0)//原本為正變負 原本為負則不變
            cvalue *= -1;
        unmatchPercent = 1 - unmatchPercent;
    }else{
        unmatchPercent *= 2;
    }
}

double showBestMatchResult(ctData Temp, ctData Sample, bool printResult){

    intArray tempDis = ctDataXZDistance(Temp);
    intArray sampleDis = ctDataXZDistance(Sample);

    intArray *tempIntA = ctDataToIntA(Temp);
    intArray *sampleIntA = ctDataToIntA(Sample);

    if(printResult){
        printf("Data Match Detail (t, s)=>Ignored pair\n");
        printf("   |      Temp     |     Sample    |      Dist     |    Type\n");
        int idx = 0;
        int tempZeroC = 0;
        for(int Midx=0 ; Midx < Temp.length ; Midx++){
            printf("%3d", Midx);

            printf("| %6d %6d | %6d %6d |",tempIntA[0].values[idx], tempIntA[2].values[idx], sampleIntA[0].values[idx], sampleIntA[2].values[idx]);

            printf(" %6d %6d | %6d %6d", tempDis.values[idx], sampleDis.values[idx], ctDataMergeV(Temp.level[Midx][0], Temp.level[Midx][2]), ctDataMergeV(Sample.level[Midx][0], Sample.level[Midx][2]));

            if( tempIntA[0].values[idx] == 0 && tempIntA[2].values[idx] == 0)
                tempZeroC++;
            idx++;

            printf("\n");
        }
        printf("   |     Temp      |    Sample     |     Dist      |	Type\n");
    }

    //計算X跟Z各自值的總和
    int tempTotalX = 0;
    int tempTotalZ = 0;
    int sampleTotalX = 0;
    int sampleTotalZ = 0;
    for(int i=0 ; i < Temp.length ; i++){
        tempTotalX += abs(Temp.level[i][0]);
        tempTotalZ += abs(Temp.level[i][2]);
        sampleTotalX += abs(Sample.level[i][0]);
        sampleTotalZ += abs(Sample.level[i][2]);
    }
    //目前使用temp的值來計算xRatio跟zRatio
    double xRatio = (double)tempTotalX / (tempTotalX + tempTotalZ);
    double zRatio = (double)tempTotalZ / (tempTotalX + tempTotalZ);

    //double pX = getCorrOfAxisWithNoZero(tempIntA[0], sampleIntA[0], 1);
    //double pZ = getCorrOfAxisWithNoZero(tempIntA[2], sampleIntA[2], 1);
    //double pDis = getCorrOfAxisWithNoZero(tempDis, sampleDis, 1);
    double pX = 1 - calcDiffCostRatioWithNotZero(tempIntA[0], sampleIntA[0]);
    double pZ = 1 - calcDiffCostRatioWithNotZero(tempIntA[2], sampleIntA[2]);
    double pDis = 1 - calcDiffCostRatioWithNotZero(tempDis, sampleDis);

    double oX = pX;
    double oZ = pZ;
    double oDis = pDis;

    pX = (pX - 0.5) * 10.0/5;
    pZ = (pZ - 0.5) * 10.0/5;
    pDis = (pDis - 0.5) * 10.0/5;

    if(printResult){
        //printf(QObject::tr("處理前 correlation X:%.4f Z:%.4f\n").toLocal8Bit().data(), oX, oZ);
        //printf( QObject::tr("處理後corr(X:%.4f, pZ:%.4f) = 處理前(corr(X:%.4f, pZ:%.4f) - 0.5) * 10.0 / 5\n").toLocal8Bit().data(), pX, pZ, oX, oZ);
        printf( QObject::tr("處理後corr(X:%.4f, pZ:%.4f, pDis:%.4f) = 處理前(corr(oX:%.4f, oZ:%.4f, oDis:%.4f) - 0.5) * 10.0 / 5\n").toLocal8Bit().data(), pX, pZ, pDis, oX, oZ, oDis);
        //printf( QObject::tr("corr(oX:%.4f, oZ:%.4f, oDis:%.4f)\n").toLocal8Bit().data(), pX, pZ, pDis);
    }

    //px pz pdis都要介於-1~1之間
    //避免如果某一軸真的非常非常不像 因此最差可以到-1
    //if(pX < -1 || pX > 1)
    //    pX = -1;
    //if(pZ < -1 || pZ > 1)
    //    pZ = -1;

    double unmathcPX   = unMatchedPercent(tempIntA[0], sampleIntA[0]);
    double unmathcPZ   = unMatchedPercent(tempIntA[2], sampleIntA[2]);
    double unmatchDis  = (xRatio*unmathcPX + zRatio*unmathcPZ);
    double ounmatchPX  = unmathcPX;
    double ounmatchPZ  = unmathcPZ;
    double ounmatchDis = unmatchDis;

    // enhance unmatch percent
    enhanceUnmatchPercent(pX, unmathcPX);
    enhanceUnmatchPercent(pZ, unmathcPZ);
    enhanceUnmatchPercent(pDis, unmatchDis);

    pX = pX * (1 - unmathcPX);
    pZ = pZ * (1 - unmathcPZ);
    pDis = pDis * (1 - unmatchDis);

    if(printResult){
        printf("Primitive Unmatch percent: %.4f %.4f %.4f\n", ounmatchPX, ounmatchPZ, ounmatchDis);
        printf("Enhanced  Unmatch percent: %.4f %.4f %.4f\n", unmathcPX, unmathcPZ, unmatchDis);
        printf(QObject::tr("若unmatch percent > 0.5 該軸相似度直接歸0 小於0.5值double\n").toLocal8Bit().data());
        printf("Final correlation(%.4f, %.4f, %.4f) = pv * (1 - unmatch)\n", pX, pZ, pDis);
    }

    //double comResult = (pX * xRatio + pZ * zRatio);
    double comResult = (pX*xRatio + pZ*zRatio)*0.5 + (pDis)*0.5;
    if(printResult){
        printf("Temp x:%7d z:%d | Sample x:%7d z:%7d\n", tempTotalX, tempTotalZ, sampleTotalX, sampleTotalZ);
        printf( QObject::tr("目前使用Temp的x z總值比例作為pX, pZ所佔的比重(xRatio:%.4f, zRatio:%.4f)\n").toLocal8Bit().data(), xRatio, zRatio);
        printf( QObject::tr("處理後 pX:%.4f pZ:%.4f pDis:%.4f\n").toLocal8Bit().data(), pX, pZ, pDis);
        printf("Result = (pX * xRatio + pZ * zRatio)*0.5 + pDis*0.5\n");
        printf( QObject::tr("最終結果 %.4f\n").toLocal8Bit().data(), comResult);
        printf("----------------------------------------------------------\n");
    }
    return comResult;
}

//刪除沒得merge 但X跟Z軸的值卻又都小於平均值超過th倍的特徵
ctData removeNoisyFeature(ctData data, int th){
    double *mean = getABSMeanOfCTData(data);

    if(mean[0]==0 || mean[2]==0)
        return data;//zero guard

    //std::cout << "mean:" << mean[0] << ", " << mean[2] << std::endl;

    double thRatio = 1.0 / th;
    int removeCount = 0;
    for(int i=0 ; i<data.length ; i++){
        double ratioX = abs(data.level[i][0]) / mean[0];
        double ratioZ = abs(data.level[i][2]) / mean[2];

        //std::cout << "i[" << i <<"]:" << ratioX << ", " << ratioZ << std::endl;

        if( ratioX < thRatio && ratioZ < thRatio){
            data.level[i][0] = 0;
            data.level[i][1] = 0;
            data.level[i][2] = 0;
            removeCount++;
        }
    }

    if( removeCount > 0 ){
        int newLen = data.length - removeCount;
        ctData newData = getNewCTData(newLen);
        int newIdx = 0;
        for(int oldIdx=0 ; oldIdx<data.length ; oldIdx++){
            if(data.level[oldIdx][0]!=0 || data.level[oldIdx][2]!=0){
                newData.level[newIdx][0] = data.level[oldIdx][0];
                newData.level[newIdx][1] = data.level[oldIdx][1];
                newData.level[newIdx][2] = data.level[oldIdx][2];
                newIdx++;
            }
        }
        freeCT(data);
        return newData;
    }else{
        return data;
    }
}

//試用code
ctData* postProcees(intArray tempIntA, intArray sampleIntA, ctData temp, ctData sample, intArray cList){
    int **bufferT = new int*[tempIntA.length];
    int **bufferS = new int*[sampleIntA.length];
    int *countT = new int[tempIntA.length];
    int *countS = new int[sampleIntA.length];

    if(tempIntA.length != sampleIntA.length){
        printf("postProcees: temp跟sample intarray的長度不同 無法做後處理\n");
        return NULL;
    }

    intArray cTempIntA = copyIntA(tempIntA);
    intArray cSampleIntA = copyIntA(sampleIntA);

    ctData mappedTemp = ctDataRecoverXZFromIntA(temp, tempIntA);
    ctData mappedSample = ctDataRecoverXZFromIntA(sample, sampleIntA);

    int len = cSampleIntA.length;
    int sameC = 1;
    int newIdx = 0;

    //printf("len:%d ; temp.length:%d ; sample.length:%d\n",len, temp.length, sample.length);
    for(int idx=0 ; idx < len ; idx++)
    {
        bufferT[idx] = new int[3];
        bufferS[idx] = new int[3];
        bufferT[idx][0] = 0;
        bufferT[idx][1] = 0;
        bufferT[idx][2] = 0;
        countT[idx] = 0;
        bufferS[idx][0] = 0;
        bufferS[idx][1] = 0;
        bufferS[idx][2] = 0;
        countS[idx] = 0;
        cList.values[idx] = 0;

        int nonZeroItem = -1;//0->temp is not zero 1->sample
        if(idx-1 >= 0)
        {
            bool theSame = true;
            for(int cCIdx = idx-1 ; cCIdx >= idx-sameC ; cCIdx--){

                if(cTempIntA.values[idx] == 0)
                    nonZeroItem = 1;
                if(cSampleIntA.values[idx] == 0)
                    nonZeroItem = 0;

                if( ((cTempIntA.values[idx] == 0 && cSampleIntA.values[cCIdx] == 0) && !isSameType(cTempIntA.values[cCIdx], cSampleIntA.values[idx])))
                {//若對角為0 且另一對角不為同type
                    theSame = false;
                    break;
                }else if( ((cTempIntA.values[cCIdx] == 0 && cSampleIntA.values[idx] == 0) && !isSameType(cTempIntA.values[idx], cSampleIntA.values[cCIdx])) ){
                    theSame = false;
                    break;
                }


                if( (cTempIntA.values[idx]   == 0 && cTempIntA.values[cCIdx]   == 0) ||
                    (cSampleIntA.values[idx] == 0 && cSampleIntA.values[cCIdx] == 0)
                ){//單邊皆為0的獨自判斷
                    int idxType = cTempIntA.values[idx] + cSampleIntA.values[idx];
                    int cCIdxType = cTempIntA.values[cCIdx] + cSampleIntA.values[cCIdx];
                    if( !isSameType(idxType, cCIdxType) ){
                        theSame = false;
                        break;
                    }

                }else if( (!isSameType(cTempIntA.values[idx], cTempIntA.values[cCIdx]) || !isSameType(cSampleIntA.values[idx], cSampleIntA.values[cCIdx])) &&
                    (!isSameType(cTempIntA.values[idx], cSampleIntA.values[cCIdx]) || !isSameType(cSampleIntA.values[idx], cTempIntA.values[cCIdx]))
                ){//單邊皆為0不行 12 0
                  //              8 0 會誤融
                    theSame = false;
                    break;
                }
            }
            //printf("%d| theSame:%d\n", idx, theSame);

            if(theSame)
            {//可以結合
                sameC++;
            }else
            {//不能結合
                bool isPassed = false;
                //測試是否能往前跨越融合
                if(nonZeroItem == 0)
                {//該點temp is not zero and sample is zero ; 但前面的bufferT不確定
                    //往前檢查 (前面的已經被融合到buffer裡了)
                    for(int crIdx = newIdx ; crIdx>=0 ; crIdx--){
                        if(    isSameType( ctDataMergeV(bufferT[crIdx][0], bufferT[crIdx][2]), cTempIntA.values[idx])
                            && isSameType( ctDataMergeV(bufferS[crIdx][0], bufferS[crIdx][2]), cTempIntA.values[idx])//避免對角為0 另一對角不合的問題
                        ){
                            //printf("crIdx:%d ; x:%d ; z:%d ; bufferT[crIdx]:%d %d\n", crIdx+1, mappedTemp.level[idx][0], mappedTemp.level[idx][2], bufferT[crIdx][0], bufferT[crIdx][2]);
                            bufferT[crIdx][0] += mappedTemp.level[idx][0];
                            bufferT[crIdx][1] += mappedTemp.level[idx][1];
                            bufferT[crIdx][2] += mappedTemp.level[idx][2];
                            countT[crIdx] += mappedTemp.count[idx];

                            mappedTemp.level[idx][0] = 0;
                            mappedTemp.level[idx][1] = 0;
                            mappedTemp.level[idx][2] = 0;
                            mappedTemp.count[idx] = 0;
                            cTempIntA.values[idx] = 0;
                            cList.values[idx] = crIdx;
                            isPassed = true;
                        }
                        //若前面的Temp不為0 不論相不相符 都不能再往下比(否則將會打破順序性)
                        if(bufferT[crIdx][0] != 0 || bufferT[crIdx][2] != 0){
                            break;
                        }
                    }

                }else if(nonZeroItem == 1)
                {//temp is zero and sample is not
                    //往前檢查
                    for(int crIdx = newIdx ; crIdx>=0 ; crIdx--){

                        if(    isSameType( ctDataMergeV(bufferS[crIdx][0], bufferS[crIdx][2]), cSampleIntA.values[idx])
                            && isSameType( ctDataMergeV(bufferT[crIdx][0], bufferT[crIdx][2]), cSampleIntA.values[idx])//避免對角為0 另一對角不合的問題
                        ){
                            //printf("crIdx:%d newIdx:%d ; idx:%d; x:%d ; z:%d ; bufferS[crIdx]:%d %d\n", crIdx+1, newIdx+1, idx, mappedSample.level[idx][0], mappedSample.level[idx][2], bufferS[crIdx][0], bufferS[crIdx][2]);
                            bufferS[crIdx][0] += mappedSample.level[idx][0];
                            bufferS[crIdx][1] += mappedSample.level[idx][1];
                            bufferS[crIdx][2] += mappedSample.level[idx][2];
                            countS[crIdx] += mappedSample.count[idx];

                            //清空 ; 因為在這個idx的temp跟sample一個融進之前的波 一個歸0 所以會被後面的波相融, 不能讓值被重複加總
                            mappedSample.level[idx][0] = 0;
                            mappedSample.level[idx][1] = 0;
                            mappedSample.level[idx][2] = 0;
                            mappedSample.count[idx] = 0;
                            cSampleIntA.values[idx] = 0;
                            cList.values[idx] = crIdx;
                            isPassed = true;
                        }
                        //else{printf("crIdx:%d bufferS[crIdx]:%d %d\n", crIdx+1, bufferS[crIdx][0], bufferS[crIdx][2]);}
                        //若前面的Sample不為0 不論相不相符 都不能再往下比(否則將會打破順序性)
                        if(bufferS[crIdx][0] != 0 || bufferS[crIdx][2] != 0){
                            break;
                        }
                    }
                }

                //往前檢查沒通過
                //	則往後檢查(後面的都還沒動過)
                if((!isPassed) && (cTempIntA.values[idx] > 0 && cSampleIntA.values[idx] == 0))
                {//temp不為0 sample為0的狀況
                    for(int crIdx = idx+1 ; crIdx<len ; crIdx++){
                        int tX = mappedTemp.level[crIdx][0];
                        int tZ = mappedTemp.level[crIdx][2];
                        int sX = mappedSample.level[crIdx][0];
                        int sZ = mappedSample.level[crIdx][2];

                        if(    isSameType( cTempIntA.values[crIdx], cTempIntA.values[idx])
                            && isSameType( cSampleIntA.values[crIdx], cTempIntA.values[idx])//避免對角為0 另一對角不合的問題
                        ){//比對成功
                            //printf("往後檢查通過 crIdx:%d ; Temp xz:%d %d ; mappedTemp:%d %d ; mappedSample:%d %d\n", crIdx+1, mappedTemp.level[idx][0], mappedTemp.level[idx][2], tX, tZ, sX, sZ);
                            mappedTemp.level[crIdx][0] += mappedTemp.level[idx][0];
                            mappedTemp.level[crIdx][1] += mappedTemp.level[idx][1];
                            mappedTemp.level[crIdx][2] += mappedTemp.level[idx][2];
                            mappedTemp.count[crIdx] += mappedTemp.count[idx];
                            cTempIntA.values[crIdx] = ctDataMergeV(mappedTemp.level[crIdx][0], mappedTemp.level[crIdx][2]);
                            //printf("往後檢查通過 crIdx:%d ; x:%d ; z:%d ; mappedTemp:%d %d ; mappedSample:%d %d\n", crIdx+1, mappedTemp.level[idx][0], mappedTemp.level[idx][2], mappedTemp.level[crIdx][0], mappedTemp.level[crIdx][2], sX, sZ);
                            //清0
                            mappedTemp.level[idx][0] = 0;
                            mappedTemp.level[idx][1] = 0;
                            mappedTemp.level[idx][2] = 0;
                            mappedTemp.count[idx] = 0;
                            cTempIntA.values[idx] = 0;
                            cList.values[idx] = len+crIdx;
                            isPassed = true;
                        }

                        //後方的碰到不為0的temp, 不能再跨越比對了 (或已經找到配對,則不需繼續比對)
                        if(cTempIntA.values[crIdx] != 0 || isPassed){
                            break;
                        }
                    }
                }else if((!isPassed) && (cTempIntA.values[idx] == 0 && cSampleIntA.values[idx] > 0))
                {//temp為0 sample不為0的狀況
                    for(int crIdx = idx+1 ; crIdx<len ; crIdx++){
                        int tX = mappedTemp.level[crIdx][0];
                        int tZ = mappedTemp.level[crIdx][2];
                        int sX = mappedSample.level[crIdx][0];
                        int sZ = mappedSample.level[crIdx][2];

                        if(    isSameType( cSampleIntA.values[crIdx], cSampleIntA.values[idx])
                            && isSameType( cTempIntA.values[crIdx], cSampleIntA.values[idx])//避免對角為0 另一對角不合的問題
                        ){//比對成功
                            //printf("往後檢查通過 crIdx:%d ; Sample xz:%d %d ; mappedTemp:%d %d ; mappedSample:%d %d\n", crIdx+1, mappedSample.level[idx][0], mappedSample.level[idx][2], tX, tZ, sX, sZ);
                            mappedSample.level[crIdx][0] += mappedSample.level[idx][0];
                            mappedSample.level[crIdx][1] += mappedSample.level[idx][1];
                            mappedSample.level[crIdx][2] += mappedSample.level[idx][2];
                            mappedSample.count[crIdx] += mappedSample.count[idx];
                            cSampleIntA.values[crIdx] = ctDataMergeV(mappedSample.level[crIdx][0], mappedSample.level[crIdx][2]);
                            //清0
                            mappedSample.level[idx][0] = 0;
                            mappedSample.level[idx][1] = 0;
                            mappedSample.level[idx][2] = 0;
                            mappedSample.count[idx] = 0;
                            cSampleIntA.values[idx] = 0;
                            cList.values[idx] = len+crIdx;
                            isPassed = true;
                            //printf("加總後 crIdx:%d ; x:%d ; z:%d ; mappedTemp:%d %d ; mappedSample:%d %d\n", crIdx+1, mappedTemp.level[idx][0], mappedTemp.level[idx][2], mappedTemp.level[crIdx][0], mappedTemp.level[crIdx][2], mappedSample.level[crIdx][0], mappedSample.level[crIdx][2]);
                        }

                        //後方的碰到不為0的temp, 不能再跨越比對了 (或已經找到配對,則不需繼續比對)
                        if(cSampleIntA.values[crIdx] != 0 || isPassed){
                            break;
                        }
                    }
                }

                if(!isPassed){
                    sameC = 1;
                    newIdx++;
                    cList.values[idx] = newIdx;
                }else{
                    sameC++;
                }
            }
        }

        //printf("newIdx:%d ; idx:%d ; tZeroC:%d sZeroC:%d\n",newIdx, idx, tZeroC, sZeroC);
        if(cList.values[idx] == 0){
            cList.values[idx] = newIdx;
        }


        if( idx >= 0 && idx < mappedTemp.length ){
            bufferT[newIdx][0] += mappedTemp.level[idx][0];
            bufferT[newIdx][1] += mappedTemp.level[idx][1];
            bufferT[newIdx][2] += mappedTemp.level[idx][2];
            countT[newIdx] += mappedTemp.count[idx];
        }

        if( idx >= 0 && idx < mappedSample.length ){
            bufferS[newIdx][0] += mappedSample.level[idx][0];
            bufferS[newIdx][1] += mappedSample.level[idx][1];
            bufferS[newIdx][2] += mappedSample.level[idx][2];
            countS[newIdx] += mappedSample.count[idx];
        }
    }

    //放入新的ctData
    ctData *newData = new ctData[2];
    int newL = newIdx+1;
    newData[0].length = newL;
    newData[0].count = new int[newL];
    newData[0].level = new int*[newL];
    newData[1].length = newL;
    newData[1].count = new int[newL];
    newData[1].level = new int*[newL];

    for(int idx = 0; idx < newL ; idx++){
        newData[0].level[idx] = new int[3];
        newData[1].level[idx] = new int[3];
        for(int axis=0; axis<3; axis++){
            newData[0].level[idx][axis] = bufferT[idx][axis];
            newData[0].count[idx] = countT[idx];
            newData[1].level[idx][axis] = bufferS[idx][axis];
            newData[1].count[idx] = countS[idx];
        }
    }

    //處理往後融合的idx問題
    for(int i=0 ; i<len ; i++){
        while(cList.values[i] >= len){
            int cListIdx = cList.values[i] - len;
            cList.values[i] = cList.values[cListIdx];
        }
    }

    //釋放記憶體
    for(int i=0 ; i<tempIntA.length ; i++){
        delete[] bufferT[i];
        delete[] mappedTemp.level[i];
    }
    for(int i=0 ; i<sampleIntA.length ; i++){
        delete[] bufferS[i];
        delete[] mappedSample.level[i];
    }
    delete[] bufferT;
    delete[] bufferS;
    delete[] countT;
    delete[] countS;
    delete[] cTempIntA.values;
    delete[] cSampleIntA.values;
    delete[] mappedTemp.level;
    delete[] mappedSample.level;
    delete[] mappedTemp.count;
    delete[] mappedSample.count;

    return newData;
}
//試用code

dualCTData compareTwoSymbol(trajData *temp, trajData *sample){
    //處理temp
    //removeTail(*temp);
    isSameTypeLimit = 1;
    ctData ctDataTemp = sumOfPNTrajWithSign(*temp);
    //處理sample
    //removeTail(*sample);
    isSameTypeLimit = 1;
    ctData ctDataSample = sumOfPNTrajWithSign(*sample);

    //subsample(使數值變小)
    subSampleEigen(ctDataTemp, 0.1);
    subSampleEigen(ctDataSample, 0.1);

    /******* Merge連續相近特徵的原因 ******
    Merge連續相近的特徵值原因是
    常常畫一直線手稍微彎曲 造成一個特徵值被區隔成兩個特徵值
        比如說畫1 之前出現過
        Temp		Sample		 Type
        (1366, -44) (3514, -216) (13, 13)
        (3621, 215) (1167, -45)  (13, 13)
        這樣造成辨識不通過的結果
    像這樣連續 且相近的特徵值沒有區分的必要
    因為這樣的細微差距已經超出本演算法所預期的處理範圍
    故將之融合 減少不必要的誤判發生
    ********************/
    //特徵值type差距在1以內的直接融合
    //type該訂多少值得思考
    //定在2會造成很多時候圓形被過度的簡化而嚴重失真
    isSameTypeLimit = 1;
    mergeSimilarType(ctDataTemp);
    mergeSimilarType(ctDataSample);

    //刪除太小又無法merge的特徵值(XZ都小於平均5倍以上)
    ctDataTemp   = removeNoisyFeature(ctDataTemp  , 3);
    ctDataSample = removeNoisyFeature(ctDataSample, 3);

    //type差距在1以內才能match在一起
    isSameTypeLimit = 2;

    //轉換成type
    intArray tempMerge = ctDataMergeXZToIntA(ctDataTemp);
    intArray sampleMerge = ctDataMergeXZToIntA(ctDataSample);

    dualIntArray bestMatch = getZoneBestMatchLoop(tempMerge, sampleMerge, ctDataTemp, ctDataSample, false);
    bestMatch = fillSpaceWithZero(bestMatch);


    /*************************************
     *  引用去年所寫的post process(雖然寫的很爛 但是重寫很麻煩)
     *  僅僅先拿來試用 若有問題再丟棄不用
     * ***********************************/
    /*intArray combineList;
    combineList.length = bestMatch.A.length;
    combineList.values = new int[bestMatch.A.length];

    isSameTypeLimit = 1;
    ctData *processedPair = postProcees(bestMatch.A, bestMatch.B, ctDataTemp, ctDataSample, combineList);

    ctDataTemp = processedPair[0];
    ctDataSample = processedPair[1];*/
    //最終決定不用Post process
    //  原因: 在前面removeNoisyFeature之後
    //       Match的結果都不錯
    //       而且postprocess當初寫的時候 有奇怪的融合
    //       明明isSameTypeLimit設2 可是有時候差3也會融合
    //       這是不用的主因
    ////////////////////////////////////////////////////////////

    //會有memory leak
    //如果試用的code 拿掉不用 就需要這段
    ctDataTemp   = ctDataRecoverXZFromIntA(ctDataTemp, bestMatch.A);
    ctDataSample = ctDataRecoverXZFromIntA(ctDataSample, bestMatch.B);

    //融合
    //printf("mergeContinuousSimilar\n");
    /*********************
    Type Diff為何設2 請資料夾"畫錯導致很像的範例"中的範例
    並不是一定要如此 請視實際需要調整
    (可以在前面mergeSimilarType時就使用2 那這邊就不太需要了)
    *********************/
    //isSameTypeLimit = 1;
    //mergeContinuousSimilar(ctDataTemp, ctDataSample);

    //印出整個對齊結果
    //similarity = showBestMatchResult(ctDataTemp, ctDataSample, printResult);

    dualCTData resultCT;
    resultCT.A = ctDataTemp;
    resultCT.B = ctDataSample;

    return resultCT;
}

//皆有數字為0
//-1 => (x1,y1)為empty
//-2  => (x2,y2)為empty
//-3 => 皆empty
int emptySide(int x1, int y1, int x2, int y2){
    int result = 0;
    if(x1 == 0 && y1 == 0){
        result-=1;
    }
    if(x2 == 0 && y2 == 0){
        result-=2;
    }
    return result;
}

int getUpperNonZeroIdx(ctData data, int idx){
    //防止idx超出範圍
    int sIdx = min(data.length, idx-1);
    for(int i=sIdx; i>=0 ; i--){
        if(data.level[i][0] != 0 || data.level[i][2] != 0)
        {
            return i;
        }
    }
    return -1;
}

int getBelowNonZeroIdx(ctData data, int idx){
    //防止idx小於0
    int sIdx = max(0, idx+1);
    for(int i=sIdx ; i<data.length ; i++){
        if(data.level[i][0] != 0 || data.level[i][2] != 0){
            return i;
        }
    }
    return -1;
}

bool mergePointWithNeighbor(ctData &targetSide, ctData emptySide, int idx){
    int x=0, y=1, z=2;

    //先確定 target side 要被處理的特徵值之主軸是否符合融何條件(融合後是否有效益)
    int minAxis = min(abs(targetSide.level[idx][x]), abs(targetSide.level[idx][z]));
    double maxAxis = (double)max(abs(targetSide.level[idx][x]), abs(targetSide.level[idx][z]));
    double ratio = minAxis / maxAxis;
    printf("min(%d) / max(%.0f) = %f\n", minAxis, maxAxis, ratio);
    //確定主軸與次要軸的比例是否划算
    /*if(ratio >= 0.5){
        return false;
    }*/
    int thisType = ctDataMergeV(targetSide.level[idx][x], targetSide.level[idx][z]);

    //找上下鄰近的特徵 看可否融合
    int idxBuffer[2] = {getUpperNonZeroIdx(targetSide, idx),
                        getBelowNonZeroIdx(targetSide, idx)};
    printf("upidx:%d downidx:%d\n", idxBuffer[0], idxBuffer[1]);
    for(int i=0 ; i<2 ; i++){
        int useIdx = idxBuffer[i];
        //確定useIdx不是-1
        if( useIdx == -1){
            continue;
        }
        //看type能否融合
        int useType = ctDataMergeV(targetSide.level[useIdx][x], targetSide.level[useIdx][z]);
        printf("isSameType:%d, thistype:%d, usetype:%d\n", isSameTypeLimit, thisType, useType);
        if( !isSameType(thisType, useType) ){
            continue;//不能融合
        }
        //看看cost是否合理
        /*int originXCost = emptySide.level[useIdx][x] - targetSide.level[useIdx][x];
        int originZCost = emptySide.level[useIdx][z] - targetSide.level[useIdx][z];
        int newXCost    = originXCost - targetSide.level[idx][x];
        int newZCost    = originZCost - targetSide.level[idx][z];
        double xIncreace = abs(newXCost / originXCost);
        double zIncreace = abs(newZCost / originZCost);
        if(xIncreace>=0.5 || zIncreace>=0.5)
        {//融合*/
            targetSide.level[useIdx][x] += targetSide.level[idx][x];
            targetSide.level[useIdx][z] += targetSide.level[idx][z];
            targetSide.level[idx][x] = 0;
            targetSide.level[idx][z] = 0;
            printf("融合\n");
            return true;
        //}
    }
    return false;
}

void mergeContinuousSimilar(ctData &Temp, ctData &Sample){
    int x=0, y=1, z=2;
    //尋過每一個特徵
    if(Temp.length != Sample.length){
        printf("mergeContinuousSimilar: error, temp and sample has different length.\n");
        system("PAUSE");
    }

    int len = Temp.length;
    for(int i=0 ; i < len ; i++){
        //確定是否另一邊
        int emptyFlag = emptySide(Temp.level[i][0], Temp.level[i][2], Sample.level[i][0], Sample.level[i][2]);
        //printf("i:%d ", i);
        //Temp empty
        if(emptyFlag == EMP_X1_EMPTY){
            //printf("Temp empty ");
            mergePointWithNeighbor(Sample, Temp, i);
        }

        //Sample empty
        if(emptyFlag == EMP_X2_EMPTY){
            //printf("Sample empty ");
            mergePointWithNeighbor(Temp, Sample, i);
        }
        //printf("\n");
    }
}

void subSampleEigen(ctData &data, double sampleRate){
    for(int i=0; i<data.length ; i++){
        data.level[i][0] *= sampleRate;
        data.level[i][2] *= sampleRate;
    }
}

void mergeSimilarType(ctData &data){
    int lastType = ctDataMergeV(data.level[0][0], data.level[0][2]);
    int lastIdx = 0;
    for(int i=1 ; i<data.length ; i++){
        int thisType = ctDataMergeV(data.level[i][0], data.level[i][2]);
        if( isSameType(thisType, lastType) ){
            //printf("X:%d = %d + %d\n", data.level[lastIdx][0]+data.level[i][0], data.level[lastIdx][0], data.level[i][0]);
            //printf("Z:%d = %d + %d\n", data.level[lastIdx][2]+data.level[i][2], data.level[lastIdx][2], data.level[i][2]);

            data.level[lastIdx][0] += data.level[i][0];
            data.level[lastIdx][2] += data.level[i][2];

            data.level[i][0] = 0;
            data.level[i][2] = 0;
            lastType = ctDataMergeV(data.level[lastIdx][0], data.level[lastIdx][2]);
        }else{
            lastType = thisType;
            lastIdx = i;
        }
    }

    //因為前面把部分特徵值融合了
    //此處要把後面的特徵往前補滿
    //不能留下任何0的空位
    int newIdx = -1;
    for(int i=0 ; i<data.length ; i++){
        if(data.level[i][0] != 0 || data.level[i][2] != 0)
        {//不是空格
            newIdx++;
            data.level[newIdx][0] = data.level[i][0];
            data.level[newIdx][2] = data.level[i][2];
        }
    }
    data.length = newIdx+1;//長度是最後一個idx+1
}
