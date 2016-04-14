#include "mousectrl.h"

#include "commonfunc.h"
#include "math.h"

MouseCtrl::MouseCtrl(){
    cs = Cursor();
    //drift的資訊歸零
    for(int i=0; i<3 ; i++){
        DriftAccl[i] = 0;
        DriftGyro[i] = 0;
        TotalAccl[i] = 0;
        TotalGyro[i] = 0;
    }
    DriftRecCounter = 0;
    posReset();
    gyroSensitivity = 225;
    state = MOUSECTRL_MSSTATE;
}

bool MouseCtrl::moveCursor(int *accl, int *gyro, double *velocity, int *AcclZeroC, int period, bool realMove){
    //gyro的值 > 30 才會計算位移量
    //若事gyroth過小(如:1), 則會產生drift問題
    const int GyroMoveTh = 50;

    //如果一開始就對drift處理
    //removeAcclDrift(accl, 0);
    //removeAcclDrift(accl, 1);
    //removeAcclDrift(accl, 2);
    //removeGyroDrift(gyro);
    //如果一開始就對drift處理

    //濾掉 雜訊 gyro
    bool isGyroMoved = true;
    //如果只是假移 不需要管是否有drift的雜訊了(很可能是用在畫軌跡)
    if(realMove == true)
    {//真的移動才需要做gyro濾值得動作
        isGyroMoved = gyroMoveFilter(gyro, GyroMoveTh);
    }
    if( isGyroMoved ){
        //處理drift問題
        //removeGyroDrift(gyro);

        double xdegree = (gyro[2]/1000.0 * period) / 131.0;
        double ydegree = (gyro[0]/1000.0 * period) / 131.0;

        //計算此次gyro產生的移動量
        int gyroOneTimeDX = 0 - getRound(xdegree * gyroSensitivity);
        int gyroOneTimeDY = 0 - getRound(ydegree * gyroSensitivity);

        //計算加速度計部分影響的移動量
        //目前只做X軸(因Z軸受到重力影響較嚴重 且 螢幕寬螢幕較常見)
        int displaceX, displaceZ;
        velocityGetDisplace(displaceX, displaceZ, accl, velocity, AcclZeroC, isGyroMoved, period, true , true);

        //更新滑鼠位移量(velocity + gyro)
        if(realMove == true){
            //updateDxDy(gyroOneTimeDX, gyroOneTimeDY, displaceX, displaceZ, realMove);
            updateDxDy(gyroOneTimeDX, gyroOneTimeDY, realMove);
        }else{
            updateDxDy(gyroOneTimeDX, gyroOneTimeDY, realMove);
        }

        if(realMove){
            cs.moveTo(cs.cX + dx, cs.cY + dy);
        }
    }
    return isGyroMoved;
}

void MouseCtrl::velocityGetDisplace(int &displaceX, int &displaceZ, int *accl, double *velocity, int *AcclZeroC, bool isGyroMoved, int period, bool updateX, bool updateZ){
    double second = period / 1000.0;
    double lastVX = velocity[0];
    double lastVZ = velocity[2];
    updateVelocity(accl, velocity, AcclZeroC, isGyroMoved, second, updateX, updateZ);

    //若上一次的速度跟此次一樣 表示此次的accl為0 所以不計算位移量
    if( lastVX != velocity[0] ){
        displaceX =  (2*(velocity[0])*second);
    }else{
        displaceX = 0;
    }
    if( lastVZ != velocity[2] ){
        displaceZ =  0 - (2*(velocity[2])*second); //Z軸的移動成反方向
    }else{
        displaceZ = 0;
    }
}
void MouseCtrl::updateVelocity(int *accl, double *velocity, int *zeroC, bool isGyroMoved, double second, bool updateX, bool updateZ){
    int moveTh[3] = {500, 500, 800};
    //int moveTh[3] = {2000, 2000, 2000};
    float time = second;
    bool isMoved = false;
    int tmpAccl[3] = {accl[0], accl[1], accl[2]};

    int axis;
    //確認從哪軸開始更新
    if(updateX){
        axis = 0;
    }else if(updateZ){
        axis = 2;//做Z軸不做X軸
    }else{
        axis = 100;//兩軸都不更新
    }

    while(axis <= 2){
        if(abs(tmpAccl[axis]) > moveTh[axis]){
            //在算入accl之前嘗試移除可能的drift值
            removeAcclDrift(tmpAccl, axis);

            //計算velocity
            tmpAccl[axis] *= time;
            isMoved = true;
            zeroC[axis] = 0;

            //x軸
            //if(axis == 0 && tmpAccl[axis] > 0)
            //	tmpAccl[axis] *= 1.125;

            //z軸
            //if(axis == 2 && tmpAccl[axis] > 0)
            //	tmpAccl[axis] *= 1.125;

            velocity[axis] += tmpAccl[axis];
            //dx+= (2*(velocity[axis])*time);
            //dx+= ((lastvelocity[x]*time)+(0.5*velocity[x]*time));
            //displace[x] += ((lastvelocity[x]*time)+(0.5*velocity[x]*time));
        }else{
            tmpAccl[axis] = 0;
            zeroC[axis]++;
        }

        //若超過一定的時間內accl都沒有超過門檻, 判定靜止狀態, velocity值歸零
        int Nth = 6;
        int Nth2 = Nth*2;
        //if((zeroC[axis] >= Nth2 ) || (zeroC[axis] >= Nth && (!isGyroMoved)))
        if((zeroC[axis] >= Nth ) || (!isGyroMoved))
        {
            velocity[axis] = 0;
        }

        //做完x做z軸
        if(updateZ){
            axis+=2;
        }else{//不需要更新Z軸 跳出迴圈
            break;
        }
    }
}

void MouseCtrl::updateDxDy(int GyroOneTimeDx, int GyroOneTimeDy, int VeloOneTimeDx, int VeloOneTimeDy, bool realMove){
    //*******重要*********//
    //若加速度計影響的方向與gyro不同, 則加速度計部分不列入考慮
    if( !isEqualSign(GyroOneTimeDx, VeloOneTimeDx) )
        VeloOneTimeDx = 0;
    if( !isEqualSign(GyroOneTimeDy, VeloOneTimeDy) )
        VeloOneTimeDy = 0;

    //單純看加速度計的影響
    //GyroOneTimeDx = 0;
    //GyroOneTimeDy = 0;

    int tTmpDx = GyroOneTimeDx + VeloOneTimeDx;
    int tTmpDy = GyroOneTimeDy + VeloOneTimeDy;
    updateDxDy(tTmpDx, tTmpDy, realMove);
}
void MouseCtrl::updateDxDy(int oneTimeDx, int oneTimeDy, bool realMove){
    int tmpDx = dx + oneTimeDx;
    int tmpDy = dy + oneTimeDy;
    //if is moving out of the screen, don't update

    if(realMove == true){
        //必須限定在邊界內
        if(tmpDx < cs.cX){
            if(tmpDx > (-cs.cX) ){
                dx = tmpDx;
            }
        }

        if(tmpDy < cs.cY){
            if(tmpDy > (-cs.cY) ){ //
                dy = tmpDy;
            }
        }
    }else
    {//只是假移 移到哪都沒差
        dx = tmpDx;
        dy = tmpDy;
    }
}

bool MouseCtrl::gyroMoveFilter(int *gyro, int threshold){
    //濾掉 雜訊 gyro
    bool isGyroMoved = false;
    if( (gyro[0]/threshold) != 0){
        isGyroMoved = true;
    }else{
        gyro[0] = 0;
    }
    if( (gyro[1]/threshold) != 0){
        isGyroMoved = true;
    }else{
        gyro[1] = 0;
    }
    if( (gyro[2]/threshold) != 0){
        isGyroMoved = true;
    }else{
        gyro[2] = 0;
    }

    return isGyroMoved;
}

void MouseCtrl::posReset(){
    dx = 0;
    dy = 0;
    //cs.moveTo(cs.cX, cs.cY);
}

int MouseCtrl::sendMouseInput(int CtrlFlag){
    //CtrlFlag can be (MOUSEEVENTF_RIGHTUP)(MOUSEEVENTF_RIGHTDOWN)(MOUSEEVENTF_LEFTUP)(MOUSEEVENTF_LEFTDOWN)
    myMouseInputs[0].type = INPUT_MOUSE;
    myMouseInputs[0].mi = MOUSEINPUT();
    myMouseInputs[0].mi.mouseData = NULL;
    myMouseInputs[0].mi.time = NULL;
    myMouseInputs[0].mi.dwExtraInfo = NULL;
    myMouseInputs[0].mi.dx = getMouseX();
    myMouseInputs[0].mi.dy = getMouseY();
    myMouseInputs[0].mi.dwFlags = CtrlFlag;

    if(SendInput(1, myMouseInputs, sizeof(INPUT))){
        //printf("Click simulation is succeeded.\n");
        return true;
    }else{
        printf("Click simulation is failed.\n\n\n\n\n");
        return false;
    }
}

long MouseCtrl::getMouseX(){
    return cs.nowX;
}
long MouseCtrl::getMouseY(){
    return cs.nowY;
}
long MouseCtrl::getDx(){
    return dx;
}
long MouseCtrl::getDy(){
    return dy;
}

void MouseCtrl::removeGyroDrift(int *gyro){
    //gyro[2] -= 12;
    //gyro[0] += 15;
    gyro[0] -= DriftGyro[0];
    gyro[1] -= DriftGyro[1];
    gyro[2] -= DriftGyro[2];
}
void MouseCtrl::removeAcclDrift(int *accl, int axis){
    accl[axis] -= DriftAccl[axis];
}
//紀錄drift的值
bool MouseCtrl::updateTotalDrift(bool isMoved){
    int udtCountTh = 20;

    if(!isMoved)//沒有移動 不需要更新
        return false;

    if( DriftRecCounter > udtCountTh ){
        for(int i=0; i<3 ; i++){
            //更新Drift的值
            DriftAccl[i] = getRound(TotalAccl[i] / (double)DriftRecCounter);
            DriftGyro[i] = getRound(TotalGyro[i] / (double)DriftRecCounter);
            //歸0
            TotalAccl[i] = 0;
            TotalGyro[i] = 0;
        }
        //計數器歸零
        DriftRecCounter = 0;

        return true;
    }else if(DriftRecCounter > 0)
    {//否則把觀測值歸零就好, 保留之前估測到的drift值
        for(int i=0; i<3 ; i++){
            TotalAccl[i] = 0;
            TotalGyro[i] = 0;
        }
        DriftRecCounter = 0;
    }
    return false;
}
void MouseCtrl::updateTotalAcclAndGyro(int *accl, int *gyro){
    updateTotalAccl(accl);
    updateTotalGyro(gyro);
    DriftRecCounter++;
}
void MouseCtrl::updateTotalAccl(int *accl){
    TotalAccl[0] += accl[0];
    TotalAccl[1] += accl[1];
    TotalAccl[2] += accl[2];
}
void MouseCtrl::updateTotalGyro(int *gyro){
    TotalGyro[0] += gyro[0];
    TotalGyro[1] += gyro[1];
    TotalGyro[2] += gyro[2];
}

void MouseCtrl::moveOnlyWithGyro(int *gyro, int time){
    int threshold = 40;

    //濾掉 雜訊 gyro
    bool isGyroMoved = gyroMoveFilter(gyro, threshold);

    if( isGyroMoved ){
        double xdegree = (gyro[2]/1000.0 * time) / 131.0;
        double ydegree = (gyro[0]/1000.0 * time) / 131.0;

        int gyroOneTimeDX = 0 - ceil(xdegree * 200);
        int gyroOneTimeDY = 0 - ceil(ydegree * 200);

        updateDxDy(gyroOneTimeDX, gyroOneTimeDY, true);

        cs.moveTo(cs.cX + dx, cs.cY + dy);
    }
}
