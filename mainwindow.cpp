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

#include "sstream"
#include "iostream"

const int IdRole = Qt::UserRole;

Window::Window()
{
    isReadingMPU6050 = false;
    tempDirPath   = "Symbol/";
    sampleDirPath = "input/";

    this->setProperty("isReadingMPU6050", isReadingMPU6050);
    setWindowStyle();

    createMyUi();
    setMyUiProperty();
    createExampleUi();

    QGridLayout *mainLayout = new QGridLayout;
    addUiToMainLayout(mainLayout);

    buildUiConnection();

    setLayout(mainLayout);

    lastSampleFileNum = getFileCount(sampleDirPath);
    int lastSampleIdx = lastSampleFileNum - 1;
    int nextSampleCount = lastSampleFileNum + 1;

    tempSelectBox->setCurrentIndex(10);
    sampleSelectBox->setCurrentIndex(lastSampleIdx);

    //指定用
    tempSelectBox->setCurrentIndex(0);
    sampleSelectBox->setCurrentIndex(17);
    tempChanged();
    sampleChanged(false);

    //一般用
    //tempChanged();
    //sampleChanged();

    penChanged();
    antialiasingCheckBox->setChecked(true);

    mpuReader = createMpuReader();
    mpuReader->setNextSymCount(nextSampleCount);
    toggleMPU6050Reading();

    setWindowTitle(tr("軌跡辨識偵錯平台"));
}

/***********************************************
 *              建構子 functions
 * *********************************************/
void         Window::setWindowStyle(){
    QFile file(":/style/window.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    this->setStyleSheet(styleSheet);
}
void         Window::createMyUi(){
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
    //penWidthSpinBox->setSpecialValueText(tr("0 (cosmetic pen)"));

    penWidthLabel = new QLabel(tr("Pen &Width:"));
    penWidthLabel->setBuddy(penWidthSpinBox);

    strokeSpinBox = new QSpinBox;
    strokeSpinBox->setRange(0, 1);
    strokeLable = new QLabel(tr("Focused Stroke:"));
    strokeLable->setBuddy(strokeSpinBox);

    renderArea->setLastStrokeSpinBox(strokeSpinBox);

    readDataBtn   = new QPushButton(tr("讀取MPU6050數據"), this);
    makeNewSymBtn = new QPushButton(tr("產生新符號"), this);


}
void         Window::createExampleUi(){
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
}
void         Window::setMyUiProperty(){
    renderArea->setProperty("isReadingMPU6050", isReadingMPU6050);
}
void         Window::buildUiConnection(){
    connect(renderArea, SIGNAL(changeMainTempCurIdx(int)),
                    this, SLOT(changeTempCurIdx(int)));
    connect(strokeSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(lastStrokeChanged()));
    connect(readDataBtn, SIGNAL(clicked()),
            this, SLOT(toggleMPU6050Reading()));
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

}
void         Window::addUiToMainLayout(QGridLayout *mainLayout){
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(3, 1);
    mainLayout->addWidget(renderArea, 0, 0, 8, 6);

    mainLayout->addWidget(tempLabel, 3, 7, Qt::AlignRight);
    mainLayout->addWidget(tempSelectBox, 3, 8);
    mainLayout->addWidget(sampleLabel, 3, 9, Qt::AlignRight);
    mainLayout->addWidget(sampleSelectBox, 3, 10);

    mainLayout->addWidget(penWidthLabel, 5, 7, Qt::AlignRight);
    mainLayout->addWidget(penWidthSpinBox, 5, 8);
    mainLayout->addWidget(penStyleLabel, 4, 7, Qt::AlignRight);
    mainLayout->addWidget(penStyleComboBox, 4, 8);
    mainLayout->addWidget(penCapLabel, 4, 9, Qt::AlignRight);
    mainLayout->addWidget(penCapComboBox, 4, 10);

    mainLayout->addWidget(strokeLable, 5, 9, Qt::AlignRight);
    mainLayout->addWidget(strokeSpinBox, 5, 10);

    mainLayout->addWidget(otherOptionsLabel, 6, 7, Qt::AlignRight);
    mainLayout->addWidget(antialiasingCheckBox, 6, 8, 1, 1, Qt::AlignRight);
    mainLayout->addWidget(transformationsCheckBox, 6, 9, 1, 2, Qt::AlignRight);

    mainLayout->addWidget(readDataBtn  , 7, 9, 1, 2, Qt::AlignCenter);
    mainLayout->addWidget(makeNewSymBtn, 7, 7, 1, 2, Qt::AlignCenter);
}
MpuReader*   Window::createMpuReader(){
    MpuReader* reader = new MpuReader(this);
    reader->setSymSaveDir(sampleDirPath);

    connect(reader, SIGNAL(updateNewSymbol()),
                              this, SLOT(updateSampleBox()));
    connect(reader, SIGNAL(readingEnded()),
                              this, SLOT(mpu6050ReadingEnded()));
    return reader;
}
//建構子 functions

/***********************************************
 *              private used functions
 * *********************************************/
int  getFileCount(QString fname){
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
//private functions

/***********************************************
 *              private slot functions
 * *********************************************/
void Window::changeTempCurIdx(int nowIdx){
    tempSelectBox->setCurrentIndex(nowIdx);
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
    //std::cout << "updateSampleBox triggered current idx: "<<symCount-1 << std::endl;
    sampleChanged(true);
}
void Window::setRenderAreaLastStrokeBox(QSpinBox *spinBoxObj){
    renderArea->setLastStrokeSpinBox(spinBoxObj);
}
void Window::lastStrokeChanged(){
    renderArea->setLastStroke( strokeSpinBox->value() );
}
void Window::mpu6050ReadingEnded(){
    toggleMPU6050Reading();
}
void Window::toggleMPU6050Reading(){
    isReadingMPU6050 = !isReadingMPU6050;
    this->setProperty("isReadingMPU6050", isReadingMPU6050);
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
    if(isReadingMPU6050){
        mpuReader->startReading();
        readDataBtn->setText(tr("停止讀取"));
        readDataBtn->setFixedWidth(100);
    }else{
        mpuReader->stopReading();
        readDataBtn->setText(tr("讀取MPU6050數據"));
        readDataBtn->setFixedWidth(150);
    }
}
void Window::tempChanged()
{
    //QString tempPath = tempSelectBox->itemData(
    //                        tempSelectBox->currentIndex(), IdRole).toString();
    int curCount = tempSelectBox->currentIndex() + 1;
    int fileAmt  = getFileCount(tempDirPath);
    renderArea->changeTempPath(tempDirPath, curCount, fileAmt);
}
void Window::sampleChanged(bool showMostSimilar)
{
    //QString samplePath = sampleSelectBox->itemData(
    //                        sampleSelectBox->currentIndex(), IdRole).toString();
    //renderArea->changeSamplePath(samplePath);
    int curCount = sampleSelectBox->currentIndex() + 1;
    int fileAmt  = getFileCount(sampleDirPath);
    renderArea->changeSamplePath(sampleDirPath, curCount, fileAmt, showMostSimilar);
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
//Private slots
