#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QDataStream>
#include <QTextStream>
#include <QtNetwork/QUdpSocket>
#include <QQueue>

#include "qcustomplot.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    const int Channel_Number = 9;

private slots:
    void on_pushButton_open_clicked();

    void on_pushButton_close_clicked();

    void processSerialRec();

    void timerUpdate();
    void timerUpdate_indicators();

    void selectionChanged();

    void on_pushButton_autoscale_clicked();

    void on_pushButton_ShowHide_clicked();

private:

    const int PIC_WIDTH=1000;

    QVector<double> x;

    //初始化画布
    void qcp_init(void);

    Ui::MainWindow *ui;

    //串口
    QSerialPort *serialport;

    //接收数据缓冲
    QVector<double> rec[9];
    QQueue<char> rec_fifo;

    //绘图
    QTimer *timer;
    //显示更新
    QTimer *timer_indicators;

    //错误帧计数
    int bad_frame_counter;

    void dataParser(QString &s, float (&AccGyroRaw)[9]);
};

#endif // MAINWINDOW_H
