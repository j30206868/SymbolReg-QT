#include "SymbolReg.h"
#include <math.h>
#include <cstdlib>

#include "mousectrl.h"

#include "iostream"

#include <QObject>

using namespace std;

int getDegreeDistance(int d1, int d2){
    int big   = max(d1, d2);
    int small = min(d1, d2);
    int distance = big - small;
    if(distance > 180){
        distance = 360 - distance;
    }
    return distance;
}

bool isSameType(int tempType, int sampleType)
{//用於ctData merge x跟z軸後的data
    if(tempType==-1 || sampleType==-1){
        return true;//type -1 是dummy 等於其他任何type
    }

    int distance = getDegreeDistance(tempType, sampleType);

    /*  dis <= N 若N大於1 要調整後面的部分 因為整個圓圈的設計14 15 16 後面是接 1 2 3 4*/
    if( isSameTypeLimit == 1 ){
        if(distance < 33.75)
            return true;
    }else if( isSameTypeLimit == 2 ){
        if(distance < 45)
            return true;
    }else if( isSameTypeLimit == 3 ){
        if(distance < 67.5)
            return true;
    }else if( isSameTypeLimit == 0 ){
        if(distance < 11.25)
            return true;
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

ctData sumOfPNTrajWithContinueTypeV(trajData &data)
{//將正負波各自加總
    int buffer[2000][3] = {0};
    int cCount[2000] = {0};
    int idx = 0;

    int lastType = 0;
    int newType = 0;

    for(int i=0 ; i<data.length ; i++){
        buffer[i][0] = 0;
        buffer[i][1] = 0;
        buffer[i][2] = 0;
        cCount[i] = 0;

        newType = ctDataMergeV(data.level[i][0], data.level[i][2]);
        //printf("LastType:%d newType:%d ; data:%d %d ; buffer:%d %d ; idx:%d\n", lastType, newType, data.level[i][0], data.level[i][2], buffer[idx][0], buffer[idx][2], idx);
        //system("PAUSE");

        bool isSame = true;
        if(!isSameType(newType, lastType))
        //if( newType != lastType )
        {//不同type
            idx++;
            lastType = newType;
            isSame = false;
        }
        //else{lastType = newType;}

        for(int sIdx = 0; sIdx < 3 ;sIdx++){
            buffer[idx][sIdx] += data.level[i][sIdx];
        }
        cCount[idx]++;
        if(isSame)
            lastType = ctDataMergeV(buffer[idx][0], buffer[idx][2]);
    }

    //將buffer跟cCount的資料放入ctData
    int length = idx+1;
    ctData newData;
    newData.level = new int*[length];
    newData.count = new int[length];
    newData.length = length;

    for(int i=0 ; i<length ; i++){
        newData.level[i] = new int[3];
        newData.count[i] = cCount[i];
        for(int axis=0; axis<3; axis++){
            newData.level[i][axis] = buffer[i][axis];
        }
    }

    return newData;
}
int ctDataMergeVNormal(int x, int y){
    if(x == 0 && y == 0){
        return -1;//不合法輸入
    }
    int degree = getRound(atan2(y, x) * 57.2957);
    if(degree < 0){
        degree = 360 - (0 - degree);
    }
    return degree;
}
int ctDataMergeV(int v1, int v2){
    if(v1 == 0 && v2 == 0){
        return -1;//不合法輸入
    }
    int degree = getRound(atan2(v1, v2*-1) * 57.2957);
    if(degree < 0){
        degree = 360 - (0 - degree);
    }
    return degree;
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
        if(tempIntA.values[i] == -1 || sampleIntA.values[i] == -1){
            if(tempIntA.values[i] == -1){
                tempZC++;
            }
            if(sampleIntA.values[i] == -1){
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
        }else if(tempIntA.values[i]!=-1 && sampleIntA.values[i]!=-1)
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
intArray insertDummy(intArray A, int insertPos){
    intArray newArray;
    newArray.length = A.length + 1;
    newArray.values = new int[newArray.length];
    int oldIdx = 0;

    for(int i=0 ; i<newArray.length ; i++){
        if(i == insertPos){
            newArray.values[i] = -1;
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

    eP+=1;

    for(int idx=0 ; idx < maxL ; idx++)
    {
        //還沒道start point只要計算zero count就好
        if(idx < sP){
            if(tempIntA.values[idx] == -1)
                tempZC++;
            if(sampleIntA.values[idx] == -1)
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
            if(tempIntA.values[idx] == -1){
                tempZC++;
            }else{
                valueTX = temp.level[ idx-tempZC ][0];
                valueTZ = temp.level[ idx-tempZC ][2];
            }
        }

        if(idx < sampleIntA.length)
        {
            if(sampleIntA.values[idx] == -1){
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
        if(i < tempIntA.length && tempIntA.values[i] == -1){
            tempZC++;
        }else{//非0找到才assign值 否則直接照預設值給0
            if(i < tempIntA.length &&
               tP == i)
            {//找到t的point了
                tXV = temp.level[ i-tempZC ][0];
                tZV = temp.level[ i-tempZC ][2];
            }
        }

        if(i < sampleIntA.length && sampleIntA.values[i] == -1){
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
        intArray newTemp   = insertDummy(tempIntA, spIdx);
        intArray newSample = insertDummy(sampleIntA, spIdx);

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
        intArray newTemp = insertDummy(tempIntA, spIdx);
        intArray newSample = insertDummy(sampleIntA, spIdx);

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

        if(intA.values[idx] == -1){
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
    //freeCT(originCT);
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
double getCorrOfAxisWithNoZero( intArray temp, intArray sample, double times )
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
    /*if( length > 1){
        tSTD = sqrt( (double)tDistance / (length-1) );
        sSTD = sqrt( (double)sDistance / (length-1) );
        coVariance = (double)coDistance / (length-1);
    }else{*/
        //取得的樣本數長度只有1
        tSTD = sqrt( (double)tDistance / (length) );
        sSTD = sqrt( (double)sDistance / (length) );
        coVariance = (double)coDistance / (length );
    //}
    printf("coDistance:%.2f tDistance:%.2f sDistance:%.2f\n",coDistance, tDistance, sDistance );
    printf("Length:%d tMean:%.2f sMean:%.2f\n",length, tMean, sMean );
    printf("coVariance:%.2f tSTD:%.2f sSTD:%.2f\n",coVariance, tSTD, sSTD );

    //取得correlation
    double denominator = (tSTD * sSTD);
    if(denominator != 0){
        corr = coVariance / denominator;
    }else{
        corr = 0;
    }
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
dualIntArray fillSpaceWithDummy(dualIntArray data){
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
                    newIntA.values[i] = -1;
                }
            }
            //釋放舊的B
            cleanIntA(data.B);
            data.B = newIntA;
        }else{
            //換掉A
            for(int i=0 ; i< maxLen ; i++){
                if(i<ALen){
                    newIntA.values[i] = data.A.values[i];
                }else{
                    newIntA.values[i] = -1;
                }
            }
            //釋放舊的A
            cleanIntA(data.A);
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
    double zRatio = 1 - xRatio;

    //後處理前計算unmatch
    intArray bpTempDis   = ctDataXZDistance(Temp);
    intArray bpSampleDis = ctDataXZDistance(Sample);

    intArray *bpTempIntA   = ctDataToIntA(Temp);
    intArray *bpSampleIntA = ctDataToIntA(Sample);

    double ounmatchPX   = unMatchedPercent(bpTempIntA[0], bpSampleIntA[0]);
    double ounmatchPZ   = unMatchedPercent(bpTempIntA[2], bpSampleIntA[2]);
    double ounmatchDis  = (xRatio*ounmatchPX + zRatio*ounmatchPZ);

    //後處理
    //rematch(該被match在一起沒有match的,或是較相似應該被match在一起卻被錯誤match到旁邊其他特徵值的)
    //處理直線跟圓滑曲線的問題
    isSameTypeLimit = 2;
    mergeContinuousSimilar(Temp, Sample);
    //dematch(特徵值差距太大的不能match在一起)
    //resultCT = dematch(resultCT.A, resultCT.B);

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



    /********** XZRatio的前置比對(防止直線斜線橫線的mismatsh) **************/
    double sampleXRatio = (double)sampleTotalX / (sampleTotalX + sampleTotalZ);
    double ratioDiff    = fabs( (sampleXRatio - xRatio) );
    if(ratioDiff > 0.4)
    {//大於0.5相似度直接設-1
        if(printResult)
            std::cout << "XZRatio difference is higher than 0.4." << std::endl;
        return -1;
    }

    //double pX;
    //double pZ;
    //double pDis = getCorrOfAxisWithNoZero(tempDis, sampleDis, 1);
    double pX   = 1 - calcDiffCostRatioWithNotZero(tempIntA[0], sampleIntA[0]);
    double pZ   = 1 - calcDiffCostRatioWithNotZero(tempIntA[2], sampleIntA[2]);
    double pDis = 1 - calcDiffCostRatioWithNotZero(tempDis, sampleDis);
    /*if(tempIntA->length == 1){
        pX = pDis;
        pZ = pDis;
    }else{
        pX = getCorrOfAxisWithNoZero(tempIntA[0], sampleIntA[0], 1);
        pZ = getCorrOfAxisWithNoZero(tempIntA[2], sampleIntA[2], 01);
    }*/

    double oX = pX;
    double oZ = pZ;
    double oDis = pDis;

    //pX = (pX - 0.5) * 10.0/5;
    //pZ = (pZ - 0.5) * 10.0/5;
    //pDis = (pDis - 0.5) * 10.0/5;

    if(printResult){
        //printf(QObject::tr("處理前 correlation X:%.4f Z:%.4f\n").toLocal8Bit().data(), oX, oZ);
        //printf( QObject::tr("處理後corr(X:%.4f, pZ:%.4f) = 處理前(corr(X:%.4f, pZ:%.4f) - 0.5) * 10.0 / 5\n").toLocal8Bit().data(), pX, pZ, oX, oZ);
        printf( QObject::tr("處理後corr(X:%.4f, pZ:%.4f, pDis:%.4f) = 處理前(corr(oX:%.4f, oZ:%.4f, oDis:%.4f) - 0.5) * 10.0 / 5\n").toLocal8Bit().data(), pX, pZ, pDis, oX, oZ, oDis);
        //printf( QObject::tr("corr(oX:%.4f, oZ:%.4f, oDis:%.4f)\n").toLocal8Bit().data(), pX, pZ, pDis);
    }

    //px pz pdis都要介於-1~1之間
    //避免如果某一軸真的非常非常不像 因此最差可以到-1
    double unmathcPX   = (unMatchedPercent(tempIntA[0], sampleIntA[0]) + ounmatchPX) / 2.0;
    double unmathcPZ   = (unMatchedPercent(tempIntA[2], sampleIntA[2]) + ounmatchPZ) / 2.0;
    double unmatchDis  = ((xRatio*unmathcPX + zRatio*unmathcPZ) + ounmatchDis) / 2.0;

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

//皆有數字為0
//-1 => (x1,y1)為empty
//-2  => (x2,y2)為empty
//-3 => 皆empty
int checkEmptySide(int x1, int y1, int x2, int y2){
    int result = 0;
    if(x1 == 0 && y1 == 0){
        result-=1;
    }
    if(x2 == 0 && y2 == 0){
        result-=2;
    }
    return result;
}
int getUpperNonZeroIdx(ctData targetSide, ctData emptySide, int limitLastIdx, int idx){
    //防止idx超出範圍
    int sIdx = min(targetSide.length, idx-1);
    for(int i=sIdx; i>=0 ; i--){
        //limitLastIdx表示該idx融合過了,所以改idx以前的都不找了
        if(i <= limitLastIdx)
            break;
        //確定要merge的地方, temp跟sample是有match到的才能融合
        int emptyFlag = checkEmptySide(targetSide.level[i][0], targetSide.level[i][2], emptySide.level[i][0], emptySide.level[i][2]);
        if(emptyFlag == EMP_NO_EMPTY){
            return i;//兩邊都有值
        }else if(emptyFlag != EMP_BOTH_EMPTY){
            break;//若一邊有值一邊為0就不用再找了
        }
    }
    return -1;
}
int getBelowNonZeroIdx(ctData targetSide, ctData emptySide, int limitLastIdx, int idx){
    //防止idx小於0
    int sIdx = max(0, idx+1);

    for(int i=sIdx ; i<targetSide.length ; i++){
        //limitLastIdx表示該idx融合過了,所以改idx以前的都不找了
        if(i <= limitLastIdx)
            break;
        //確定要merge的地方, temp跟sample是有match到的才能融合
        int emptyFlag = checkEmptySide(targetSide.level[i][0], targetSide.level[i][2], emptySide.level[i][0], emptySide.level[i][2]);
        if(emptyFlag == EMP_NO_EMPTY){
            return i;//兩邊都有值
        }else if(emptyFlag != EMP_BOTH_EMPTY){
            break;//若一邊有值一邊為0就不用再找了
        }
    }
    return -1;
}
bool mergePointWithNeighbor(ctData &targetSide, ctData emptySide, int limitLastIdx, int &idx){//如果merge成功idx會被改變
    int x=0, y=1, z=2;

    int thisType = ctDataMergeV(targetSide.level[idx][x], targetSide.level[idx][z]);

    //找上下鄰近的特徵 看可否融合
    int idxBuffer[2] = {getUpperNonZeroIdx(targetSide, emptySide, limitLastIdx, idx),
                        getBelowNonZeroIdx(targetSide, emptySide, limitLastIdx, idx)};
    //printf("upidx:%d downidx:%d\n", idxBuffer[0], idxBuffer[1]);
    for(int i=0 ; i<2 ; i++){
        int useIdx = idxBuffer[i];
        //確定useIdx不是-1
        if( useIdx == -1){
            continue;
        }
        //看type能否融合
        int useType = ctDataMergeV(emptySide.level[useIdx][x], emptySide.level[useIdx][z]);
        //printf("isSameType:%d, thistype:%d, usetype:%d\n", isSameTypeLimit, thisType, useType);
        if( !isSameType(thisType, useType) ){
            continue;//不能融合
        }

        targetSide.level[useIdx][x] += targetSide.level[idx][x];
        targetSide.level[useIdx][z] += targetSide.level[idx][z];
        targetSide.level[idx][x] = 0;
        targetSide.level[idx][z] = 0;
        idx = useIdx;//將指到的idx改到merge完的地方
        //printf("融合\n");
        return true;

    }
    return false;
}
void mergeContinuousSimilar(ctData &Temp, ctData &Sample){
    //尋過每一個特徵
    if(Temp.length != Sample.length){
        printf("mergeContinuousSimilar: error, temp and sample has different length.\n");
        system("PAUSE");
    }

    int len = Temp.length;
    //最多只能融合到lastIdx指到的下一個特徵(用來限制 不能多次融合)
    int lastTIdx = -1;
    int lastSIdx = -1;
    for(int i=0 ; i < len ; i++){
        //確定是否另一邊
        int emptyFlag = checkEmptySide(Temp.level[i][0], Temp.level[i][2], Sample.level[i][0], Sample.level[i][2]);
        //printf("i:%d ", i);
        //Temp empty
        bool isMerged = false;
        if(emptyFlag == EMP_X1_EMPTY){
            //printf("Temp empty ");
            isMerged = mergePointWithNeighbor(Sample, Temp, lastSIdx, i);
            if(isMerged)
                lastSIdx = i;

        }

        //Sample empty
        if(emptyFlag == EMP_X2_EMPTY){
            //printf("Sample empty ");
            isMerged = mergePointWithNeighbor(Temp, Sample, lastTIdx, i);
            if(isMerged)
                lastTIdx = i;
        }
        //printf("\n");
    }
}
//dematch處理
double singleCost(int tX, int tZ, int sX, int sZ){

    int totalT = abs(tX) + abs(tZ);
    int totalS = abs(sX) + abs(sZ);

    double tXPercent = abs(tX) / (double)totalT;
    double sXPercent = abs(sX) / (double)totalS;

    double fXRatio = (tXPercent + sXPercent) / 2.0;
    double fZRatio = 1 - fXRatio;

    double xCost = (double)abs(tX - sX) / abs(max(tX, sX));
    double zCost = (double)abs(tZ - sZ) / abs(max(tZ, sZ));

    if(fXRatio <= 0.25){
        std::cout << "return zCost" << std::endl;
        return zCost;
    }else if(fZRatio <= 0.25){
        std::cout << "return xCost" << std::endl;
        return xCost;
    }else{
        std::cout << "return both axis cost" << std::endl;
        return (xCost*fXRatio + zCost*fZRatio) / 2.0;
    }
    /*int totalT = abs(tX) + abs(tZ);
    int totalS = abs(sX) + abs(sZ);
    double cost = abs(totalT - totalS) / (double)abs(max(totalT, totalS));
    return cost;*/
    /*
    int totalT = abs(tX) + abs(tZ);
    int totalS = abs(sX) + abs(sZ);

    double tXPercent = abs(tX) / (double)totalT;
    double sXPercent = abs(sX) / (double)totalS;

    double fXRatio = (tXPercent + sXPercent) / 2.0;
    double fZRatio = 1 - fXRatio;

    double xCost = (double)abs(tX - sX) / abs(max(tX, sX)) * fXRatio;
    double zCost = (double)abs(tZ - sZ) / abs(max(tZ, sZ)) * fZRatio;

    return xCost + zCost;*/
}
dualCTData dematch(ctData &Temp, ctData &Sample){
    if(Temp.length != Sample.length){
        printf("dematch: error, temp and sample has different length.\n");
        system("PAUSE");
    }
    int invalidNum = 32768;
    int invalidSum = 131072;
    int buffer[Temp.length][4];
    double thRatio = 0.6;
    int len = Temp.length;
    int dematchCount = 0;
    for(int i=0 ; i < len ; i++){
        //確定是否另一邊
        int emptyFlag = checkEmptySide(Temp.level[i][0], Temp.level[i][2], Sample.level[i][0], Sample.level[i][2]);
        if(emptyFlag == EMP_NO_EMPTY){
            double cost = singleCost(Temp.level[i][0], Temp.level[i][2], Sample.level[i][0], Sample.level[i][2]);
            std::cout << "[" << i << "]cost: " << cost << std::endl;
            if(cost >= thRatio){
                buffer[dematchCount][0] = Temp.level[i][0];
                buffer[dematchCount][1] = Temp.level[i][2];
                buffer[dematchCount][2] = Sample.level[i][0];
                buffer[dematchCount][3] = Sample.level[i][2];
                Temp.level[i][0]   = invalidNum;
                Temp.level[i][2]   = invalidNum;
                Sample.level[i][0] = invalidNum;
                Sample.level[i][2] = invalidNum;
                dematchCount++;
            }
        }
    }

    if(dematchCount == 0)
    {//沒有任何dematch
        dualCTData newPair;
        newPair.A = Temp;
        newPair.B = Sample;
        return newPair;
    }

    dualCTData newPair;
    int newLen = Temp.length + dematchCount;
    newPair.A = getNewCTData(newLen);
    newPair.B = getNewCTData(newLen);
    int oldIdx    = 0;
    int bufferIdx = 0;
    for(int i=0 ; i<newLen ; i++){
        int sum = Temp.level[oldIdx][0] + Temp.level[oldIdx][2] + Sample.level[oldIdx][0] + Sample.level[oldIdx][2];
        if(sum == invalidSum)
        {//碰到剛剛dematch刪掉的地方了
            //放temp
            newPair.A.level[i][0] = buffer[bufferIdx][0];
            newPair.A.level[i][2] = buffer[bufferIdx][1];
            newPair.B.level[i][0] = 0;
            newPair.B.level[i][2] = 0;
            //放sample
            newPair.A.level[i+1][0] = 0;
            newPair.A.level[i+1][2] = 0;
            newPair.B.level[i+1][0] = buffer[bufferIdx][2];
            newPair.B.level[i+1][2] = buffer[bufferIdx][3];
            i++;//新的idx要跳兩格
            oldIdx++;
            bufferIdx++;
        }else
        {//不變
            newPair.A.level[i][0] = Temp.level[oldIdx][0];
            newPair.A.level[i][2] = Temp.level[oldIdx][2];
            newPair.B.level[i][0] = Sample.level[oldIdx][0];
            newPair.B.level[i][2] = Sample.level[oldIdx][2];
            oldIdx++;
        }
    }
    return newPair;
}
//dematch處理
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

        /*std::cout << "[" << i << "]";
        std::cout << "thisType:" <<thisType;
        std::cout << " lastType:" << lastType;
        std::cout << " | last idx value:"<<data.level[lastIdx][2] << "," << data.level[lastIdx][0];
        std::cout << " | ivalue:"<<data.level[i][2] << "," << data.level[i][0];
        std::cout << " | if is same type: "<< isSameType(thisType, lastType)<<std::endl;*/

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
//刪除沒得merge 但X跟Z軸的值卻又都小於平均值超過th倍的特徵
ctData removeNoisyFeature(ctData data, int th, int method){
    double *denominator = 0;
    if(method == SYMREG_RMNOISE_BYMEAN){
        int largest;
        denominator = getABSMeanOfCTDataWithNoZero(data, largest);
    }else if(method == SYMREG_RMNOISE_BYLARGEST){
        //denominator = getABSMeanOfCTDataWithNoZero(data);
        int largestv = getABSLargestValueInAllAxis(data);
        denominator = new double[3];
        denominator[0] = (double)largestv;
        denominator[2] = (double)largestv;
    }else if(method == SYMREG_RMNOISE_BYBOTH){
        int largest;
        denominator = getABSMeanOfCTDataWithNoZero(data, largest);
        denominator = new double[3];
        denominator[0] = ((double)largest + denominator[0])/2.0;
        denominator[2] = ((double)largest + denominator[2])/2.0;
    }
    if(denominator[0]==0 || denominator[2]==0){
        //std::cout << "zero guard" << std::endl;
        return data;//zero guard
    }

    double thRatio = 1.0 / th;
    int removeCount = 0;

    //std::cout << "th ratio: " << thRatio << std::endl;
    //std::cout << "denominator:" << denominator[0] << ", " << denominator[2] << std::endl;

    for(int i=0 ; i<data.length ; i++){
        double ratioX = abs(data.level[i][0]) / denominator[0];
        double ratioZ = abs(data.level[i][2]) / denominator[2];

        if( ratioX < thRatio && ratioZ < thRatio){
            data.level[i][0] = 0;
            data.level[i][1] = 0;
            data.level[i][2] = 0;
            removeCount++;
        }
    }
    if(denominator != 0)
        delete[] denominator;

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
ctData getQuadrantEigen(trajData *temp){
    //取得象限特徵
    isSameTypeLimit = 1;
    ctData ctDataTemp = sumOfPNTrajWithSign(*temp);
    //融合連續相似特徵
    isSameTypeLimit = 1;
    mergeSimilarType(ctDataTemp);
    //刪除太小又無法merge的特徵值(XZ都小於平均3倍以上)
    ctDataTemp   = removeNoisyFeature(ctDataTemp  , 12, SYMREG_RMNOISE_BYLARGEST);
    ctDataTemp   = removeNoisyFeature(ctDataTemp  , 3, SYMREG_RMNOISE_BYMEAN);
    //ctDataTemp   = removeNoisyFeature(ctDataTemp  , 5, SYMREG_RMNOISE_BYBOTH);
    //放入DualCT
    return ctDataTemp;
}
ctData getContinuousEigen(trajData *temp){
    //取得象限特徵
    isSameTypeLimit = 0;
    ctData ctDataTemp = sumOfPNTrajWithContinueTypeV(*temp);

    //融合連續相似特徵
    isSameTypeLimit = 0;
    mergeSimilarType(ctDataTemp);
    //isSameTypeLimit = 1;
    //mergeSimilarType(ctDataTemp);
    //刪除太小又無法merge的特徵值(XZ都小於平均5倍以上)
    ctDataTemp   = removeNoisyFeature(ctDataTemp  , 4, SYMREG_RMNOISE_BYLARGEST);
    ctDataTemp   = removeNoisyFeature(ctDataTemp  , 3, SYMREG_RMNOISE_BYMEAN);
    //ctDataTemp   = removeNoisyFeature(ctDataTemp, 3, SYMREG_RMNOISE_BYMEAN);
    //放入DualCT
    return ctDataTemp;
}
double getTempPowerOfSample(double *tMean, double *sMean){
    double xPower = tMean[0] / sMean[0];
    double zPower = tMean[2] / sMean[2];

    //double tXRatio = tMean[0] / (tMean[0] + tMean[2]);
    //double sXRatio = sMean[0] / (sMean[0] + sMean[2]);

    double totalX = tMean[0] + sMean[0];
    double totalZ = tMean[2] + sMean[2];

    double xRatio = totalX / (totalX + totalZ);
    //double xRatio  = (tXRatio + sXRatio) / 2;
    double zRatio  = 1 - xRatio;

    cout << "tMean[0]: " << tMean[0] <<endl;
    cout << "tMean[2]: " << tMean[2] <<endl;
    cout << "sMean[0]: " << sMean[0] <<endl;
    cout << "sMean[2]: " << sMean[2] <<endl;
    cout << "xRatio: " << xRatio <<endl;
    cout << "zRatio: " << zRatio <<endl;

    if(xRatio < 0.25){
        return zPower;
    }else if(zRatio < 0.25){
        return xPower;
    }else{
        return xPower * xRatio + zPower * zRatio;
    }

    //return (xPower + zPower) / 2;
}
dualCTData getBestMatchResult(dualCTData eigenPair){
    //type差距在2以內才能match在一起
    isSameTypeLimit = 2;
    //轉換成intArray
    intArray tempMerge = ctDataMergeXZToIntA(eigenPair.A);
    intArray sampleMerge = ctDataMergeXZToIntA(eigenPair.B);
    //取得bestMatch結果
    dualIntArray bestMatch = getZoneBestMatchLoop(tempMerge, sampleMerge, eigenPair.A, eigenPair.B, false);
    //補0
    bestMatch = fillSpaceWithDummy(bestMatch);

    //將結果還原成CTData
    dualCTData resultCT;
    resultCT.A = ctDataRecoverXZFromIntA(eigenPair.A, bestMatch.A);
    resultCT.B = ctDataRecoverXZFromIntA(eigenPair.B, bestMatch.B);

    return resultCT;
}
dualCTData compareTwoSymbol(trajData *temp, trajData *sample){
    dualCTData eigenPair;
    eigenPair.A = getQuadrantEigen(temp);
    eigenPair.B = getQuadrantEigen(sample);

    //計算temp(A) sample(B)的大小差距倍數
    double *tempMean    = getABSMeanOfCTData(eigenPair.A);
    double *samplepMean = getABSMeanOfCTData(eigenPair.B);
    //subsample(使數值變小)
    double power = getTempPowerOfSample(tempMean, samplepMean);
    //如果有做affine transform就不需要了
    subSampleEigen(eigenPair.A, 0.1);
    subSampleEigen(eigenPair.B, 0.1 * power);

    cout << "power: " << power << endl;

    //std::cout << QObject::tr("Temp是Sample的 ").toLocal8Bit().data() << power;
    //std::cout << QObject::tr(" 倍大").toLocal8Bit().data() << std::endl;

    dualCTData matchedPair = getBestMatchResult(eigenPair);

    freeDualCT(eigenPair);
    return matchedPair;
}

double calcContinuousSimilarity(int tLen, int sLen){
    return 1 - (abs(tLen - sLen) / (double)max(tLen, sLen));
}
double getDistanceWeight(ctData data, int eX, int eY){
    int largestV = getABSLargestValueOfBothAxis(data);
    cout<<"largestV: "<<largestV<<endl;
    return (abs(eX)+abs(eY)) / (double)largestV;
}
int getStartAndEndPosition(ctData temp, int &x2, int &y2){
    //暫存移動時的資訊
    /*double velocity1[3] = {0, 0, 0};
    int AcclZeroC1[3] = {0, 0, 0};
    int accl[3] = {0,0,0};

    MouseCtrl mc1 = MouseCtrl();
    mc1.gyroSensitivity = 250;

    int dx = 0;
    int dy = 0;

    x2 = 0;
    y2 = 0;
    for(int i=0; i<temp.length ; i++){
        mc1.moveCursor(accl, temp.level[i], velocity1, AcclZeroC1, 10, false);
        int sx = dx;
        int sy = dy;
        dx = getRound(mc1.getDx()/3.0*10);
        dy = getRound(mc1.getDy()/3.0*10);
        int ex = dx;
        int ey = dy;
        x2 += ex - sx;
        y2 += ey - sy;
    }*/
    x2 = 0;
    y2 = 0;
    for(int i=0 ; i<temp.length ; i++){
        x2-=temp.level[i][2];
        y2+=temp.level[i][0];
    }
}
double calcDistanceCost(int tX, int tZ, int sX, int sZ){
    int totalT = abs(tX) + abs(tZ);
    int totalS = abs(sX) + abs(sZ);

    double tXPercent = abs(tX) / (double)totalT;
    double sXPercent = abs(sX) / (double)totalS;

    double fXRatio = (tXPercent + sXPercent) / 2.0;
    double fZRatio = 1 - fXRatio;

    double xCost = (double)abs(tX - sX) / max(abs(tX), abs(sX)) * fXRatio;
    double zCost = (double)abs(tZ - sZ) / max(abs(tZ), abs(sZ)) * fZRatio;

    return xCost + zCost;
}
double getCESimilairty(trajData *temp, trajData *sample){
    dualCTData continuousPair;
    continuousPair.A = getContinuousEigen(temp);
    continuousPair.B = getContinuousEigen(sample);

    int tempCECount = continuousPair.A.length;
    int sampleCECount = continuousPair.B.length;
    return calcContinuousSimilarity(tempCECount, sampleCECount);
}
double calcComplexSimilarity(dualCTData quadrantPair, trajData *temp, trajData *sample, bool printResult){
    //算end跟start向量影響的權重
    int tX2, tY2, sX2, sY2;
    getStartAndEndPosition(quadrantPair.A, tX2, tY2);
    getStartAndEndPosition(quadrantPair.B, sX2, sY2);
    //算degree distance
    int tDegree = ctDataMergeVNormal(tX2, tY2);
    int sDegree = ctDataMergeVNormal(sX2, sY2);
    int degreeDis = getDegreeDistance(tDegree, sDegree);

    double tDisWeight = getDistanceWeight(quadrantPair.A, tX2, tY2);
    double sDisWeight = getDistanceWeight(quadrantPair.B, sX2, sY2);
    double distanceWeight = (tDisWeight + sDisWeight) / 2.0;

    //distance的大小 是 degree的similarity 的權重
    double SEDegSimilarity = 1 - (degreeDis / 180.0);
    //SEDegSimilarity = (SEDegSimilarity - 0.5) * 2;

    double positionDistance = 1 - calcDistanceCost(tX2,  tY2,  sX2,  sY2);

    double CESimilarity = getCESimilairty(temp, sample);
    //CEsimilarity如果很高像是0.8~1其實不需要被區別
    //但如果很低 就要明顯區別
    CESimilarity = min((CESimilarity - 0.5) * 2.5, 1.0);

    double quadrantSimilarity = showBestMatchResult(quadrantPair.A, quadrantPair.B, false);

    double maxDegSimi = 0.1;
    double maxPosSimi = 0.3;
    double degreeSimi = (SEDegSimilarity  * distanceWeight) * maxDegSimi;
    double posiSimi   =  (positionDistance * distanceWeight) * maxPosSimi;

    if(tDisWeight < 0.4 && sDisWeight < 0.4){
        degreeSimi = 0.1;
        posiSimi = 0.2;
    }

    double result = degreeSimi;
           result+= posiSimi;
           result+= CESimilarity     * 0.2;
           result+= quadrantSimilarity * 0.5;
           //result = result / 3.0;


   if(printResult){
       cout << "tDisWeight = " << tDisWeight << endl;
       cout << "sDisWeight = " << sDisWeight << endl;
       cout << "Temp   end (" << tX2 << ", " << tY2 << ")" << ctDataMergeVNormal(tX2, tY2) << endl;
       cout << "Sample end (" << sX2 << ", " << sY2 << ")" << ctDataMergeVNormal(sX2, sY2) << endl;
       cout << "positionDistance   = " << posiSimi << endl;
       cout << "degreeSimilarity(" << degreeSimi << ") = ";
       cout << " SEDegSimilarity(" << SEDegSimilarity << ") * ";
       cout << "distanceWeight(" << distanceWeight << ") * 0.2" << endl;
       cout << "CESimilarity       = " << CESimilarity << endl;
       cout << "quadrantSimilarity = " << quadrantSimilarity << endl;
       cout << "result           = " << result << endl;
   }

    return result;
}
