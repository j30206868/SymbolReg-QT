#include "symrecorder.h"

//local used function
//讀入user輸入的trajData
//trajData處理
int getAcclLevel( int g )
{
    int lv = getRound(g / 327.0);
    //printf("g=%d ; level=%d\n", g, lv);
    return lv;
}
int getGyroLevel( int g )
{
    int lv = getRound(g / 10.0);
    //printf("g=%d ; level=%d\n", g, lv);
    return lv;
}
bool recordNewSingleAccl(trajData *data, int *accl)
{
    int idx = data->length;
    int Pthreshold = 4;
    int Nthreshold = -4;
    int belowThCount = 0;

    for(int i=0 ; i < 3 ; i++){
        data->level[idx][i] = getAcclLevel( accl[i] );
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
bool recordNewSingleGyro(trajData *data, int *gyro, bool subSampleToLevel = false)
{
    int idx = data->length;
    int Pthreshold = 40;
    int Nthreshold = -40;
    int belowThCount = 0;

    for(int i=0 ; i < 3 ; i++){
        if(subSampleToLevel){
            data->level[idx][i] = getGyroLevel( gyro[i] );
        }else{
            data->level[idx][i] = gyro[i];
        }
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

//********************************************************//
//						 SymbolRecorder
//********************************************************//
//private
void SymbolRecorder::startRecordNewSymbol(){
    isRecording = true;
    //init traj array
    data->length = 0;
    printf("Start Record Symbol %d.\n", recordCount);
}
void SymbolRecorder::stopRecordSymbol(){
    isRecording = false;
    isRecordStarted = false;
    recordCount++;
    printf("Stop Record Symbol %d.\n", recordCount);
}
bool SymbolRecorder::recordAcclSymbol(std::string fname, int *accl){
    //if it is the first record, return true
    bool isFirstRecord = false;
    if( isRecordStarted==false && isRecording==true ){ //isRecordStarted==false && isRecording==true

        startRecordNewSymbol();
        //開始讀之前先清空將要寫入的檔案
        cleanFile(fname);
        isFirstRecord = true;
        isRecordStarted = true;
    }

    if( recordNewSingleAccl(data, accl) ){
        //將最新的一筆traj寫入檔案
        writeLastTrajToFile(fname, *data);
    }
    return isFirstRecord;
}
bool SymbolRecorder::recordGyroSymbol(std::string fname, int *gyro){
    //if it is the first record, return true
    bool isFirstRecord = false;
    if( isRecordStarted==false && isRecording==true ){ //isRecordStarted==false && isRecording==true

        startRecordNewSymbol();
        //開始讀之前先清空將要寫入的檔案
        cleanFile(fname);
        isFirstRecord = true;
        isRecordStarted = true;
    }

    if( recordNewSingleGyro(data, gyro) ){
        //將最新的一筆traj寫入檔案
        writeLastTrajToFile(fname, *data);
    }
    return isFirstRecord;
}
//public
SymbolRecorder::SymbolRecorder(){
    isRecording = false;
    isRecordStarted = false;
    data = new trajData(dataBufferSize);
    data->length = 0;
    recordCount = SYMRECSTARTIDX;
}
int SymbolRecorder::acclSymbolRecordToggle(short ctrlKeyState, std::string fname, int *accl){
    //update key state
    isRecording = (( 1 << 16 ) & ctrlKeyState);
    if( !isRecording )
    {// state = idle, don't do anything
        if( isRecordStarted ){
            //still recording, toggle it down
            stopRecordSymbol();
            return SYMREC_TOGGLEDOFF;
        }else{
            //do nothing since control key isn't pressed
            return SYMREC_DONOTHING;
        }
    }
    //is toggled on, record Symbol
    return recordAcclSymbol(fname, accl);
}
int SymbolRecorder::gyroSymbolRecordToggle(bool trigger, std::string fname, int *gyro){
    //update key state
    isRecording = trigger;
    if( !isRecording )
    {// state = idle, don't do anything
        if( isRecordStarted ){
            //still recording, toggle it down
            stopRecordSymbol(); // will inc record count(symbol count)

            return SYMREC_TOGGLEDOFF;
        }else{
            //do nothing since control key isn't pressed
            return SYMREC_DONOTHING;
        }
    }
    //is toggled on, record Symbol
    return recordGyroSymbol(fname, gyro);
}
//recSymInSeqNum return the flag that received from symbolRecordToggle
int SymbolRecorder::recAcclSymInSeqNum(short ctrlKeyState, std::string filepath, int *accl){
    return acclSymbolRecordToggle( ctrlKeyState, filepath, accl );
}
int SymbolRecorder::recGyroSymInSeqNum(bool trigger, std::string filepath, int *gyro){
    return gyroSymbolRecordToggle( trigger, filepath, gyro );
}
trajData *SymbolRecorder::getDataCopy(){
    trajData *newData = new trajData(data->length);
    newData->length = data->length;
    for(int i=0; i<data->length; i++){
        newData->level[i][0] = data->level[i][0];
        newData->level[i][1] = data->level[i][1];
        newData->level[i][2] = data->level[i][2];
    }
    return newData;
}

int SymbolRecorder::getRecordCount(){
    return this->recordCount;
}
