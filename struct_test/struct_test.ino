typedef struct Data {
  unsigned long t = 0;
  double speed = 0;
} Data;


byte readed = 0;
// получаем структуру
Data qt_data;

bool get_data() {
  char* bWrite = (char*)&qt_data;
  bWrite += readed;

  while (Serial.available()) {
    if (readed >= sizeof(Data)) {
      readed = 0;
      return true;
    }

    *bWrite++ = Serial.read();
    readed++;
  }
  return false;
}

void setup() {
  // Настройка вывода
  // инициирует последовательное соединение
  // и задает скорость передачи данных в бит/с (бод)
  Serial.begin(115200);
  // Отправляем сигнал, что готовы к работе
  Serial.println("OK");
  // Serial.println(sizeof(qt_data));
}

void loop() {
  if (Serial.available()) {
    if (get_data()) {
      Serial.println(qt_data.speed, 5);
      Serial.flush();
    } else {
      Serial.println("B");
    }
  }
}