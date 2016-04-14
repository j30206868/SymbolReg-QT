#include "mpu6050reader.h"
#include "symrecorder.h"
#include "mousectrl.h"
#include <iostream>
#include <QString>

MpuReader::MpuReader(QObject *parent)
: QThread(parent)
{
    stop = true;
    symDirPath = "";
    setNextSymCount(0);
}

MpuReader::~MpuReader()
{
    stop = true;
    wait();//使執行緒在Stop被改為true時 會等到run那邊的迴圈結束才把執行緒destory掉
           //避免不可預期之錯誤發生
}

void MpuReader::run()
{
    //********************************************************//
    //							變數宣告
    //********************************************************//
    //Handle Serial Buffer
    char incomingData[500] = "";			// don't forget to pre-allocate memory
    int dataLength = 256;
    int readResult = 0;
    const int bSize = 10000;
    char buffer[bSize] = "";
    int bLen = 0;

    //Decode information from buffer string
    int count = 0;
    int flag = -1; //-1等於上次是不合法結束的情況
    int accl[3]; // x, y, z
    int gyro[3]; // x, y, z
    float gravity[3];
    float quatern[4];
    int buttons[2];
    int period;

    //中位值濾波
    int MFLen = 5; //濾波器長度為5
    int **Accls = allcIntDArray(MFLen, 3);
    int **Gyros = allcIntDArray(MFLen, 3);
    int MFCount = 0; // 只用於前五次, 確認filter的data set是不是被裝滿了, 裝滿了才開始濾波

    //Symbol record
    SymbolRecorder SR = SymbolRecorder();

    //滑鼠控制
    MouseCtrl mc = MouseCtrl();
    double velocity[3] = {0, 0, 0};
    int AcclZeroC[3] = {0, 0, 0};
    bool isLeftMouseBtnDown = false;
    bool isRightMouseBtnDown = false;
    bool isLeftAndRightDown = false;

    //********************************************************//
    //					連接Com port並確認連線
    //********************************************************//
    std::cout << "Arduino with GY521 board and chip MPU6050!" << std::endl;
    char *COM_NUM = "COM5";
    Serial* SP = new Serial(COM_NUM);    // adjust as needed
    if (SP->IsConnected())
        std::cout << "We're connected" << std::endl;
    //read few set of data until it's getting stable
    waitUntilSerialStable(SP, incomingData, dataLength);

    std::cout << "Data should be stable, start to read MPU6050" << std::endl;

    while(!stop){
        if(!SP->IsConnected()){
            std::cout << "Failed to read " << COM_NUM << " Mpu6050 reader is off." << std::endl;
            emit readingEnded();
            break;
        }

        //********************************************************//
        //	 從Buffer Str取得(accl[3], gyro[3], quatern[4], buttons[2], period)
        //********************************************************//
        if( !readSerialIntoBuffer(SP, incomingData, dataLength, readResult, bLen, bSize, buffer) )
            continue;//If no valid serial data are read or buffer overflow, skip this round

        //decode mpu6050 data from buffer str, take off processed part from buffer
        strcpy_s(buffer, bSize, getAccelAndGyro(&count, &flag, &bLen, buffer, accl, gyro, quatern, buttons, &period, bSize));

        //   每一組合法數值為 (acclX,acclY,acclZ,gyroX,gyroY,gyroZ,quatern1,quatern2,quatern3,quatern4,buttonRight,buttonLeft,readperiod)
        if( flag != GETAANDG_NEW_VALID_DATA )
            continue;//尚未讀到整組完整資料 不處理 繼續讀

        //********************************************************//
        //						操控滑鼠
        //********************************************************//
        bool realMove = false;
        if(mc.state == MOUSECTRL_MSSTATE)
            realMove = true;
        bool isMoved = mc.moveCursor(accl, gyro, velocity, AcclZeroC, period, realMove);

        //控制滑鼠左鍵(對應上button)
        if((buttons[1]==1) && (!isRightMouseBtnDown)){
            isRightMouseBtnDown = true;
            printf("Right btn Down!\n");
            mc.sendMouseInput(MOUSEEVENTF_RIGHTDOWN);
            //mc.sendMouseInput(MOUSEEVENTF_RIGHTUP);
        }else if((buttons[1]==0) && (isRightMouseBtnDown)){
            isRightMouseBtnDown = false;
            printf("Right btn Up!\n");
            mc.sendMouseInput(MOUSEEVENTF_RIGHTUP);
        }
        //控制滑鼠右鍵(對應下button)
        if((buttons[0]==1) && (!isLeftMouseBtnDown)){
            isLeftMouseBtnDown = true;
            printf("Left btn Down!\n");
            mc.sendMouseInput(MOUSEEVENTF_LEFTDOWN);
            //mc.sendMouseInput(MOUSEEVENTF_LEFTUP);
        }else if((buttons[0]==0) && (isLeftMouseBtnDown)){
            isLeftMouseBtnDown = false;
            printf("Left btn Up!\n");
            mc.sendMouseInput(MOUSEEVENTF_LEFTUP);
        }
        //同時click滑鼠左右鍵
        if(isRightMouseBtnDown && isLeftMouseBtnDown){
            if(!isLeftAndRightDown){
                isLeftAndRightDown = true;
                //mc state toggle
                if(mc.state == MOUSECTRL_MSSTATE){
                    mc.state = MOUSECTRL_TRAJPROC;
                }else{
                    mc.state = MOUSECTRL_MSSTATE;
                }
            }
        }else{
            if(isLeftAndRightDown){
                isLeftAndRightDown = false;
            }
        }
        //********************************************************//
        //				處理加速度計與陀螺儀Raw Data
        //********************************************************//
        //get gravity
        QtoGravity(quatern, gravity);
        //std::cout << "Gyro: " << gyro[0] << "," << gyro[1] << "," << gyro[2] << std::endl;
        //去重力
        removeGravity(accl, gravity);
        //Median Filter(中位值濾波)
        agMedianFilter(Accls, Gyros, accl, gyro, MFCount, MFLen, true, true); //是否做濾波 accl=true gyro=true

        //********************************************************//
        //						記錄Symbol
        //********************************************************//
        //單純紀錄Gyro的raw data(當然因為前面有中位值濾波 所以不是完全的raw data)
        std::stringstream sstm;
        sstm << symDirPath.toStdString() << nextSymCount << ".txt";

        //buttons[0] = false;
        //buttons[1] = false;

        int toggleFlag = SR.recGyroSymInSeqNum( buttons[0] | buttons[1] , sstm.str(), gyro);
        if(toggleFlag == SR.SYMREC_TOGGLEDOFF)
        {//當最新一個完整的符號錄製結束
            std::cout << "Got New Symbol: " << sstm.str() << std::endl;
            emit updateNewSymbol();
            nextSymCount++;
        }
    }
    SP->~Serial();

    //釋放中位值濾波使用的記憶體空間
    freeIntDArray(Accls, MFLen);
    freeIntDArray(Gyros, MFLen);
}

void MpuReader::stopReading(){
    stop = true;
}
void MpuReader::startReading(){
    stop = false;
    this->start();
}
bool MpuReader::isReading(){
    if(stop){
        return false;
    }else{
        return true;
    }
}

void MpuReader::setSymSaveDir(QString dirPath){
    symDirPath = dirPath;
    std::cout << "symDirPath updated: " << symDirPath.toLocal8Bit().data() << std::endl;
}
void MpuReader::setNextSymCount(int nextCount){
    nextSymCount = nextCount;
    std::cout << "nextSymCount updated: " << nextSymCount << std::endl;
}

//********************************************************//
//														  //
//			加速度計與陀螺儀Raw Data處理
//														  //
//********************************************************//
//flag = -1 不合法結束
//flag = 0 合法結束 新一組數值出現
//flag = 1 正在讀取
//極限了 再多讀三個數字就會炸掉 原因無從得知
char* getAccelAndGyro(int *count, int *flag, int *bLen, char *buffer, int *accl, int *gyro, float *quatern, int *button, int *time, const int bSize){
    while(true){
        int value = 0;
        float dvalue = 0.0;
        int result = 0;

        if(*count < 6)
        {//accl and gyro are integer
            strcpy_s(buffer, bSize, readValue(&value, bLen, buffer, &result, count));
        }else if(*count >= 6 && *count <= 9)
        {//gravity and quaternion are double
            strcpy_s(buffer, bSize, readValue(&dvalue, bLen, buffer, &result, count));
        }else
        {//time is integer
            strcpy_s(buffer, bSize, readValue(&value, bLen, buffer, &result, count));
        }

        if( result > 0 ){ //有value讀出
            if(result == 2){
                // end with ','
                //assign value
                if(*count < 3){//accl(int)
                    int idx = (*count);
                    accl[idx] = value;
                }else if(*count >= 3 && *count<=5){//gyro(int)
                    int idx = (*count-3);
                    gyro[idx] = value;
                }else if(*count >= 6 && *count <= 9){//Quaternion(double)
                    int idx = (*count-6);
                    quatern[idx] = dvalue;
                }else if(*count >= 10 && *count <= 11){
                    int idx = (*count-10);
                    button[idx] = value;
                }

                *flag = 1;
                *count = *count + 1;
//				printf("in count = %d\n", *count);
            }else if(result == 1){
                //end of a round
                if(*count == 12){
                    //合法結束
                    *time = value;
                    //quatern[3] = 10.0;
                    *flag = 0;
                    *count = 0;
                    return buffer;
                }else{
                    //不合法結束
                    *flag = -1;
                    *count = 0;
                    printf("getAccelAndGyro: abnormal end of data, abort.\n");
                }
            }

        }else if( result < 0){ //沒有value讀出
            //不合法情況出現
            if( result == -1 ){
                //不合法結束
                *flag = -1;
                *count = 0;
            }
        }else if(result == 0){
            return buffer;
        }
    }

    return buffer;
}
//if value assigned
//if stop at | return 1 else if stop at , return 2
//else if value not assign
//'|' return -1, ',' return -2
//if no end sign or number return 0;
char* readValue(float *value, int *bLen, char *buffer, int *result, int *countFlag){
    std::string tmp = "";
    int count = 0;
    int total = *bLen;

    // nothing
    if(total == 0){
        *result = 0;
        return buffer;
    }

    //read first
    total--;

    int id = 0;
    char c = buffer[id];
    do{
        if(c == '|'){
            *result = 1;
            break;
        }else if(c == ','){
            *result = 2;
            break;
        }else{
            //
            tmp = tmp + c;
            count++;
        }

        //read next
        total--;
        if(total <= 0){
            //no end sign, cancel reading
            *result = 0;
            return buffer;
        }

        id++;
        c = buffer[id];
    }while(true);

    if(count > 0){
        bool validate;
        *value = QString::fromStdString(tmp).toFloat(&validate);
        if(validate == false){
            std::cout<<"tmp = "<<tmp<< std::endl;
            *result = 1;
            *countFlag = 0;
            std::cout << "Exception caugth!" << std::endl;
        }
        //cut buffer and deduct bLen
        *bLen-= (id+1);

        //printf("in buffer = %.*s, bLen = %d, id+1 = %d\n", *bLen, buffer, *bLen, (id+1));

        return &buffer[id+1];
    }else if(count == 0){

        //cut buffer and deduct bLen
        *bLen-= (id+1);

        *result *= -1;
        return &buffer[id+1];
    }else{
        *result = 0;
        //error
        return buffer;
    }
}
char* readValue(int *value, int *bLen, char *buffer, int *result, int *countFlag){
    std::string tmp = "";
    int count = 0;
    int total = *bLen;

    // nothing
    if(total == 0){
        *result = 0;
        return buffer;
    }

    //read first
    total--;

    int id = 0;
    char c = buffer[id];
    do{
        if(c == '|'){
            *result = 1;
            break;
        }else if(c == ','){
            *result = 2;
            break;
        }else{
            //
            tmp = tmp + c;
            count++;
        }

        //read next
        total--;
        if(total <= 0){
            //no end sign, cancel reading
            *result = 0;
            return buffer;
        }

        id++;
        c = buffer[id];
    }while(true);

    if(count > 0){
        bool validate;
        *value = QString::fromStdString(tmp).toInt(&validate);
        if(validate == false){
            std::cout<<"tmp = "<<tmp<<std::endl;
            *result = 1;
            *countFlag = 0;
            std::cout << "Exception caugth!" << std::endl;
        }
        //cut buffer and deduct bLen
        *bLen-= (id+1);

        //printf("in buffer = %.*s, bLen = %d, id+1 = %d\n", *bLen, buffer, *bLen, (id+1));

        return &buffer[id+1];
    }else if(count == 0){

        //cut buffer and deduct bLen
        *bLen-= (id+1);

        *result *= -1;
        return &buffer[id+1];
    }else{
        *result = 0;
        //error
        return buffer;
    }
}
void QtoGravity(float *quatern, float *gravity){
    int w=0, x=1, y=2, z=3;
    gravity[0] = 2 * (quatern[x] * quatern[z] - quatern[w] * quatern[y]);
    gravity[1] = 2 * (quatern[w] * quatern[x] + quatern[y] * quatern[z]);
    gravity[2] = quatern[w] * quatern[w] - quatern[x] * quatern[x] - quatern[y] * quatern[y] + quatern[z] * quatern[z];
}
void removeGravity(int *accl, float *gravity){
    accl[0] = accl[0] - (gravity[0]*16384);
    accl[1] = accl[1] - (gravity[1]*16384);
    accl[2] = accl[2] - (gravity[2]*16384);
}
int agMedianFilter(int *Accls[3], int *Gyros[3], int *accl, int *gyro, int &MFCount, int MFLen, bool applyToAccl, bool applyToGyro){
    int MFLastIdx = MFLen - 1;
    double MFMedianLen = MFLen - 2; // 去掉頭尾(max跟min)的長度
    int x=0, y=1, z=2;
    int gMax[3] = {-32769, -32769, -32769}; //normal range from -32768 to 32767
    int gMin[3] = {32768, 32768, 32768};
    int gAll[3] = {0, 0, 0};
    int aMax[3] = {-32769, -32769, -32769}; //normal range from -32768 to 32767
    int aMin[3] = {32768, 32768, 32768};
    int aAll[3] = {0, 0, 0};

    if(MFCount < MFLastIdx){
        //紀錄最初的n-1個數值
        int idx = MFCount;
        Accls[idx][0] = accl[0];
        Accls[idx][1] = accl[1];
        Accls[idx][2] = accl[2];

        Gyros[idx][0] = gyro[0];
        Gyros[idx][1] = gyro[1];
        Gyros[idx][2] = gyro[2];

        MFCount++;
        return false;
    }else{// 0 = x , 1 = y , 2 = z

        //printf("Accl Move = %d : %d : %d | ", axMove, ayMove, azMove);
        //printf("Accl = %d : %d : %d | ", accl[0], accl[1], accl[2]);
        //printf("Accl = %d : %d : %d | ", axMove, ayMove, azMove);
        //printf("Gyro = %d : %d : %d | bLen = %d\n", gyro[0], gyro[1], gyro[2], bLen);

        //防止overflow的值爆衝
        if( (gyro[z] > 1000 || gyro[z] < -1000) &&
            (gyro[y] > 1000 || gyro[y] < -1000) &&
            (gyro[x] > 1000 || gyro[x] < -1000) ){
            //continue;
            //printf("Gyro = %d : %d : %d | bLen = %d\n", gyro[0], gyro[1], gyro[2], bLen);
            //printf("detected \n\n\n\n\n\n\n\n\n");
        }

        //中位值率波 (去頭去尾)
        //新的值都放在最後一個
        Accls[MFLastIdx][x] = accl[x];
        Accls[MFLastIdx][y] = accl[y];
        Accls[MFLastIdx][z] = accl[z];

        Gyros[MFLastIdx][x] = gyro[x];
        Gyros[MFLastIdx][y] = gyro[y];
        Gyros[MFLastIdx][z] = gyro[z];

        //去掉頭尾取平均
        for(int i=0 ; i < MFLen ; i++){
            for(int j=0 ; j<3 ; j++){

                //accl
                if( Accls[i][j] > aMax[j] ){
                    //取最大值
                    aMax[j] = Accls[i][j];
                }
                if( Accls[i][j] < aMin[j] ){
                    //取最小值
                    aMin[j] = Accls[i][j];
                }
                //3軸各自累加
                aAll[j] += Accls[i][j];

                //gyro
                if( Gyros[i][j] > gMax[j] ){
                    //取最大值
                    gMax[j] = Gyros[i][j];
                }
                if( Gyros[i][j] < gMin[j] ){
                    //取最小值
                    gMin[j] = Gyros[i][j];
                }
                //3軸各自累加
                gAll[j] += Gyros[i][j];

                //覆蓋舊的數值
                if(i > 0){
                    //第1個丟棄 第2個變成第1個 ; 第3個變成第2個 以此類推
                    Accls[i-1][j] = Accls[i][j];
                    Gyros[i-1][j] = Gyros[i][j];
                }
            }
        }

        if(applyToAccl){
            accl[x] = (int)( (aAll[x] - aMax[x] - aMin[x]) / MFMedianLen);
            accl[y] = (int)( (aAll[y] - aMax[y] - aMin[y]) / MFMedianLen);
            accl[z] = (int)( (aAll[z] - aMax[z] - aMin[z]) / MFMedianLen);
        }
        if(applyToGyro){
            gyro[x] = (int)( (gAll[x] - gMax[x] - gMin[x]) / MFMedianLen);
            gyro[y] = (int)( (gAll[y] - gMax[y] - gMin[y]) / MFMedianLen);
            gyro[z] = (int)( (gAll[z] - gMax[z] - gMin[z]) / MFMedianLen);
        }
        //中位值率波 (去頭去尾) 結束
        return true;
    }
}


//********************************************************//
//														  //
//			Decode information from mpu6050
//														  //
//********************************************************//
void waitUntilSerialStable(Serial* SP, char *incomingData, int dataLength){
    int pass = 0;

    while(SP->IsConnected())
    {
        SP->ReadData(incomingData,dataLength);

        if(pass < 50){
            //printf("%.*s\n", readResult, incomingData);
            pass++;
        }else{
            break;
        }
    }
}
//return 0 if buffer overflow or no data is read(readResult = -1), else return 1
int readSerialIntoBuffer(Serial* SP, char *incomingData, int &dataLength, int &readResult, int &bLen, int bSize, char *buffer){
    readResult = SP->ReadData(incomingData,dataLength);

    std::string test(incomingData);
    if(readResult != -1){
        char *newData = new char[readResult];
        memcpy(newData, &incomingData[0], readResult);
        //printf("newData = %.*s\n", readResult, newData);

        bLen = bLen + readResult;
        // avoid buffer overflow
        if(bLen <= bSize){
            //printf("Blen = %d\n", bLen);
            //printf("readResult = %d\n", readResult);
            //get new string into buffer
            strncat (buffer, newData, readResult);

            return 1;
        }else{
            printf("Buffer overflow.\n\n\n\n\n\n\n\n\n\n");
            strcpy(buffer,"");
            bLen = 0;
            return 0;
        }
    }else{
        return 0; // result = -1 no data read
    }
}
