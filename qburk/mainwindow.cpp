#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stand/stand.h"
#include <QString>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QtDebug>
#include <QTimer>
#include <QMessageBox>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          m_serial(new QSerialPort(this)),
                                          m_stand(new Stand)

{
    ui->setupUi(this);
    // добавлям в комбобокс все известные COM порты
    this->updatePorts();
    // Настраиваем графики
    this->prepareGrafics();
}

MainWindow::~MainWindow()
{
    delete ui;
    this->m_serial->close();
}

void MainWindow::updateTextEdit(QString data)
{
    QTextCursor c = ui->textEdit->textCursor();
    // Запоминимаем последнюю позицию курсора перед вставкой новых данных
    int last_pos = c.position();
    // Если включен флаг отображения времени
    if (ui->timeShowBox->isChecked())
    {
        // Добавляем к данным текущее время
        data = QString::number(this->m_stand->timer.val * 0.001) + " -> " + data;
    }
    // Добавляем данные в textEdit
    ui->textEdit->append(data);

    // Если включена опция автоскролинга
    if (ui->autoScrolBox->isChecked())
    {
        // Ставим курсор в самый конец
        c.movePosition(QTextCursor::End);
        this->isCursorWasStop = false;
    }
    else if (!this->isCursorWasStop)
    {
        // Если нет автоскрола то ставим курсор в предыдущую позицию
        // При этом это надо сделать только один раз
        // для этого у нас есть флаг this->isCursorWasStop
        c.setPosition(last_pos);
        this->isCursorWasStop = true;
    }
    ui->textEdit->setTextCursor(c);
}

void MainWindow::showAllVals()
{
    // Если это первый раз,то сначала выведем названия того, что выводим
    if (this->m_stand->timer.val == 0)
    {
        // скорость, угол и сигнал после возмущения (отклонения)
        ui->textEdit->append("speed , angle , dev");
    }

    // Получаем значение скорости
    QString speedStr = QString::number(this->m_stand->speed.out());
    // Получаем значение угла
    QString angleStr = QString::number(this->m_stand->angle.out());
    // Сигнал после объекта возмущения (отклонения)
    QString devStr = QString::number(this->m_stand->dev.out());

    // Выводим все значения в textEdit
    this->updateTextEdit(speedStr + " , " + angleStr + " , " + devStr);
    // Вывод в qDebug значения скорости и угла
    // qDebug() << speedStr + " , " + angleStr ;

}

void MainWindow::updatePorts()
{
    // очищаем комбобокс с выбранными портами
    ui->comboBox->clear();
    // добавлям в комбобокс все известные COM порты
    for (QSerialPortInfo &myport : QSerialPortInfo::availablePorts())
    {
        ui->comboBox->addItem(myport.portName());
    }
  ui->comboBox->setCurrentIndex(0);
}

/*Нажали на кнопку Подключиться*/
void MainWindow::on_connectButton_clicked()
{
    // Перенастраиваем графики
    this->prepareGrafics();

    // Сбрасываем все данные
    this->resetAll();
    this->m_serial->setPortName(ui->comboBox->currentText());
    this->m_serial->setBaudRate(QSerialPort::Baud115200);
    this->m_serial->setDataBits(QSerialPort::Data8);
    this->m_serial->setParity(QSerialPort::NoParity);
    this->m_serial->setStopBits(QSerialPort::OneStop);
    this->m_serial->setFlowControl(QSerialPort::NoFlowControl);
    this->m_serial->open(QIODevice::ReadWrite);
    connect(this->m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    ui->textEdit->clear();
    ui->PBIsInside->setValue(0);
    ui->textEdit_PassedTheTest->clear();
    ui->textEdit_Time->clear();

    ui->progressBar->setValue(50);

    // Дальше ждём от 1907вм014 сигнал, что всё ОК
}

/*В первой транзакции мы*/
void MainWindow::firstTransaction()
{
    // Задаёмся отклонением (возмущением) с формы
    this->m_stand->dev.dev = ui->devBox->value();
    // Впервые рисуем график фазового портрета (ставим начальные точки)
    this->updatePhasePortrait();
    // Задаём значение начальной скорости полученной с формы
    this->m_stand->speed.s = ui->speedBox->value();
    // Запускаем таймер стенда
    this->m_stand->timer.start();
    // Записываем данные в последовательный порт
    this->writeData(this->m_stand->out());
    // Ставим прогрес бар на 100
    ui->progressBar->setValue(100);
    // Смотрим в отладчике первую посылку "tStr;speedStr"
    // в самом начале время 0, а скорость берется с формы Qt
    // если скорость на форме 3 градуса/с, то
    // вывод в qDebug будет выглядеть так "0;3"
    //qDebug() << this->m_stand->out();
}

void MainWindow::readData()
{
    // Если еще не получали ответ от 1907вм014, что всё ок
    if (!this->isOK)
    {
        // всё, что есть в последовательном порте
        // добавляем к нашей мусорной переменной
        this->startTrash += this->m_serial->readAll();

        // хоть мы всегда и очищаем последовательный порт, иногда там могут
        // появляться мосурные данные, с предыдущих запусков или тп
        // И иногда в одной строке с мусором может быть сигнал OK
        // Или сначала идёт мусор, а потом на следующей строке сигнал ОК
        // Поэтому чтобы избежать ошибок вместо readLine используем readAll
        // И прибавляем к старому мусору, вдруг OK записалось в порт по одной букве

        // дальше нужно проверить есть ли среди всего этого наш
        // заветный сигнал ОК - что мы можем начинать работу
        // проверяем содержит ли весь собранный нами мусор строку "OK"
        if (this->startTrash.contains("OK"))
        {
            // Ура мы получили сигнал OK от 1907вм014, выводим это в дебаг
            qDebug() << "OK";
            // Меняем значенеи флага - говорим, что получили сигнал OK от 1907вм014
            this->isOK = true;
            // Берём данные с формы и отправляем на 1907вм014
            // Впервые отправляем данные со стенда
            this->firstTransaction();
        }

        // Если в мусоре нет сигнала OK значит ждём когда в последовательном порте появятся
        // новые данные, из-за чего вызовется функция ReadData и мы попадём сюда
    }
    else
    {
        // Если мы здесь, то мы уже получили синал OK от 1907вм014
        // Отпарвили ей данные со стенда, 1907вм014 эти данные отработала
        // И вернула какой-то сигнал, который мы дальше будем обрабатывать в этом блоке

        // Сначала надо проверить, можем ли считать данные с 1907вм014
        // данные с 1907вм014 передаются построчно
        // То есть данные будут верными если мы сможем считать строку
        // Если мы можем считать строку
        if (this->m_serial->canReadLine())
        {
            // Получаем строку с ардунио
            QString data = this->m_serial->readLine();
            // очищаем строку от пробелов
            data = data.trimmed();
            // qDebug() << data << data.toDouble();
            // Обновим фазовый портрет
            this->updatePhasePortrait();
            // Выведем данные в текст эдит
            this->showAllVals();
            // Обновим график значений от времени
            this->updateTimeGrafic();

            // Теперь новые данные с 1907вм014 передадим стенду на вход
            // Тем самым осуществляем проход по стенду и обновление его параметров
            // времени, скорости
            this->m_stand->in(data);
            // Обновлённые данные со стенда печатаем обратно в последовательный порт
            this->writeData(this->m_stand->out());
            if (!this->IsInside)
                {
                    if (abs(this->m_stand->speed.s) <= 0.03 && abs(this->m_stand->angle.s) <=0.125)
                    {
                         // Ставим прогрес бар на 100
                         ui->PBIsInside->setValue(100);
                         // Меняем значенеи флага - внутри заданной по ТЗ области
                         this->IsInside = true;
                         QString time = QString::number(this->m_stand->timer.val * 0.001);
                         // Добавляем данные в textEdit_Time
                         ui->textEdit_Time->append(time);
                         // Выводим в textEdit "Тест пройден!"
                         ui->textEdit_PassedTheTest->setText("Тест пройден!");
                         // Выводим сообщение о прохождении теста
                         QMessageBox::information(this,"Сообщение", "Попали в заданную область\nТест пройден!");
                    }
                }
        }

        // Если мы не можем считать строку, то ждём, когда в последовательном порте появятся
        // новые данные, из-за чего вызовется функция ReadData и мы попадём сюда
    }
}

void MainWindow::writeData(QString data)
{
    // Очищаем данные от пробельных символов и добавляем перевод на новую строку
    data = data.trimmed() + "\n";
    // преобразуем строку в байтовый вид, который уже потом можно записать в 1907вм014
    QByteArray inBytes = data.toUtf8();
    const char *cStrByte = inBytes.constData();
    // Записываем данные в 1907вм014
    this->m_serial->write(cStrByte);
}

void MainWindow::resetAll()
{
    // обновляем список известных портов
    this->updatePorts();
    // Если последовательный порт открыт
    if (this->m_serial->isOpen())
    {
        // Очищаем последовательный порт
        this->m_serial->clear();
        // закрываем последовательный порт
        this->m_serial->close();
    }
    // Очищаем память после него
    delete this->m_serial;
    // Создаём новый
    m_serial = new QSerialPort(this);
    // Очищаем память после стенда
    delete this->m_stand;
    // Создаём новый стенд
    this->m_stand = new Stand();
    // Сбрасываем прогресc бар
    ui->progressBar->setValue(0);
    this->IsInside = false;
    // Говорим, что синал еще не получали
    this->isOK = false;
    // Для курсора
    this->isCursorWasStop = false;
    // Очищаем переменную с мусором из последовательного порта
    this->startTrash = "";
}

void MainWindow::on_clearButton_clicked()
{
    ui->textEdit->clear();
    ui->textEdit_PassedTheTest->clear();
    ui->PBIsInside->setValue(0);
    ui->textEdit_Time->clear();
}

void MainWindow::on_stopButton_clicked()
{
    // Отправляем 1907вм014 команду остановиться
    // this->writeData("stop");
    // Сбрасываем все значения формы
    this->resetAll();
}

void MainWindow::prepareGrafics()
{
    // Настраиваем графики:
    // Сначала очистим оба графика
    ui->widget->clearPlottables();
    ui->widget_2->clearPlottables();

    /*widget Фазовый портрет*/

    // можно перетаскивать график (взаимодействие перетаскивания графика)
    ui->widget->setInteraction(QCP::iRangeDrag,true);

    // можно зумировать график (взаимодействие удаления / приближения графика)
    ui->widget->setInteraction(QCP::iRangeZoom,true);

    //Вкл легенду
    ui->widget->legend->setVisible(true);
    // легенду в правый верхний угол графика
    ui->widget->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignRight|Qt::AlignTop);

    // задаем имена осей координат
    ui->widget->xAxis->setLabel("Angle");
    ui->widget->yAxis->setLabel("Speed");

    // для фазового портрета не подойдёт простой график нужна кривая Curve
    this-> phasePortrait = new QCPCurve(ui->widget->xAxis, ui->widget->yAxis);

    this->phasePortrait->setPen(QPen(Qt::black));
    this->phasePortrait->setName("Phase portrait");


    //задаем размеры осей
    ui->widget->xAxis->setRange(-10,10);
    ui->widget->yAxis->setRange(-5,5);

    //нулевое значение по осям Х и У рисуем толстой линией
    QPen qAxisPen;
    qAxisPen.setWidth(2.);
    ui->widget->xAxis->grid()->setZeroLinePen(qAxisPen);
    ui->widget->yAxis->grid()->setZeroLinePen(qAxisPen);

    //рисуем сами графики
    ui->widget->replot();


    /*widget_2 График скорости,угла и управления от времени*/

    // можно перетаскивать график (взаимодействие перетаскивания графика)
    ui->widget_2->setInteraction(QCP::iRangeDrag,true);

    // можно зумировать график (взаимодействие удаления / приближения графика)
    ui->widget_2->setInteraction(QCP::iRangeZoom,true);

   if (ui->speedBox->value() > 0)
    {
        //Вкл легенду
        ui->widget_2->legend->setVisible(true);
        // легенду в правый верхний угол графика
        ui->widget_2->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignRight|Qt::AlignTop);
    }
   else
   {
       //Вкл легенду
       ui->widget_2->legend->setVisible(true);
       // легенду в правый верхний угол графика
       ui->widget_2->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignRight|Qt::AlignBottom);
   }
    // первый график СКОРОСТЬ
    ui->widget_2->addGraph(ui->widget_2->xAxis, ui->widget_2->yAxis);
    ui->widget_2->graph(0)->setPen(QPen(Qt::green));
    ui->widget_2->graph(0)->setName("Speed");

    // второй график УГОЛ
    ui->widget_2->addGraph(ui->widget_2->xAxis, ui->widget_2->yAxis);
    ui->widget_2->graph(1)->setPen(QPen(Qt::red));
    ui->widget_2->graph(1)->setName("Angle");

    // третий график УПРАВЛЕНИЕ U
    ui->widget_2->addGraph(ui->widget_2->xAxis, ui->widget_2->yAxis);
    ui->widget_2->graph(2)->setPen(QPen(Qt::blue));
    ui->widget_2->graph(2)->setName("U");

    // задаем имена осей координат
    ui->widget_2->xAxis->setLabel("Time");
    ui->widget_2->yAxis->setLabel("Speed, Angle, U");

    //задаем размеры осей
    ui->widget_2->xAxis->setRange(0,70);
    ui->widget_2->yAxis->setRange(-5,10);

    //нулевое значение по осям Х и У рисуем толстой линией
    QPen qAxisPen_2;
    qAxisPen_2.setWidth(2.);
    ui->widget_2->xAxis->grid()->setZeroLinePen(qAxisPen_2);
    ui->widget_2->yAxis->grid()->setZeroLinePen(qAxisPen_2);

    //рисуем сами графики
    ui->widget_2->replot();
}

void MainWindow::updatePhasePortrait()
{
    // По оси x - угол
    double x = this->m_stand->angle.s;
    // По оси y - скорость
    double y = this->m_stand->speed.s;
    // Добавляем данные по одной точке
    this->phasePortrait->addData(x, y);
    // ui->widget->rescaleAxes();
    ui->widget->replot();
}

void MainWindow::updateTimeGrafic()
{
    // текущее время в секундах
    double t = this->m_stand->timer.val*0.001;
    // Получаем значение скорости
    double speed = this->m_stand->speed.out();
    // обновляем график скорости
    ui->widget_2->graph(0)->addData(t, speed);
    // Получаем значение угла
    double angle = this->m_stand->angle.out();
    // обновляем график угла
    ui->widget_2->graph(1)->addData(t, angle);
    // Сигнал после объекта возмущения (отклонения)
    double dev = this->m_stand->dev.out();
    // обновялем график U
    ui->widget_2->graph(2)->addData(t, dev);

    // масштабируем график
    ui->widget_2->rescaleAxes();
    ui->widget_2->yAxis->padding();
    // Перерисовываем график
    ui->widget_2->replot();
}
