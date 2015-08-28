#ifndef COMPORTREADER_H
#define COMPORTREADER_H

#include <QThread>
#include "serialclass.h"
#include <string>

//全域變數宣告
const int GETAANDG_INVALID_DATA = -1;
const int GETAANDG_NEW_VALID_DATA = 0;
const int GETAANDG_READING = 1;
//加速度計與陀螺儀Raw Data處理
char* getAccelAndGyro(int *count, int *flag, int *bLen, char *buffer, int *accl, int *gyro, float *quatern, int *button, int *time, const int bSize);
char* readValue(float *value, int *bLen, char *buffer, int *result, int *countFlag);
char* readValue(int *value, int *bLen, char *buffer, int *result, int *countFlag);
void QtoGravity(float *quatern, float *gravity);
void removeGravity(int *accl, float *gravity);
int agMedianFilter(int *Accls[3], int *Gyros[3], int *accl, int *gyro, int &MFCount, int MFLen, bool applyToAccl = true, bool applyToGyro = true);
//Decode information from mpu6050
void waitUntilSerialStable(Serial* SP, char *incomingData, int dataLength);
int readSerialIntoBuffer(Serial* SP, char *incomingData, int &dataLength, int &readResult, int &bLen, int bSize, char *buffer);


class MpuReader : public QThread
{
    Q_OBJECT

public:
    MpuReader(QObject *parent = 0);
    ~MpuReader();

    void setSymSaveDir(QString dirPath);
    void setNextSymCount(int nextCount);
    void stopReading();
    void startReading();
    bool isReading();
signals:
    void updateNewSymbol();//return值 是從main那邊取得下一個檔案的編號
    void readingEnded();

protected:
    void run() Q_DECL_OVERRIDE;

private:
    //QMutex mutex;
    //QWaitCondition condition;

    //private變數
    bool stop;
    int nextSymCount;
    QString symDirPath;

};

#endif // COMPORTREADER_H

