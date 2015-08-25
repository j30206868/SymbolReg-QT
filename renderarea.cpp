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

#include "iostream"
#include "math.h"

#include "mousectrl.h"

#include <QPainter>

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    shape = Polygon;
    antialiased = false;
    transformed = false;

    //this->setFixedSize(QSize(900, 500));

    isCompared = false;
    tempPath = "Symbol/19.txt";
    samplePath = "Symbol/2.txt";

    //this->setFixedSize(200,250);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

QSize RenderArea::minimumSizeHint() const
{
    return QSize(100, 250);
}

QSize RenderArea::sizeHint() const
{
    return QSize(600, 250);
}

void RenderArea::setShape(Shape shape)
{
    this->shape = shape;
    update();
}

void RenderArea::setPen(const QPen &pen)
{
    this->pen = pen;
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

void RenderArea::changeTempPath(QString str){
    tempPath = str;
    std::cout << tr("tempPath: ").toLocal8Bit().data() << tempPath.toLocal8Bit().data() << std::endl;

    if(samplePath != ""){
        isCompared = false;
        update();
    }
}

void RenderArea::changeSamplePath(QString str){
    samplePath = str;
    std::cout << tr("samplePath: ").toLocal8Bit().data() << samplePath.toLocal8Bit().data() << std::endl;

    if(tempPath != ""){
        isCompared = false;
        update();
    }
}

void RenderArea::drawSymbolWithSymLine(SymLine *lines, int lineNum, int offsetX, int offsetY, double scaleX, double scaleY){
    QPainter symPainter(this);
    symPainter.setBrush(brush);

    //畫出來
    for(int i=0; i<lineNum ; i++){
        if(lines[i].color == 'g'){
            pen.setColor(QColor(0,255,0,255));
        }else if(lines[i].color == 'r'){
            pen.setColor(QColor(255,0,0,255));
        }
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
void RenderArea::transformSymbolIntoBoxSize(SymLine *lines, int lineNum, int w, int h, int &offsetX, int &offsetY, double &scaleX, double &scaleY){
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

    //如果scale變大或變小, offset也要跟著縮放(因為他縮放的規則是座標直接乘以scale的數值)
    offsetX = (0 - left) * scaleX;
    offsetY = (0 - top)  * scaleY;
}

void RenderArea::drawMatchedResult(dualCTData bestMatch, int boxW, int boxH, int sym1X, int sym1Y, int sym2X, int sym2Y ){
    MouseCtrl mc1 = MouseCtrl();
    MouseCtrl mc2 = MouseCtrl();

    //移動幅度
    mc1.gyroSensitivity = 30;
    mc2.gyroSensitivity = 30;

    //畫擷取後特徵值的比較
    int dx1 = 0, dy1 = 0;
    int dx2 = 0, dy2 = 0;

    //暫存移動時的資訊
    double velocity1[3] = {0, 0, 0};
    int AcclZeroC1[3] = {0, 0, 0};
    double velocity2[3] = {0, 0, 0};
    int AcclZeroC2[3] = {0, 0, 0};

    SymLine sym1Lines[bestMatch.A.length];
    SymLine sym2Lines[bestMatch.B.length];

    //由於不的match的地方都補0了 所以bestMatch A跟B長度是一樣的
    for(int i=0; i<bestMatch.A.length ; i++){
        //借用移動滑鼠的object 但不真的移動滑鼠, 僅僅是用來算出各筆畫結束時的座標, 加速度不用設0
        int accl[3] = {0,0,0};
        bool isMoved1 = mc1.moveCursor(accl, bestMatch.A.level[i], velocity1, AcclZeroC1, 10, false);
        bool isMoved2 = mc2.moveCursor(accl, bestMatch.B.level[i], velocity2, AcclZeroC2, 10, false);

        int emptyFlag = emptySide(bestMatch.A.level[i][0], bestMatch.A.level[i][2], bestMatch.B.level[i][0], bestMatch.B.level[i][2]);
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

    int lineNum = bestMatch.A.length;

    if( lineNum > 0)
    {//至少有一個筆才需要畫
        int    offsetX;
        int    offsetY;
        double scaleX;
        double scaleY;
        transformSymbolIntoBoxSize(sym1Lines, lineNum, boxW, boxH, offsetX, offsetY, scaleX, scaleY);
        drawSymbolWithSymLine(sym1Lines, bestMatch.A.length, sym1X + offsetX, sym1Y + offsetY, scaleX, scaleY);

        transformSymbolIntoBoxSize(sym2Lines, lineNum, boxW, boxH, offsetX, offsetY, scaleX, scaleY);
        drawSymbolWithSymLine(sym2Lines, bestMatch.B.length, sym2X + offsetX, sym2Y + offsetY, scaleX, scaleY);
    }
}

void RenderArea::paintEvent(QPaintEvent * /* event */)
{

    if(isCompared == false){
        trajData *temp   = readTrajDataFromFile(tempPath  .toStdString());
        trajData *sample = readTrajDataFromFile(samplePath.toStdString());
        double result = 0;
        dualCTData bestMatch = compareTwoSymbol(temp, sample, result, true);

        //起始座標
        int sym1X = 20; int sym1Y = 20;
        int sym2X = 20; int sym2Y = 130;
        int w = 90, h = 90;

        drawMatchedResult(bestMatch, w, h, sym1X, sym1Y, sym2X, sym2Y);

        isCompared = true;
    }

    /*painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));*/

    //qDebug() << tr("Paint event done!");
}
