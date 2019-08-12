#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QtDebug>
#include <QThread>

#include <QRegExp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialport = new QSerialPort();

    ui->pushButton_close->setEnabled(false);

    //connect serialport signal and slot.
    connect(serialport,SIGNAL(readyRead()),this,SLOT(processSerialRec()));

    qcp_init();

    //Timer Init
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),
            this,SLOT(timerUpdate()));
    timer->start(25);
    //定时更新各种指示控件
    timer_indicators = new QTimer(this);
    connect(timer_indicators, SIGNAL(timeout()),
            this,SLOT(timerUpdate_indicators()));
    timer_indicators->start(100);

    //启动接收
    //ui->pushButton_open->clicked();
    ui->pushButton_close->setEnabled(false);

    //错误帧计数
    bad_frame_counter=0;
}

MainWindow::~MainWindow()
{
    serialport->close();
    delete ui;
}

void MainWindow::qcp_init(void)
{
    ui->customPlot->addGraph();
    ui->customPlot->xAxis->setRange(0,PIC_WIDTH);
    ui->customPlot->yAxis->setRange(-20000, 20000);
    ui->customPlot->graph(0)->setPen(QPen(QColor(255,0,0)));
    ui->customPlot->graph(0)->setName(QString::number(1));

    ui->customPlot->addGraph();
    ui->customPlot->graph(1)->setPen(QPen(QColor(0,255,0)));
    ui->customPlot->graph(1)->setName(QString::number(2));

    ui->customPlot->addGraph();
    ui->customPlot->graph(2)->setPen(QPen(QColor(128,0,255)));
    ui->customPlot->graph(2)->setName(QString::number(3));

    ui->customPlot->addGraph();
    ui->customPlot->graph(3)->setPen(QPen(QColor(0,128,255)));
    ui->customPlot->graph(3)->setName(QString::number(4));

    ui->customPlot->addGraph();
    ui->customPlot->graph(4)->setPen(QPen(QColor(0,255,128)));
    ui->customPlot->graph(4)->setName(QString::number(5));

    ui->customPlot->addGraph();
    ui->customPlot->graph(5)->setPen(QPen(QColor(255,128,0)));
    ui->customPlot->graph(5)->setName(QString::number(6));

    QColor graph_color[3]={QColor(128,128,128),QColor(60,128,200),QColor(90,30,128)};
    for(int i=0;i<3;i++)
    {
        ui->customPlot->addGraph();
        ui->customPlot->graph(i+6)->setPen(QPen(graph_color[i]));
        ui->customPlot->graph(i+6)->setName(QString::number(i+7));
    }

    //legend
    //可见
    ui->customPlot->legend->setVisible(true);


    //将图例矩形域放到右上角
    ui->customPlot->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignRight);
    //设置图例背景色
    ui->customPlot->legend->setBrush(QColor(255,255,255,0));

    //ui->customPlot->yAxis2->setRange(-5,+5);
    //ui->customPlot->yAxis->setLabel("adc_value");
    //ui->customPlot->yAxis2->setVisible(true);
    //ui->customPlot->yAxis2->setLabel("voltage(V)");


    for(int i=0;i<Channel_Number;i++)
    {
        rec[i].resize(PIC_WIDTH);
    }

    x.resize(PIC_WIDTH);
    for(int i=0;i<PIC_WIDTH;i++){
        x[i]=i;
    }

    // 支持鼠标拖拽轴的范围、滚动缩放轴的范围，左键点选图层（每条曲线独占一个图层）
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom
                                     | QCP::iSelectPlottables | QCP::iSelectItems
                                     | QCP::iSelectLegend);

    //设置Legend只能选择图例
    ui->customPlot->legend->setSelectableParts(QCPLegend::spItems);
    connect(ui->customPlot,SIGNAL(selectionChangedByUser()),this,SLOT(selectionChanged()));
}

void MainWindow::on_pushButton_open_clicked()
{
    serialport->setPortName("COM"+QString::number(ui->spinBox_comPort->value()));
    serialport->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);
    serialport->setDataBits(QSerialPort::Data8);
    serialport->setParity(QSerialPort::NoParity);
    serialport->setStopBits(QSerialPort::OneStop);
    serialport->setFlowControl(QSerialPort::NoFlowControl);
    serialport->setReadBufferSize(1024);
    if(serialport->open(QIODevice::ReadWrite))
    {
        ui->pushButton_close->setEnabled(true);
        ui->pushButton_open->setEnabled(false);
    }
}

void MainWindow::on_pushButton_close_clicked()
{
    serialport->close();

    ui->pushButton_close->setEnabled(false);
    ui->pushButton_open->setEnabled(true);
}

void MainWindow::processSerialRec()
{
    QByteArray arr;
    arr=serialport->readAll();
//    qDebug()<<arr;
    for(int i=0;i<arr.length();i++)
    {
        rec_fifo.enqueue(arr.at(i));
    }
    //qDebug("l=%d  (0)=%x",arr.length(),arr.at(0)&0xff);
}

void MainWindow::dataParser(QString &s, float (&AccGyroRaw)[9])
{
    QRegExp reg_pattern("\\[([0-9]{1,2})\\]=(.+),");
    reg_pattern.setMinimal(true);   //非贪婪模式
    int pos=0;
    while((pos=reg_pattern.indexIn(s,pos))!=-1) //找下一个匹配
    {
        pos+=reg_pattern.matchedLength();   //指针往下移动
        QStringList list = reg_pattern.capturedTexts(); //匹配字符串
        //qDebug()<<"reg:"<<QString::number(i)<<"=="<<list.at(i);
        AccGyroRaw[list.at(1).toInt()]=list.at(2).toFloat();
        //qDebug()<<"====";
    }
}

void MainWindow::timerUpdate()
{
    int FRAME_LENGTH = ui->spinBox_frameLength->value();//100;    //Magic Number//"NRF= 0 DevID=34 [0]=-6622.0 [1]=   28.0 [2]=15892.0 [3]= -523.0 [4]=  235.0 [5]=  -40.0\n"
    QString rec_str;
    while(rec_fifo.size()>=FRAME_LENGTH)
    {
        rec_str.resize(FRAME_LENGTH);
        for(int i=0;i<FRAME_LENGTH;i++)
        {
            char c=rec_fifo.dequeue();
            if(c==0x0A && i!=FRAME_LENGTH-1)    //换行符，出现太早，丢弃坏帧
            {
                bad_frame_counter++;
                qDebug()<<rec_str<<"return at"<<QString::number(i)<<"fifo size"<<rec_fifo.size();
                return;
            }
            rec_str[i]=c;
        }
        qDebug()<<rec_str<<"fifo size"<<rec_fifo.size();

        //转化新的到的一帧数据
        float AccGyroRaw[9];
        dataParser(rec_str,AccGyroRaw);
        //qDebug()<<AccGyroRaw[0]<<AccGyroRaw[1]<<AccGyroRaw[2]<<AccGyroRaw[3]<<AccGyroRaw[4]<<AccGyroRaw[5];
        QString debugValueString;
        for(int i=0;i<Channel_Number;i++)
        {
            debugValueString+="["+QString::number(i)+"]="+QString::number(AccGyroRaw[i])+"  ";
        }
        qDebug()<<debugValueString;
        //往前搬移
        for(int i=0;i<PIC_WIDTH-1;i++)
        {
            for(int j=0;j<Channel_Number;j++)
                rec[j][i]=rec[j][i+1];
        }
        //最后一个数据是新的数据
        for(int i=0;i<Channel_Number;i++)
        {
            rec[i][PIC_WIDTH-1]=AccGyroRaw[i];
            //绘制完整的一帧数据
            ui->customPlot->graph(i)->setData(x,rec[i]);
        }
    }
    //全部数据写完以后再重绘
    ui->customPlot->replot();
    //FIFO使用率
    ui->progressBar->setValue(rec_fifo.count());
}

void MainWindow::timerUpdate_indicators()
{
    //错误帧计数
    ui->lcdNumber_badFrame->display(bad_frame_counter);
}

void MainWindow::selectionChanged()
{
    //遍历整个图和图例，如果选中了图或者图例，就选中另一个对应的元素
    for(int i=0;i<ui->customPlot->graphCount();i++)
    {
        QCPGraph *graph = ui->customPlot->graph(i);
        QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
        if(item->selected() || graph->selected())
        {
            item->setSelected(true);
            graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
        }
    }
}

void MainWindow::on_pushButton_autoscale_clicked()
{
    for(int i=0;i<Channel_Number;i++)
        ui->customPlot->graph(i)->rescaleAxes(true);
    ui->customPlot->rescaleAxes(true);
}

void MainWindow::on_pushButton_ShowHide_clicked()
{
    for(int i=0;i<ui->customPlot->graphCount();i++)
    {
        QCPGraph *graph = ui->customPlot->graph(i);
        QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
        if(item->selected() || graph->selected())
        {
            graph->setVisible(!graph->visible());   //隐藏或者显示
        }
    }
}
