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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QtWidgets>
#include "mpu6050reader.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;
class QPushButton;

class RenderArea;

//private used functions
int  getFileCount(QString fname);
void addSelectBoxItemsByFileName(QComboBox *combox, QString fname);
//private used functions

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private:
    //建構子
    void         setWindowStyle();
    void         createMyUi();
    void         createExampleUi();
    void         setMyUiProperty();
    void         buildUiConnection();
    void         addUiToMainLayout(QGridLayout *mainLayout);
    MpuReader*   createMpuReader();

private slots:
    void penChanged();
    void tempChanged();
    void sampleChanged(bool showMostSimilar = true);
    void toggleMPU6050Reading();
    void mpu6050ReadingEnded();
    void updateSampleBox();
    void lastStrokeChanged();
    void setRenderAreaLastStrokeBox(QSpinBox *spinBoxObj);
    void changeTempCurIdx(int nowIdx);

private:
    RenderArea *renderArea;

    QLabel *tempLabel;
    QLabel *sampleLabel;

    QLabel *penWidthLabel;
    QLabel *penStyleLabel;
    QLabel *penCapLabel;
    QLabel *otherOptionsLabel;

    QComboBox *tempSelectBox;
    QComboBox *sampleSelectBox;

    QLabel *strokeLable;
    QSpinBox *strokeSpinBox;

    QSpinBox *penWidthSpinBox;
    QComboBox *penStyleComboBox;
    QComboBox *penCapComboBox;
    QCheckBox *antialiasingCheckBox;
    QCheckBox *transformationsCheckBox;

    QPushButton *readDataBtn;
    QPushButton *makeNewSymBtn;

    MpuReader *mpuReader;

    bool isReadingMPU6050;
    QString tempDirPath;
    QString sampleDirPath;
    int lastSampleFileNum;
};

#endif // WINDOW_H
