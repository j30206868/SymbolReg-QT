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
#include "mainwindow.h"

#include <QtWidgets>
#include "sstream"
#include "iostream"

const int IdRole = Qt::UserRole;

int getFileCount(QString fname){
    QString sampleDirPath = fname;
    QDir sampleDir(sampleDirPath);
    int fileCount = 0;
    //先計算檔案數量
    foreach(QFileInfo item, sampleDir.entryInfoList() )
    {
        if(item.isDir()){

        }else if(item.isFile()){
            fileCount++;
        }
    }
    return fileCount;
}

void addSelectBoxItemsByFileName(QComboBox *combox, QString fname){
    QString sampleDirPath = fname;
    QDir sampleDir(sampleDirPath);

    int fileCount = getFileCount(fname);
    //依序加入檔案 檔名照(1~filecount.txt)排序
    for(int i=1 ; i<=fileCount ; i++){
        std::stringstream sstm;
        sstm << i << ".txt";
        QString orderedFname = sstm.str().c_str();
        foreach(QFileInfo item, sampleDir.entryInfoList() )
        {
            if( orderedFname.compare(item.fileName()) == 0 ){
                QString filename = sampleDirPath + item.fileName();
                combox->addItem(filename, filename);
                break;
            }
        }
    }
}

Window::Window()
{
    isReadingMPU6050 = false;
    tempDirPath   = "Symbol/";
    sampleDirPath = "input/";

    renderArea = new RenderArea;

    tempSelectBox = new QComboBox;
    addSelectBoxItemsByFileName(tempSelectBox, tempDirPath);
    tempLabel = new QLabel(tr("&Temp:"));
    tempLabel->setBuddy(tempSelectBox);

    sampleSelectBox = new QComboBox;
    addSelectBoxItemsByFileName(sampleSelectBox, sampleDirPath);
    sampleLabel = new QLabel(tr("&Sample:"));
    sampleLabel->setBuddy(sampleSelectBox);

    penWidthSpinBox = new QSpinBox;
    penWidthSpinBox->setRange(0, 20);
    penWidthSpinBox->setSpecialValueText(tr("0 (cosmetic pen)"));

    penWidthLabel = new QLabel(tr("Pen &Width:"));
    penWidthLabel->setBuddy(penWidthSpinBox);

    strokeSpinBox = new QSpinBox;
    strokeSpinBox->setRange(0, 1);
    strokeLable = new QLabel(tr("Last Stroke:"));
    strokeLable->setBuddy(strokeSpinBox);
    connect(strokeSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(lastStrokeChanged()));
    renderArea->setLastStrokeSpinBox(strokeSpinBox);


    readDataBtn = new QPushButton(tr("開始讀取MPU6050數據"), this);
    connect(readDataBtn, SIGNAL(released()),
            this, SLOT(toggleMPU6050Reading()));

    penStyleComboBox = new QComboBox;
    penStyleComboBox->addItem(tr("Solid"), static_cast<int>(Qt::SolidLine));
    penStyleComboBox->addItem(tr("Dash"), static_cast<int>(Qt::DashLine));
    penStyleComboBox->addItem(tr("Dot"), static_cast<int>(Qt::DotLine));
    penStyleComboBox->addItem(tr("Dash Dot"), static_cast<int>(Qt::DashDotLine));
    penStyleComboBox->addItem(tr("Dash Dot Dot"), static_cast<int>(Qt::DashDotDotLine));
    penStyleComboBox->addItem(tr("None"), static_cast<int>(Qt::NoPen));

    penStyleLabel = new QLabel(tr("&Pen Style:"));
    penStyleLabel->setBuddy(penStyleComboBox);

    penCapComboBox = new QComboBox;
    penCapComboBox->addItem(tr("Flat"), Qt::FlatCap);
    penCapComboBox->addItem(tr("Square"), Qt::SquareCap);
    penCapComboBox->addItem(tr("Round"), Qt::RoundCap);

    penCapLabel = new QLabel(tr("Pen &Cap:"));
    penCapLabel->setBuddy(penCapComboBox);

    otherOptionsLabel = new QLabel(tr("Options:"));
    antialiasingCheckBox = new QCheckBox(tr("&Antialiasing"));
    transformationsCheckBox = new QCheckBox(tr("&Transformations"));

    connect(tempSelectBox, SIGNAL(activated(int)),
            this, SLOT(tempChanged()));
    connect(sampleSelectBox, SIGNAL(activated(int)),
            this, SLOT(sampleChanged()));

    connect(penWidthSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(penChanged()));
    connect(penStyleComboBox, SIGNAL(activated(int)),
            this, SLOT(penChanged()));
    connect(penCapComboBox, SIGNAL(activated(int)),
            this, SLOT(penChanged()));

    connect(antialiasingCheckBox, SIGNAL(toggled(bool)),
            renderArea, SLOT(setAntialiased(bool)));
    connect(transformationsCheckBox, SIGNAL(toggled(bool)),
            renderArea, SLOT(setTransformed(bool)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(3, 1);
    mainLayout->addWidget(renderArea, 0, 0, 1, 4);

    mainLayout->addWidget(tempLabel, 2, 0, Qt::AlignRight);
    mainLayout->addWidget(tempSelectBox, 2, 1);
    mainLayout->addWidget(sampleLabel, 2, 2, Qt::AlignRight);
    mainLayout->addWidget(sampleSelectBox, 2, 3);

    mainLayout->addWidget(penWidthLabel, 4, 0, Qt::AlignRight);
    mainLayout->addWidget(penWidthSpinBox, 4, 1);
    mainLayout->addWidget(penStyleLabel, 3, 0, Qt::AlignRight);
    mainLayout->addWidget(penStyleComboBox, 3, 1);
    mainLayout->addWidget(penCapLabel, 3, 2, Qt::AlignRight);
    mainLayout->addWidget(penCapComboBox, 3, 3);

    mainLayout->addWidget(strokeLable, 4, 2, Qt::AlignRight);
    mainLayout->addWidget(strokeSpinBox, 4, 3);

    mainLayout->addWidget(otherOptionsLabel, 5, 0, Qt::AlignRight);
    mainLayout->addWidget(antialiasingCheckBox, 5, 1, 1, 1, Qt::AlignRight);
    mainLayout->addWidget(transformationsCheckBox, 5, 2, 1, 2, Qt::AlignRight);

    mainLayout->addWidget(readDataBtn, 6, 3, Qt::AlignRight);

    setLayout(mainLayout);

    lastSampleFileNum = getFileCount(sampleDirPath);
    int lastSampleIdx = lastSampleFileNum - 1;
    int nextSampleCount = lastSampleFileNum + 1;

    tempSelectBox->setCurrentIndex(10);
    tempChanged();
    sampleSelectBox->setCurrentIndex(lastSampleIdx);
    sampleChanged();

    penChanged();
    antialiasingCheckBox->setChecked(true);

    mpuReader = new MpuReader(this);
    mpuReader->setSymSaveDir(sampleDirPath);
    mpuReader->setNextSymCount(nextSampleCount);
    QMetaObject::Connection cR = connect(mpuReader, SIGNAL(updateNewSymbol()),
                                         this, SLOT(updateSampleBox()));

    toggleMPU6050Reading();

    //std::cout << "connect result: " << cR.d_ptr << std::endl;

    setWindowTitle(tr("Basic Drawing"));
}

void Window::updateSampleBox(){
    int symCount = lastSampleFileNum + 1;

    std::stringstream sstm;
    sstm << symCount << ".txt";
    QString filename = sampleDirPath + sstm.str().c_str();
    sampleSelectBox->addItem(filename, filename);
    sampleSelectBox->setCurrentIndex(symCount-1);

    //更新最新的檔案數量
    lastSampleFileNum = getFileCount(sampleDirPath);
    //如果數量不正確應該要重新加入combobox的item 不過目前沒寫
    std::cout << "updateSampleBox triggered current idx: "<<symCount-1 << std::endl;
    sampleChanged();
}

void Window::setRenderAreaLastStrokeBox(QSpinBox *spinBoxObj){
    renderArea->setLastStrokeSpinBox(spinBoxObj);
}

void Window::lastStrokeChanged(){
    renderArea->setLastStroke( strokeSpinBox->value() );
}

void Window::toggleMPU6050Reading(){
    isReadingMPU6050 = !isReadingMPU6050;

    if(isReadingMPU6050){
        mpuReader->startReading();
        readDataBtn->setText(tr("停止讀取"));
    }else{
        mpuReader->stopReading();
        readDataBtn->setText(tr("開始讀取MPU6050數據"));
    }
}

void Window::tempChanged()
{
    QString tempPath = tempSelectBox->itemData(
                            tempSelectBox->currentIndex(), IdRole).toString();
    renderArea->changeTempPath(tempPath);
}

void Window::sampleChanged()
{
    QString samplePath = sampleSelectBox->itemData(
                            sampleSelectBox->currentIndex(), IdRole).toString();
    renderArea->changeSamplePath(samplePath);
}

void Window::penChanged()
{
    int width = penWidthSpinBox->value();
    Qt::PenStyle style = Qt::PenStyle(penStyleComboBox->itemData(
            penStyleComboBox->currentIndex(), IdRole).toInt());
    Qt::PenCapStyle cap = Qt::PenCapStyle(penCapComboBox->itemData(
            penCapComboBox->currentIndex(), IdRole).toInt());

    renderArea->setPen(QPen(Qt::blue, width, style, cap));
}
