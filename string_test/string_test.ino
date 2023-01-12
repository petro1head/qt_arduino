#include <string.h>

typedef struct Buffer {
  int len = 0;
  char str[100];
} Buffer;

int myIndex(Buffer *buf, char symbol)
{
  for(int i = 0; i < buf->len; i++)
  {
    if (buf->str[i] == symbol)
    {
      return i;
    }
  }
  return -1;
}

int parse(unsigned int * t, double *speed, Buffer * buf)
{   
  char * space = NULL;
  char t_str[100];
  char speed_str[100];
  int i = myIndex(buf, ' '); 
  if (i != -1)
  {
    // получаем строку времени - всё что до пробела
    strncpy(t_str, buf->str, i);
    // преобразуем в целое числа
    *t = atoi(t_str);

    // получаем скорость в виде строки
    // используем указатель на символ после пробела
    strcpy(speed_str, &buf->str[i+1]);
    // преобразуем в вещественное число   
    *speed = atof(speed_str);

    return 1;
  }

  return 0;
}

void get_data(Buffer *buf)
{
  buf->len = Serial.readBytesUntil('\n', buf->str, sizeof buf->str);
  buf->str[buf->len] = "\0";
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Отправляем сигнал, что готовы к работе
  Serial.println("OK");
}

void loop() {
  
  Buffer buf;
  unsigned int t = 0;
  double speed = 0;
  // check if data is available
  if (Serial.available() > 0) {
    // получили данные с ПК
    get_data(&buf);
    // парсим данные
    if (parse(&t, &speed, &buf) > 0)
    {
      Serial.print("I received: ");
      Serial.print(t);
      Serial.print(" ");
      Serial.println(speed);          
    } else
    {
      Serial.println("Data broken");
    }   
  }
}
