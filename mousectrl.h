#ifndef MOUSECTRL_H
#define MOUSECTRL_H

#include "cursor.h"

const int MOUSECTRL_MSSTATE = 1; // mouse control
const int MOUSECTRL_TRAJPROC = 2;// symbol processing(including record new symbol and recognize symbol)

class MouseCtrl
{
    private:
        Cursor cs;
        long dx;
        long dy;
        INPUT myMouseInputs[1];
        //紀錄drift的值
        int TotalAccl[3];
        int TotalGyro[3];

    public:
        //state
        int state;
        //嘗試解決drift問題
        int DriftRecCounter;
        int DriftAccl[3];
        int DriftGyro[3];
        //
        int gyroSensitivity;

        MouseCtrl();
        //~MouseCtrl();

        //移動滑鼠
        void moveOnlyWithGyro(int *gyro, int time);
        bool moveCursor(int *accl, int *gyro, double *velocity, int *AcclZeroC, int period, bool realMove);

        //處理加速度計的位移計算
        void velocityGetDisplace(int &displaceX, int &displaceZ, int *accl, double *velocity, int *AcclZeroC, bool isGyroMoved, int period, bool updateX = true, bool updateZ = false);
        void updateVelocity(int *accl, double *velocity, int *zeroC, bool isGyroMoved, double second, bool updateX, bool updateZ);

        //gyro濾雜訊
        bool gyroMoveFilter(int *gyro, int threshold);

        //更新滑鼠座標
        void updateDxDy(int GyroOneTimeDx, int GyroOneTimeDy, int VeloOneTimeDx, int VeloOneTimeDy, bool realMove);
        void updateDxDy(int oneTimeDx, int oneTimeDy, bool realMove);
        void posReset();

        //滑鼠按鍵點擊
        int sendMouseInput(int CtrlFlag);

        //取得滑鼠座標
        long getMouseX();
        long getMouseY();
        long getDx();
        long getDy();

        //處理drift問題
        void removeGyroDrift(int *gyro);
        void removeAcclDrift(int *accl, int axis);

        //紀錄drift的值
        bool updateTotalDrift(bool isMoved);
        void updateTotalAcclAndGyro(int *accl, int *gyro);
        void updateTotalAccl(int *accl);
        void updateTotalGyro(int *gyro);
};

#endif // MOUSECTRL_H

