#ifndef COMMONFUNC_H
#define COMMONFUNC_H

#include <math.h>
#include <string>

struct intArray{
    int *values;
    int length;
};
struct dualIntArray{
    intArray A;
    intArray B;
};

//IntArray處理
void cleanIntA(intArray &data);
intArray copyIntA(intArray data);

//dualIntArray處理
void cleanDualIntArray(dualIntArray &data);

//陣列處理
int **allcIntDArray(int rowN, int colN);
void freeIntDArray(int **dArr, int rowN);

//數學運算
int getRound(double num);
bool isEqualSign(int A, int B);
int getSign(int v);

intArray splitAsInt(std::string str, std::string delimiter);

#endif // COMMONFUNC_H

