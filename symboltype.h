#ifndef SYMBOLTYPE_H
#define SYMBOLTYPE_H

#include <QString>

struct ctData{//continuous trajectory data with count
    int **level; //[idx][axis]
    int *count;
    int *sP;
    int *eP;
    int length;
};

struct dualCTData{
    ctData A;
    ctData B;
};

struct trajData{
    int **level;//[idx][axis] //事先宣告1000很傷 很吃空間 要用的時候必須改掉
    int length;

    trajData(int size){
        this->level = new int*[size];
        this->length = size;

        for(int i=0; i<size ;i++){
            this->level[i] = new int[3];
            for(int j=0 ;j<3; j++){
                this->level[i][j] = 0;
            }
        }
    }
};

//宣告ctdata內的記憶體
void freeCT(ctData data);
void freeDualCT(dualCTData dualData);
ctData getNewCTData(int newLen);
double* getABSMeanOfCTData(ctData data);

//trajData處理
int getLevel( int g );

//讀入user輸入的trajData
bool recordNewSingleTrajData(trajData *data, int *accl);

//從檔案讀取trajdata
trajData *readTrajDataFromFile(std::string fname);
void writeLastTrajToFile(std::string fname, trajData data);

#endif // SYMBOLTYPE_H

