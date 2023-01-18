#include "stand.h"

/* ПУСТОЙ Конструктор класса */
// Вызывается, когда пишем
// MyTime timer() - создаём новый объект таймера
MyTimer::MyTimer()
{
  // По умолчанию при создании таймера, текущее значение времени = 0
  this->val = 0;
  // Задаём значение по умолчанию 0
  this->delta = 0;
}

/* Запуск таймера */
void MyTimer::start()
{
  this->qtimer.start();
}

/* Обновление таймера */
void MyTimer::update()
{
  // Вычисляем текущее время (от запуска программы)
  unsigned long cur_time = qtimer.elapsed();
  // Вычисляем шаг дискритизации, через какое время мы обновляем таймер
  this->delta = cur_time - this->val;
  // Задаём текущее значение времени
  this->val = cur_time;
}

/* ПУСТОЙ Конструктор класса */
// Вызывается, когда пишем
// Integrator integ() - создаём новый объект, без заданных смещений
Integrator::Integrator()
{
  // По умолчанию смещения = 0
  this->s = 0;
}

/* Конструктор класса со СМЕЩЕНИЯМИ */
// Вызывается, когда пишем
// Integrator integ(1) - создаём новый объект, c заданными отклонениями c_s
Integrator::Integrator(double c_s)
{
  // По умолчанию смещения = 0
  this->s = c_s;
}

/* Подаём на вход сигнал */
// u - значение сигнала на входе в интегратор
// h - дискретность РЕАЛЬНОГО сигнала
void Integrator::in(double in_u, unsigned long h)
{
  this->s += in_u * h * 0.001;
}

/* Выходная линия с интеграла */
double Integrator::out()
{
  return this->s;
}

Gain::Gain()
{
  this->u = 0;
  this->multiplier = 1;
}
/*Конструктор множителя*/
Gain::Gain(double m)
{
  // Задаём значение коэффициента умножения
  this->multiplier = m;
}

/* На входе подаем значение сигнала */
void Gain::in(double in_u)
{
  this->u = in_u * this->multiplier;
}

/* Выходная линия множителя */
double Gain::out()
{
  return this->u;
}

Deviation::Deviation()
{
  this->u = 0;
  this->dev = 0.001;
}

Deviation::Deviation(double def_dev)
{
  this->u = 0;
  this->dev = def_dev;
}

/* На входе подаём сигнал */
void Deviation::in(double u_in)
{
  this->u = u_in;
}

/* Выходная линия с сумматора */
double Deviation::out()
{
  return this->u + this->dev;
}

Stand::Stand()
{
  // имеет свой таймер
  this->timer = MyTimer();
  // Множитель
  this->gain = Gain(0.5);
  // Отклонение (возмущение)
  this->dev = Deviation();
  // Интеграл скорости
  this->speed = Integrator(3);
  // Интеграл угла
  this->angle = Integrator();
}

// при старте стенда
void Stand::start()
{
  // Стартуем таймер
  this->timer.start();
}

// Данные на входе в стенд
void Stand::in(QString data)
{
  // data - строка с одним числом
  // data = "15.555"
  double u_in = data.toDouble();
  // Делаем прямой проход по стенду
  this->run(u_in);
}

void Stand::run(double u_in)
{
  // обновляем таймер
  this->timer.update();
  // входной сигнал сначала передаём в множитель
  this->gain.in(u_in);
  // передаём сигнал Отклонению (возмущению)
  this->dev.in(this->gain.out());
  // Дальше выход из объекта возмущения передаём
  // на вход интегратору скороти
  this->speed.in(this->dev.out(), this->timer.delta);
  // Обновлённую скорость передаём углу
  this->angle.in(this->speed.out(), this->timer.delta);
}

// На выходе получаем строку
QString Stand::out()
{
  // На выходе мы должны вернуть время и скорость
  QString tStr = QString::number(this->timer.val);
  QString speedStr = QString::number(this->speed.s);
 // QString angleStr = QString::number(this->angle.s);
  return tStr + ";" + speedStr;
}
