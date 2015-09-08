#include "symboltype.h"

#include "commonfunc.h"

#include <string>
#include <fstream>
#include <iostream>

#include <QObject>

using namespace std;

dualCTData compareTwoSymbol(trajData *temp, trajData *sample, double &similarity, bool printResult);

void freeCT(ctData data){
    if(data.length > 0){
        delete[] data.count;
        for(int i=0 ; i<data.length ; i++){
            delete[] data.level[i];
        }
        delete[] data.level;
        data.length = 0;
    }
}
void freeDualCT(dualCTData dualData){
    freeCT(dualData.A);
    freeCT(dualData.B);
}
ctData getNewCTData(int newLen){
    ctData newTemp;
    newTemp.length = newLen;
    newTemp.level = new int*[newLen];
    for(int i=0 ; i<newLen ; i++){
       newTemp.level[i] = new int[3];
    }
    newTemp.count = new int[newLen];
    return newTemp;
}

double* getABSMeanOfCTData(ctData data){
    double *mean = new double[3];
    mean[0] = 0;
    mean[1] = 0;
    mean[2] = 0;
    if(data.length > 0){
        for(int i=0 ; i<data.length ; i++){
            mean[0] += abs(data.level[i][0]);
            mean[1] += abs(data.level[i][1]);
            mean[2] += abs(data.level[i][2]);
        }
        mean[0] = mean[0]/data.length;
        mean[1] = mean[1]/data.length;
        mean[2] = mean[2]/data.length;
    }
    return mean;
}

double* getABSMeanOfCTDataWithNoZero(ctData data, int &largest){
    double *mean = new double[3];
    mean[0] = 0;
    mean[1] = 0;
    mean[2] = 0;

    largest = 0;

    if(data.length > 0){
        for(int axis=0 ; axis<=2 ; axis+=2){
            int zeroC = 0;
            for(int i=0 ; i<data.length ; i++){
                int absv = abs(data.level[i][axis]);
                mean[axis] += absv;
                if(absv != 0)
                    zeroC++;
                if(absv > largest)
                    largest = absv;
            }
            mean[axis] = mean[axis]/zeroC;
        }
    }
    return mean;
}
int getABSLargestValueOfBothAxis(ctData data){
    int largest = 0;

    for(int i=0 ; i<data.length ; i++){
        int absv = std::abs(data.level[i][0]) + std::abs(data.level[i][2]);
        if( absv > largest )
            largest = absv;
    }

    return largest;
}
int getABSLargestValueInAllAxis(ctData data){
    int largest = 0;
    for(int axis=0 ; axis<=2 ; axis+=2){
        for(int i=0 ; i<data.length ; i++){
            int absv = std::abs(data.level[i][axis]);
            if( absv > largest )
                largest = absv;
        }
    }
    return largest;
}
//trajData處理
int getLevel( int g )
{
    int lv = getRound(g / 327.0);
    //printf("g=%d ; level=%d\n", g, lv);
    return lv;
}
void freePTraj(trajData *data){
    if(data != 0){
        if(data->length > 0){
            for(int i=0 ; i < data->length ; i++){
                delete[] data->level[i];
            }
            delete[] data->level;
            data->length = 0;
        }
        delete data;
        data = 0;
    }
}

//讀入user輸入的trajData
bool recordNewSingleTrajData(trajData *data, int *accl)
{
    int idx = data->length;
    int Pthreshold = 4;
    int Nthreshold = -4;
    int belowThCount = 0;

    for(int i=0 ; i < 3 ; i++){
        data->level[idx][i] = getLevel( accl[i] );
        //if(abs(accl[i]) >= 300)
        //	data->level[idx][i] = accl[i];
        //else
        //	data->level[idx][i] = 0;
        //printf("accl[i]:%d level:%d\n", accl[i], data->level[idx][i]);

        if( data->level[idx][i] <= Pthreshold && data->level[idx][i] >= Nthreshold){
            belowThCount++;
        }
    }

    if( belowThCount != 3 )
    {//若三軸的值都低於門檻 判定為靜止狀態
        data->length++;
        return true;
    }else{
        return false;
    }
}

trajData *readTrajDataFromFile(std::string fname){
    //std::cout << QObject::tr("開始讀入").toLocal8Bit().data() << fname << QObject::tr("資料...\n").toLocal8Bit().data();

    //trajData *data = new trajData();
    //data.length = 0;
    int idx = 0;
    int buffer[2000][3];

    std::string line = "";
    std::ifstream fin;
    fin.open(fname.c_str(), std::ios::in);
    while( getline(fin, line) ){

        intArray result = splitAsInt(line, "\t");

        if(result.length != 3){
            continue;
        }

        for(int i=0 ; i<result.length ; i++){
            //std::cout << result.values[i] <<" "<<std::endl;
            buffer[idx][i] = result.values[i];
        }
        idx++;
    }
    fin.close();

    //std::cout << QObject::tr("總共 ").toLocal8Bit().data() << idx << QObject::tr(" 筆特徵值").toLocal8Bit().data() << std::endl;

    trajData *newData = new trajData(idx);
    for(int i=0 ;i<idx ; i++){
        newData->level[i][0] = buffer[i][0];
        newData->level[i][1] = buffer[i][1];
        newData->level[i][2] = buffer[i][2];
    }

    return newData;
}

void writeLastTrajToFile(std::string fname, trajData data){
    int idx = data.length-1;
    ofstream myfile (fname.c_str(), ios::app);
    myfile << data.level[idx][0] << "\t" << data.level[idx][1] << "\t" << data.level[idx][2] <<"\n";
    myfile.close();
}
