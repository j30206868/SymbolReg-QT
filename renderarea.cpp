/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "renderarea.h"

#include "symbolreg.h"

#include <string>
#include <iostream>
#include "math.h"
#include <sstream>
#include <iomanip>
#include <stdio.h>

#include "mousectrl.h"
#include "symfinder.h"

#include <QPainter>

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    shape = Polygon;
    antialiased = false;
    transformed = false;

    isCompared = true;//一開始什麼都沒有不要比較
    tempDirPath = "";
    tempCurFileCount = -1;
    tempFileAmount = 0;
    sampleDirPath = "";
    sampleCurFileCount = -1;
    sampleFileAmount = 0;
    showMostSimilarTemp = false;
    lastStroke = 0;
    strokeWidth = 1;
    isNewResultValid = false;
    //this->setFixedSize(200,250);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    bestMatchResult.A.length = 0;
    bestMatchResult.B.length = 0;
    usedTemp   = 0;
    usedSample = 0;
}

QSize RenderArea::minimumSizeHint() const
{
    return QSize(720, 500);
}

QSize RenderArea::sizeHint() const
{
    return QSize(720, 600);
}

void RenderArea::setLastStroke(int value){
    lastStroke = value;
    update();
}
void RenderArea::setLastStrokeSpinBox(QSpinBox *obj){
    spinBoxObj = obj;
}

void RenderArea::setShape(Shape shape)
{
    this->shape = shape;
    update();
}

void RenderArea::setPen(const QPen &pen)
{
    this->pen = pen;
    strokeWidth = pen.width();
    update();
}

void RenderArea::setBrush(const QBrush &brush)
{
    this->brush = brush;
    update();
}

void RenderArea::setAntialiased(bool antialiased)
{
    this->antialiased = antialiased;
    update();
}

void RenderArea::setTransformed(bool transformed)
{
    this->transformed = transformed;
    update();
}

void RenderArea::changeTempPath(QString tDirPath, int tCurCount, int tFileAmt){
    tempDirPath      = tDirPath;
    tempCurFileCount = tCurCount;
    tempFileAmount   = tFileAmt;

    std::cout << tr("tempPath: ").toLocal8Bit().data() << tempDirPath.toLocal8Bit().data() << std::endl;

    if(sampleCurFileCount >= 0 && sampleFileAmount > 0){
        isCompared = false;
        update();
    }
}

void RenderArea::changeSamplePath(QString sDirPath, int sCurCount, int sFileAmt, bool showMostSimilar){
    sampleDirPath      = sDirPath;
    sampleCurFileCount = sCurCount;
    sampleFileAmount   = sFileAmt;
    showMostSimilarTemp = showMostSimilar;
    std::cout << tr("samplePath: ").toLocal8Bit().data() << sampleDirPath.toLocal8Bit().data() << std::endl;

    if(tempCurFileCount >= 0 && tempFileAmount > 0){
        isCompared = false;
        update();
    }
}

void RenderArea::setPenColorByChar(int i, char c)
{
    if(i == lastStroke){
        //pen.setColor(QColor(0,0,255,255));
        pen.setWidth(strokeWidth + 10);\
    }else{
        pen.setWidth(strokeWidth);
    }
    if(c == 'g'){
        pen.setColor(QColor(0,0,0,255));
    }else if(c == 'r'){
        pen.setColor(QColor(255,0,0,255));
    }
}

void RenderArea::drawSymbolWithSymLine(SymLine *lines, int lineNum, int offsetX, int offsetY, double scaleX, double scaleY){
    QPainter symPainter(this);
    symPainter.setBrush(brush);

    //畫出來
    for(int i=0; i<lineNum ; i++){
        setPenColorByChar(i, lines[i].color);
        symPainter.setPen(pen);

        QPoint p1 = QPoint(lines[i].x1, lines[i].y1);
        QPoint p2 = QPoint(lines[i].x2, lines[i].y2);

        symPainter.save();
        symPainter.translate(offsetX, offsetY);
        //symPainter.rotate(60.0);
        symPainter.scale(scaleX, scaleY);
        //symPainter.translate(-50, -50);
        symPainter.drawLine(p1, p2);
        symPainter.restore();
    }
}

void updateTopLeft(int &TLValue, int input){
    if( input < TLValue ){
        TLValue = input;
    }
}
void updateBtmRight(int &BRValue, int input){
    if( input > BRValue ){
        BRValue = input;
    }
}

void RenderArea::transformSymbolIntoBoxSize(SymLine *lines, int lineNum, int w, int h, int &offsetX, int &offsetY, double &scaleX, double &scaleY, bool xyEqualProportion){
    //取得將symbol位移到x1, y1為(0,0) ; x2,y2為(w,h) 時
    //  所需使用的offset跟scale參數值

    //首先必須取得 未調整前的(x1,y1)跟(x2,y2)
    //第一筆的數值為初使數值, 迴圈從第二筆開始跑
    int left   = std::min(lines[0].x1, lines[0].x2);
    int right  = std::max(lines[0].x1, lines[0].x2);
    int top    = std::min(lines[0].y1, lines[0].y2);
    int bottom = std::max(lines[0].y1, lines[0].y2);
    for(int i=1; i<lineNum ; i++){
        int thisLeft   = std::min(lines[i].x1, lines[i].x2);
        int thisRight  = std::max(lines[i].x1, lines[i].x2);
        int thisTop    = std::min(lines[i].y1, lines[i].y2);
        int thisBtm    = std::max(lines[i].y1, lines[i].y2);

        updateTopLeft (left  , thisLeft);
        updateBtmRight(right , thisRight);
        updateTopLeft (top   , thisTop);
        updateBtmRight(bottom, thisBtm);
    }

    double ow = right  - left;
    double oh = bottom - top;
    //若Symbol原本大於box的width跟height限制 會縮小, 反之會放大
    scaleX = (w/ow);
    scaleY = (h/oh);

    if(xyEqualProportion){
        //若是scaleX跟scaleY各自使用, 則就不是XY同時等比例縮放了, 所以選擇小的來用
        scaleX = std::min(scaleX, scaleY);
        scaleY = std::min(scaleX, scaleY);
    }

    //如果scale變大或變小, offset也要跟著縮放(因為他縮放的規則是座標直接乘以scale的數值)
    offsetX = (0 - left) * scaleX;
    offsetY = (0 - top)  * scaleY;
}

void RenderArea::drawDetailBoxBesideComparedFigure(dualCTData bestMatch, SymLine *lines, double result, QRect rect){
    //畫結果
    QPainter symPainter(this);
    symPainter.setBrush(brush);
    //symPainter.setPen(pen);
    std::stringstream sstm;

    //框框的位置資訊
    int x1, y1, x2, y2;
    rect.getCoords(&x1,&y1,&x2,&y2);

    int fontSize   = 15;
    int lineIndent = 3;

    QFont font("標楷體");
    font.setBold(true);
    font.setPixelSize(fontSize);
    symPainter.setFont(font);
    if(result >= 0.6){
        pen.setColor(QColor(0,255,0,255));
    }else if(result < 0.3){
        pen.setColor(QColor(255,0,0,255));
    }else{
        pen.setColor(QColor(0,0,0,255));
    }
    sstm << "相似度: " << result << std::endl;

    symPainter.setPen(pen);
    symPainter.drawText(x1, y1, sstm.str().c_str());

    sstm.str("");
    sstm << "  |    Temp特徵向量   |  Sample特徵向量   " << std::endl;
    fontSize = 13;
    font.setPixelSize(fontSize);
    font.setBold(false);
    //font.setUnderline(true);
    pen.setColor(QColor(0,0,0,255));
    symPainter.setFont(font);
    symPainter.setPen(pen);
    symPainter.translate(0, fontSize + lineIndent + 2);
    symPainter.drawText(x1, y1, sstm.str().c_str());

    for(int i=0 ; i<bestMatch.A.length ; i++){
        sstm.str("");

        char s[27];
        sprintf(s, "%2d| %6d %6d(%2d)", i, 0-bestMatch.A.level[i][2], bestMatch.A.level[i][0], ctDataMergeV(bestMatch.A.level[i][0], bestMatch.A.level[i][2]));
        sstm << s << " | ";
        sprintf(s, "%6d %6d(%2d)", 0-bestMatch.B.level[i][2], bestMatch.B.level[i][0], ctDataMergeV(bestMatch.B.level[i][0], bestMatch.B.level[i][2]));
        sstm << s << "\n";

        setPenColorByChar(i, lines[i].color);
        symPainter.setPen(pen);
        symPainter.translate(0, fontSize + lineIndent);
        fontSize = 13;
        font.setPixelSize(fontSize);
        if(i == lastStroke){
            font.setUnderline(true);
            font.setBold(true);
            symPainter.setFont(font);
        }else{
            font.setUnderline(false);
            font.setBold(false);
            symPainter.setFont(font);
        }
        symPainter.drawText(QPoint(x1, y1), sstm.str().c_str());
    }

    //一次直接在框框中把所有字畫出來(但就必須統一顏色)
    //symPainter.drawText(rect, Qt::AlignLeft, sstm.str().c_str());
}

void getComparedSymLines(dualCTData bestMatch, SymLine *sym1Lines, SymLine *sym2Lines, int lineNum){
    MouseCtrl mc1 = MouseCtrl();
    MouseCtrl mc2 = MouseCtrl();

    //移動幅度(如果太小 很多筆畫可能會被四捨五入掉 導致完全不動)
    //值設大一點 因為有boxlimit所以不影響
    mc1.gyroSensitivity = 250;
    mc2.gyroSensitivity = 250;

    //畫擷取後特徵值的比較
    int dx1 = 0, dy1 = 0;
    int dx2 = 0, dy2 = 0;

    //暫存移動時的資訊
    double velocity1[3] = {0, 0, 0};
    int AcclZeroC1[3] = {0, 0, 0};
    double velocity2[3] = {0, 0, 0};
    int AcclZeroC2[3] = {0, 0, 0};
    int accl[3] = {0,0,0};

    //由於不的match的地方都補0了 所以bestMatch A跟B長度是一樣的
    for(int i=0; i<lineNum ; i++){
        //借用移動滑鼠的object 但不真的移動滑鼠, 僅僅是用來算出各筆畫結束時的座標, 加速度不用設0
        bool isMoved1 = mc1.moveCursor(accl, bestMatch.A.level[i], velocity1, AcclZeroC1, 10, false);
        bool isMoved2 = mc2.moveCursor(accl, bestMatch.B.level[i], velocity2, AcclZeroC2, 10, false);

        int emptyFlag = checkEmptySide(bestMatch.A.level[i][0], bestMatch.A.level[i][2], bestMatch.B.level[i][0], bestMatch.B.level[i][2]);
        if(emptyFlag == EMP_NO_EMPTY)
        {//兩邊都有的筆畫
            //筆畫為綠色
            sym1Lines[i].color = 'g';
            sym2Lines[i].color = 'g';
        }else{
            //筆畫為紅色
            sym1Lines[i].color = 'r';
            sym2Lines[i].color = 'r';
        }

        //取得A的向量
        sym1Lines[i].x1 = dx1;
        sym1Lines[i].y1 = dy1;
        dx1 = getRound(mc1.getDx()/3.0*10);
        dy1 = getRound(mc1.getDy()/3.0*10);
        sym1Lines[i].x2 = dx1;
        sym1Lines[i].y2 = dy1;

        //取得B的向量
        sym2Lines[i].x1 = dx2;
        sym2Lines[i].y1 = dy2;
        dx2 = getRound(mc2.getDx()/3.0*10);
        dy2 = getRound(mc2.getDy()/3.0*10);
        sym2Lines[i].x2 = dx2;
        sym2Lines[i].y2 = dy2;
    }
}
SymLine * getTrajRawSymLines(trajData *data){
    MouseCtrl mc = MouseCtrl();

    //移動幅度(如果太小 很多筆畫可能會被四捨五入掉 導致完全不動)
    //值設大一點 因為有boxlimit所以不影響
    mc.gyroSensitivity = 250;

    //畫擷取後特徵值的比較
    int dx = 0, dy = 0;

    //暫存移動時的資訊
    double velocity[3] = {0, 0, 0};
    int AcclZeroC[3] = {0, 0, 0};
    int accl[3] = {0,0,0};

    SymLine *symLines = new SymLine[data->length];

    //由於不的match的地方都補0了 所以bestMatch A跟B長度是一樣的
    for(int i=0; i<data->length ; i++){
        //借用移動滑鼠的object 但不真的移動滑鼠, 僅僅是用來算出各筆畫結束時的座標, 加速度不用設0
        bool isMoved = mc.moveCursor(accl, data->level[i], velocity, AcclZeroC, 10, false);

        symLines[i].color = 'g';

        //取得向量
        symLines[i].x1 = dx;
        symLines[i].y1 = dy;
        dx = getRound(mc.getDx()/3.0*10);
        dy = getRound(mc.getDy()/3.0*10);
        symLines[i].x2 = dx;
        symLines[i].y2 = dy;
    }

    return symLines;
}
void getProportionalSymOfsetAndScaleXY(int &offset1X, int &offset2X, int &offset1Y, int &offset2Y, double &scale1X, double &scale2X, double &scale1Y, double &scale2Y, double &fScaleX, double &fScaleY){
    //如果直接照 框框的 比例縮放的話
    //兩個大小相差非常非常多的字也會看起來 也會一樣大
    //所以這邊scale的值 如果>1則取小的, 如果<1則取大的
    //(因為正常來說讀到的值scale都是<1 所以直接取大)
    offset1X *= 1/scale1X;
    offset1Y *= 1/scale1Y;
    offset2X *= 1/scale2X;
    offset2Y *= 1/scale2Y;
    //先把offset值還原
    fScaleX = std::min(scale1X, scale2X);
    fScaleY = std::min(scale1Y, scale2Y);
    //取得最終版本的scale值
    offset1X *= fScaleX;
    offset1Y *= fScaleY;
    offset2X *= fScaleX;
    offset2Y *= fScaleY;
}

void RenderArea::drawMatchedResult(dualCTData bestMatch, trajData *temp, trajData *sample, double result, int boxW, int boxH, int sym1X, int sym1Y, int sym2X, int sym2Y ){

    int lineNum = bestMatch.A.length;
    SymLine sym1Lines[lineNum];
    SymLine sym2Lines[lineNum];

    getComparedSymLines(bestMatch, sym1Lines, sym2Lines, lineNum);

    SymLine *tempSymLines = 0;
    int tempSymLineNum    = 0;
    SymLine *sampleSymLines = 0;
    int sampleSymLineNum    = 0;
    if(temp != 0 && sample != 0){
        tempSymLines = getTrajRawSymLines(temp);
        tempSymLineNum = temp->length;
        sampleSymLines = getTrajRawSymLines(sample);
        sampleSymLineNum = sample->length;
    }

    if( lineNum > 0)
    {//至少有一筆特徵向量才需要畫
        int    offset1X, offset2X;
        int    offset1Y, offset2Y;
        double scale1X, scale2X;
        double scale1Y, scale2Y;
        transformSymbolIntoBoxSize(sym1Lines, lineNum, boxW, boxH, offset1X, offset1Y, scale1X, scale1Y);
        transformSymbolIntoBoxSize(sym2Lines, lineNum, boxW, boxH, offset2X, offset2Y, scale2X, scale2Y);

        double fScaleX, fScaleY;
        getProportionalSymOfsetAndScaleXY(offset1X, offset2X, offset1Y, offset2Y, scale1X, scale2X, scale1Y, scale2Y, fScaleX, fScaleY);

        //將symbol畫出
        drawSymbolWithSymLine(sym1Lines, lineNum, sym1X + offset1X, sym1Y + offset1Y, fScaleX, fScaleY);
        drawSymbolWithSymLine(sym2Lines, lineNum, sym2X + offset2X, sym2Y + offset2Y, fScaleX, fScaleY);
        //將symbol用框框套起來
        QPainter symPainter(this);
        pen.setColor(QColor(155,155,155,155));
        pen.setWidth(0);
        symPainter.setPen(pen);
        symPainter.drawRect(sym1X-5, sym1Y-5, boxW+10, boxH+10);
        symPainter.drawRect(sym2X-5, sym2Y-5, boxW+10, boxH+10);
        //在框框上方寫字
        pen.setColor(QColor(0,0,0,255));
        QFont font("標楷體");
        font.setBold(true);
        symPainter.setFont(font);
        symPainter.setPen(pen);
        symPainter.drawText(sym1X, sym1Y-10, tr("Template特徵向量(右圖為Raw Data)"));
        symPainter.drawText(sym2X, sym2Y-10, tr("Sample特徵向量(右圖為Raw Data)"));


        //畫raw symbol
        int rawSymBoxW = boxW / 2;
        int rawSymBoxH = boxH / 2;
        if(tempSymLines != 0 && sampleSymLines != 0){
            transformSymbolIntoBoxSize(tempSymLines, tempSymLineNum, rawSymBoxW, rawSymBoxH, offset1X, offset1Y, scale1X, scale1Y);
            transformSymbolIntoBoxSize(sampleSymLines, sampleSymLineNum, rawSymBoxW, rawSymBoxH, offset2X, offset2Y, scale2X, scale2Y);
            getProportionalSymOfsetAndScaleXY(offset1X, offset2X, offset1Y, offset2Y, scale1X, scale2X, scale1Y, scale2Y, fScaleX, fScaleY);

            drawSymbolWithSymLine(tempSymLines, tempSymLineNum, sym1X + offset1X + boxW + 10, sym1Y + offset1Y, fScaleX, fScaleY);
            drawSymbolWithSymLine(sampleSymLines, sampleSymLineNum, sym2X + offset2X + boxW + 10, sym2Y + offset2Y, fScaleX, fScaleY);
        }

        int figureRight = sym1X + boxW + rawSymBoxW;
        int figureTop   = sym1Y;
        int detailBoxLeftPadding = 25;
        int dBoxX1 = figureRight + detailBoxLeftPadding;
        int dBoxY1 = figureTop;
        int dBoxW = 150;
        int dBoxH = 320;
        int dBoxX2 = dBoxX1 + dBoxW;
        int dBoxY2 = dBoxY1 + dBoxH;
        QRect rect(dBoxX1, dBoxY1, dBoxX2, dBoxY2);
        //給哪條line其實不影響, 因為兩條的color設定基本上是一樣的, 而drawDetailBoxBesideComparedFigure只會用到color
        drawDetailBoxBesideComparedFigure(bestMatch, sym1Lines, result, rect);
    }

    if(tempSymLines != 0)
        delete[] tempSymLines;
    if(sampleSymLines != 0)
        delete[] sampleSymLines;
}

void RenderArea::paintEvent(QPaintEvent * /* event */)
{
    if(isCompared == false){
        std::stringstream sstm;
        sstm << sampleDirPath.toStdString() << sampleCurFileCount << ".txt";

        trajData *temp   = 0;
        trajData *sample = readTrajDataFromFile(sstm.str());

        if( sample->length==0 ){
            std::cout << "Sample is empty, can't be compared." << std::endl;
            isNewResultValid = false;
        }else{
            result = 0;
            freeDualCT(bestMatchResult);
            //bestMatchResult = compareTwoSymbol(temp, sample);

            int matchSymIdx = -1;
            if( showMostSimilarTemp ){
                //找相似度最高的temp並秀出來
                SymbolMatch SymMatch = SymbolMatch(tempDirPath.toStdString(), tempFileAmount);
                double *simiList = new double[SymMatch.getSymAmt()];
                matchSymIdx = SymMatch.findBestMatchedSym(sample, simiList);

                if(matchSymIdx != -1){
                    temp = SymMatch.getSymbol(matchSymIdx);
                    emit changeMainTempCurIdx(matchSymIdx);
                }

                //除理被選重的temp其它的記憶體空間都free掉
                SymMatch.freeTemplates(matchSymIdx);
                showMostSimilarTemp = false;//只秀第一次
            }

            if(matchSymIdx == -1)
            {//若找不到相似度夠高的 或 指定使用目前指到的temp, 則使用目前的temp
                sstm.str("");
                sstm << tempDirPath.toStdString() << tempCurFileCount << ".txt";
                temp   = readTrajDataFromFile(sstm.str());
            }

            freePTraj(usedTemp);
            freePTraj(usedSample);
            usedTemp   = temp;
            usedSample = sample;
            if(usedTemp->length > 0){
                std::cout << "Final comparision" << std::endl;
                bestMatchResult = compareTwoSymbol(temp, sample);
                //印出整個對齊結果
                //result = showBestMatchResult(bestMatchResult.A, bestMatchResult.B, true);

                result = calcComplexSimilarity(bestMatchResult, temp, sample, true);

                std::cout << std::endl;

                //更新筆數總數
                lastStroke = bestMatchResult.A.length-1;
                spinBoxObj->setRange(0, lastStroke);
                spinBoxObj->setValue(lastStroke);
                isNewResultValid = true;
            }
        }

        isCompared = true;
    }

    int w = 220, h = 220;
    //起始座標
    int figuresUpDownPadding = 30;
    int sym1X = 20; int sym1Y = 20;
    int sym2X = 20; int sym2Y = sym1Y + h + figuresUpDownPadding;

    if(isNewResultValid){

        drawMatchedResult(bestMatchResult, usedTemp, usedSample, result, w, h, sym1X, sym1Y, sym2X, sym2Y);

    }else{
        QPainter symPainter(this);
        pen.setColor(QColor(255,0,0,255));
        pen.setWidth(5);
        symPainter.setPen(pen);

        QFont font("標楷體");
        font.setPixelSize(20);
        symPainter.setFont( font );
        symPainter.drawText(sym1X, sym1Y, tr("Template或Sample為空,無法比較!"));
    }
}
