#ifndef SYMRECORDER_H
#define SYMRECORDER_H

#include "commonfunc.h"
#include "symboltype.h"
#include <iostream>
#include <sstream>

const int SYMRECSTARTIDX = 1;
class SymbolRecorder{
    private:
        const static int dataBufferSize = 1000;
        bool isRecording;
        bool isRecordStarted;
        int recordCount;
        trajData *data;

    //private function
        void startRecordNewSymbol();
        void stopRecordSymbol();
        //return true, if it's the first record
        // true = SYMREC_TOGGLEDON | false = SYMREC_ISRECORDING
        bool recordAcclSymbol(std::string fname, int *accl);
        bool recordGyroSymbol(std::string fname, int *gyro);

    public:
        SymbolRecorder();
        const static int SYMREC_DONOTHING = -2;
        const static int SYMREC_TOGGLEDOFF = -1;
        const static int SYMREC_ISRECORDING = 0;
        const static int SYMREC_TOGGLEDON = 1;
        int acclSymbolRecordToggle(short ctrlKeyState, std::string fname, int *accl);
        int gyroSymbolRecordToggle(bool trigger, std::string fname, int *gyro);
        //recSymInSeqNum return the flag that received from symbolRecordToggle
        int recAcclSymInSeqNum(short ctrlKeyState, std::string filepath, int *accl);
        int recGyroSymInSeqNum(bool trigger, std::string filepath, int *gyro);
        trajData *getDataCopy();
        int getRecordCount();

};

#endif // SYMRECORDER_H

