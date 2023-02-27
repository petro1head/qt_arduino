#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include "stand/stand.h"
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Запись данных в серийный порт
    void writeData(QString data);
    // Чтение данных из серийного порта
    void readData();
    // Первая передача данных через серийный порт
    // Для задания значений
    void firstTransaction();
    // Записать новые данные в textEdit
    void updateTextEdit(QString data);
    // Показать все данные - угол, скорость, сигнал после отклонения
    void showAllVals();
    // Обновить все данные
    void resetAll();
    // Обновить все порты
    void updatePorts();
    // Настраиваем графики
    void prepareGrafics();
    // Записываем угол и скорость в файл
    void WriteFile();
    // обновление графика фазового портрета
    void updatePhasePortrait();
    // Обновляет временной график
    void updateTimeGrafic();

    // Обработчики событий
    void on_connectButton_clicked();
    void on_clearButton_clicked();
    void on_stopButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serial = nullptr;
    Stand *m_stand = nullptr;
    bool isCursorWasStop = false;
    bool isOK = false;
    bool IsInside = false;
    // Мусор в серийном порте, по умолчанию пустая строка
    QString startTrash = "";

    // для фазового портрета не подойдёт простой график нужна кривая Curve
    QCPCurve *phasePortrait = nullptr;
};
#endif // MAINWINDOW_H
