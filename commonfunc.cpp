#include "commonfunc.h"
#include <QString>
#include <fstream>

//IntArray處理
void cleanIntA(intArray &data){
    if(data.values != NULL)
            delete[] data.values;
}
intArray copyIntA(intArray data){
    intArray newData;
    newData.length = data.length;
    newData.values = new int[data.length];
    for(int i=0; i<data.length; i++){
        newData.values[i] = data.values[i];
    }
    return newData;
}

//dualIntArray處理
void cleanDualIntArray(dualIntArray &data){
    cleanIntA(data.A);
    cleanIntA(data.B);
}

//陣列處理
int **allcIntDArray(int rowN, int colN){
    int **newIntDArr = new int*[rowN];
    for(int i=0; i<rowN ; i++)
        newIntDArr[i] = new int[colN];

    return newIntDArr;
}
void freeIntDArray(int **dArr, int rowN){
    for(int i=0; i<rowN ; i++)
        delete[] dArr[i];
    delete[] dArr;
}

//數學運算
int getRound(double num){
    int sign = 0;
    int result = 0;

    //記錄正負
    if(num > 0){
        sign = 1;
    }else if(num < 0){
        sign = -1;
    }else{
        return 0;
    }

    //全部以正數處理
    num = abs(num);
    int integerpart = floor(num);

    double left = num - integerpart;

    if(left >=0.5){
        result = ceil(num);
    }else if( left < 0.5){
        result = floor(num);
    }

    return result * sign;
}
bool isEqualSign(int A, int B){
    if(A >= 0 && B >= 0){
        return true;
    }else if( A <= 0 && B <= 0){
        return true;
    }else{
        return false;
    }
}
int getSign(int v){
    if(v == 0){
        return 0;
    }else if(v > 0){
        return 1;
    }else{
        return -1;
    }
}

//檔案處理
bool cleanFile(std::string fname){

    //clean the file
    std::ofstream myfile (fname.c_str());
    myfile << "";
    myfile.close();

    return true;
}

//String處理
intArray splitAsInt(std::string str, std::string delimiter){
    int buffer[10];
    int idx = 0;

    int posi=0;
    std::string tmp = "";
    while( (posi = str.find(delimiter)) != (signed)std::string::npos )
    {
        tmp = str.substr(0, posi);
        //cout <<" " << tmp << " ";
        buffer[idx] = QString::fromUtf8(tmp.c_str()).toInt();
        //cout <<" " << result[idx] << " ";
        str.erase(0, posi + delimiter.length());
        idx++;
    }

    tmp = str.substr(0, str.length());
    buffer[idx] = QString::fromUtf8(tmp.c_str()).toInt();
    //cout <<" "<< result[idx] << " ";

    intArray result;
    result.length = (idx+1);
    result.values = new int[result.length];
    for(int i=0 ; i<result.length ;i++)
    {
        result.values[i] = buffer[i];
    }

    return result;
}
