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

#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QBrush>
#include <QSpinBox>
#include <QPen>
#include <QWidget>

#include "symboltype.h"

//在畫symbol時用來暫存向量
struct SymLine{
    int x1;
    int y1;
    int x2;
    int y2;
    char color;
};

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    enum Shape { Line, Points, Polyline, Polygon, Rect, RoundedRect, Ellipse, Arc,
                 Chord, Pie, Path, Text };

    RenderArea(QWidget *parent = 0);

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

public slots:
    void setShape(Shape shape);
    void setPen(const QPen &pen);
    void setBrush(const QBrush &brush);
    void setAntialiased(bool antialiased);
    void setTransformed(bool transformed);

    void setLastStroke(int value);
    void setLastStrokeSpinBox(QSpinBox *obj);
    void setPenColorByChar(int i, char c);

    void changeTempPath(QString tDirPath, int tCurCount, int tFileAmt);
    void changeSamplePath(QString sDirPath, int sCurCount, int sFileAmt, bool showMostSimilar);
    void transformSymbolIntoBoxSize(SymLine *lines, int lineNum, int w, int h, int &offsetX, int &offsetY, double &scaleX, double &scaleY, bool xyEqualProportion = true);
    void drawDetailBoxBesideComparedFigure(dualCTData bestMatch, SymLine *lines, double result, QRect rect);
    void drawSymbolWithSymLine(SymLine *lines, int lineNum, int offsetX, int offsetY, double scaleX, double scaleY);
    void drawMatchedResult(dualCTData bestMatch, trajData *temp, trajData *sample, double result, int boxW, int boxH, int sym1X, int sym1Y, int sym2X, int sym2Y );

signals:
    void changeMainTempCurIdx(int nowIdx);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    Shape shape;
    QPen pen;
    QBrush brush;
    bool antialiased;
    bool transformed;

    int strokeWidth;
    int lastStroke;
    QSpinBox *spinBoxObj;

    bool isCompared;
    QString tempDirPath;
    int tempCurFileCount;
    int tempFileAmount;
    QString sampleDirPath;
    int sampleCurFileCount;
    int sampleFileAmount;

    bool showMostSimilarTemp;

    trajData *usedTemp;
    trajData *usedSample;
    dualCTData bestMatchResult;
    bool isNewResultValid;
    double result;
};

#endif // RENDERAREA_H
