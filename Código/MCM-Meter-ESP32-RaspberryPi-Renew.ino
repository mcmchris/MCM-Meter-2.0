/*
 * Proyecto: Medidor energético inteligente 2.0
 * Autor: Christopher Mendez
 * Documentación: https://youtube.com/mcmchris
 */
 
//Librerias para el manejo de Archivos.
#include "FS.h"
#include "SPIFFS.h"

//Librerias para la comunicacion WiFi
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>

//Libreria para calculos de consumo
#include "EmonLib.h"

//Libreria del RTC (Reloj de tiempo real)
//#include "RTClib.h"

//Librerias de la pantalla OLED
#include <Adafruit_SSD1327.h>

#define FORMAT_SPIFFS_IF_FAILED true

// Usado para el SPI por software o hardware
#define OLED_CS A12
#define OLED_DC A11

// Usado por I2C o SPI
#define OLED_RESET -1

// Hardware SPI de la pantalla
Adafruit_SSD1327 display(128, 128, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// I2C
//Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET, 1000000); // Si prefieres la pantalla por I2C


EnergyMonitor emon1;             // Instancia de Emonlib

WiFiManager wm;                  // Instancia de el WiFi Manager

//RTC_DS3231 rtc;                // Instancia del RTC (si lo necesitas)

const int boton = A8;            // Pin del pulsador
const int LED = A7;              // Pin del LED de estado
const int modo = A6;             // Pin del switch de 120/240v

// URL con la IP y la dirección a tu sensor
char* serverName = "http://192.168.100.7:8123/api/states/sensor.medidor_ai"; // Para comunicación local, actualizar con el IP de tu Raspberry Pi

unsigned long previousMillis = 0;
const long interval = 1000;           // Intervalo de envio en ms

unsigned long lastTime = 0;
float V, KW, VA, COSPHI, I, Vbat, KWH, VCal, ICal, segs, segundos;
int reads, level, estado, bye = 1, httpResponseCode, estadoBoton = 0, tiempo, extra = 0;
unsigned int KWHT, contador;
long divir, timer, epoch;
bool energia = false;
String hora;
byte seguro = 0, ocupado = 1, portal = 0;

String token, recuerdo;

String consumo, clavis = "*****************************************************"; // Generar una clave de larga duración como se muestra en el video

// Todas las tareas creadas
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;

// Parametros personalizados que agregamos el WiFi manager
WiFiManagerParameter actual_lecture("contador", "KW/h-Contador", "", 20);


// Función que extrae el valor de consumo historico del contador del sistema de archivos
void readConsumo(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from Consumo:");
  while (file.available()) {
    consumo += (char)file.read();

  }
  Serial.println(consumo);
  contador = consumo.toInt();
  Serial.println("Este es el numero: ");
  Serial.println(contador);
}

// Función que extrae el valor de consumo hasta la fecha del sistema de archivos
void readKWH(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from KWH:");

  while (file.available()) {
    recuerdo += (char)file.read();
  }
  //KWH = atof(recuerdo);
  Serial.println(recuerdo);
  KWH = recuerdo.toFloat();
  Serial.println("Este es el numero: ");
  Serial.println(KWH, 11);
}

// Función para escribir en el sistema de archivos
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Función para leer el ADC e intentar linealizar la lectura (voltaje de bateria)
double ReadVoltage(byte pin) {
  double reading = analogRead(pin);
  if (reading < 1 || reading > 4095) return 0;
  return -0.000000000009824 * pow(reading, 3) + 0.000000016557283 * pow(reading, 2) + 0.000854596860691 * reading + 0.092440348345433;
  //return -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
} // Added an improved polynomial, use either, comment out as required

// Función que el WiFi manager usa para capturar lo que se escribe en los Textbox de la interface
void saveParamsCallback () {
  Serial.println("Obtener consumo:");
  Serial.print(actual_lecture.getID());
  Serial.print(" : ");
  consumo = actual_lecture.getValue();
  Serial.println(consumo);
  contador = consumo.toInt();
  writeFile(SPIFFS, "/registro.txt", (char*)consumo.c_str());   //Convierto el string consumo en un Char Array
}

void setup()   {
  WiFi.mode(WIFI_STA); // configuramos el WiFi en modo access point y estación
  Serial.begin(115200);

  //Iniciando la pantalla
  Serial.println("SSD1327 OLED probandose");
  if ( ! display.begin(0x3D) ) {
    Serial.println("OLED no puede iniciar");
    //while (1) yield();
  }
  display.setRotation(0);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(10, 60);
  display.println("INICIANDO");
  display.display();


#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  /*if (! rtc.begin()) {
    Serial.println("RTC no encontrado");
    Serial.flush();
    abort();
    }

    if (rtc.lostPower()) {
    Serial.println("RTC perdió la alimentación");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }*/

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Montaje de SPIFFS falló");
    return;
  }

  pinMode(LED, OUTPUT);
  pinMode(modo, INPUT);
  pinMode(boton, INPUT);


  readConsumo(SPIFFS, "/registro.txt");
  readKWH(SPIFFS, "/recuerdo.txt");

  wm.addParameter(&actual_lecture);

  wm.setConfigPortalBlocking(false);
  wm.setWiFiAutoReconnect(true);
  wm.setSaveParamsCallback(saveParamsCallback);

  //wm.resetSettings(); // borrar la config del WiFi
  wm.setConnectTimeout(10); // Cuanto tiempo durar intentando conectarnos al WiFi antes de continuar
  wm.setConfigPortalTimeout(60); // Cerrar el portal de configuración despues n segundos

  // Configuración de parámetros de calibración del sistema a 120v o a 240v
  if (digitalRead(modo) == 0) { //120v
    VCal = 165.1;
    ICal = 111.1;
  }
  else if (digitalRead(modo) == 1) { //240v
    VCal = 164.7;
    ICal = 99.6;
  }

  emon1.voltage(A2, VCal, 1.7);  // Voltaje: input pin, calibration, phase_shift
  emon1.current(A3, ICal);       // Corriente: input pin, calibration.

  bool res;

  res = wm.autoConnect("ESP32-MCMeter"); // nombre de la red que se genera para el portal e intento de reconexión

  if (!res) {
    Serial.println("Fallo de conexión o timeout");
    presionaron("NO WIFI", 2);
    portal = 0;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //ESP.restart();
    estado = 2;
  }
  else {
    //si estamos aquí es porque estamos conectados al WiFi
    presionaron("CONECTADO", 2);
    portal = 1;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("conectado...yeey :)");
    estado = 1;
  }

  //Configuraciones de las tareas del RTOS (nombre, stack size, prioridad, nucleo, etc)
  xTaskCreatePinnedToCore(
    medir,   /* Task function. */
    "Task1",     /* name of task. */
    1000,      /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    envioUbidots,   /* Task function. */
    "Task2",     /* name of task. */
    5000,      /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

  xTaskCreatePinnedToCore(
    estadoLED,   /* Task function. */
    "Task3",     /* name of task. */
    1000,      /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &Task3,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

  xTaskCreatePinnedToCore(
    wifiCall,   /* Task function. */
    "Task4",     /* name of task. */
    10000,      /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task4,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

  xTaskCreatePinnedToCore(
    pantalla,   /* Task function. */
    "Task5",     /* name of task. */
    5000,      /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &Task5,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

}


void loop() {
  //Como estamos trabajando con RTOS el Loop que acostumbrados a llenar de código, lo dejamos vacío, confien
}

void medir(void * parameter) {
  //UBaseType_t uxHighWaterMark;

  for (;;) { // infinite loop
    //Serial.print("Medir está corriendo en el nucleo: ");
    //Serial.println(xPortGetCoreID());
    Vbat = ReadVoltage(35) * 2;          //Leo el valor de voltaje de la bateria
    timer  = micros();                   //guardo el tiempo en ms que ha pasado

    emon1.calcVI(120, 2000);             //Promedio las lecturas de 120/2 ciclos de la senoidal, 2 segundos de time-out

    divir = micros() - timer;            //calculo cuánto duró la función calcVI en microsegs
    segs = divir / 1000000.0;            //convierto divir en segundos
    KW        = emon1.realPower;         //extract Real Power into variable

    if (energia) { // si hay energía eléctrica integro

      KWH += KW / (((float)1 / segs) * 3600000.0);
      KWHT = round(KWH) + contador;

    } else { // si no hay energía eléctrica, pues no hay porque calcular rápido, pongamos un delay

      vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    //Serial.println("Segundos que tardo en calcular: " + String(segs, 11));
    //Serial.println(KWH, 11);
    //Serial.println(KWHT);

    VA        = emon1.apparentPower;    //extract Apparent Power into variable
    COSPHI    = emon1.powerFactor;      //extract Power Factor into Variable
    V         = emon1.Vrms;             //extract Vrms into Variable
    I         = emon1.Irms;             //extract Irms into Variable
    reads     = emon1.readings;

    vTaskDelay(1 / portTICK_PERIOD_MS);
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.print("Stack en salida 'medir': "); Serial.println(uxHighWaterMark);
  }
}

void envioUbidots( void * parameter) {

  //UBaseType_t uxHighWaterMark;

  for (;;) {

    wm.process();

    if (WiFi.status() == !WL_CONNECTED) {
      bool res;

      res = wm.autoConnect("ESP32-MCMeter"); // anonymous ap

      if (!res) {
        Serial.println("Fallo de conexión o timeout");
        portal = 0;
        presionaron("NO WIFI", 2);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //ESP.restart();
        estado = 2;
      }
      else {
        //if you get here you have connected to the WiFi
        portal = 1;
        presionaron("CONECTADO", 2);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("conectado...yeey :)");
        estado = 1;
      }
    }

    if ((V < 70)) {

      if (bye == 1) {

        writeFile(SPIFFS, "/recuerdo.txt", (char*)String(KWH, 11).c_str());  //Convierto el string KWH en un Char Array
        bye = 0;

      }
      energia = false;
      estado = 4;

    } else {

      bye = 1;
      estado = 3;
      energia = true;
    }

    unsigned long currentMillis = millis();

    //    DateTime now = rtc.now();
    //    char buf1[] = "hh:mm";
    //    hora = now.toString(buf1);
    //    epoch = now.unixtime();

    // Cada intervalo envio un post HTTP
    if ((currentMillis - previousMillis >= interval)&& energia && portal
    ) { // 
      previousMillis = currentMillis;

      HTTPClient http;
      // Your Domain name with URL path or IP address with path
      http.begin(serverName);

      String valores = "{\"entity_id\":\"sensor.medidor_ai\",\"state\":\"1\",\"attributes\":{\"corriente\":\"" + String(I) + "\",\"voltaje\":\"" + String(V) + "\",\"potencia\":\"" + String(KW) + "\",\"fp\":\"" + String(COSPHI) + "\",\"bateria\":\"" + String(Vbat) + "\",\"energia\":\"" + String(KWHT) + "\"}}";

      http.addHeader("authorization", clavis);
      http.addHeader("content-type", "application/json");

      httpResponseCode = http.POST(valores);

      //Serial.print(clavis);
      Serial.print("Respuesta HTTP: ");
      Serial.println(httpResponseCode);
      // Libero recursos
      http.end();

    }
    vTaskDelay(1 / portTICK_PERIOD_MS);

    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.print("Stack en salida 'EnvioUbidots': "); Serial.println(uxHighWaterMark);
  }
}

//Función que intenta indicar con parpadeos del LED el estado del sistema (mejorable)
void estadoLED( void * parameter) {
  //UBaseType_t uxHighWaterMark;

  for (;;) {

    switch (estado) {
      case 1:                             //Parpadeo rapido
        for (int i = 0; i < 50; i++) {
          level = !level;
          digitalWrite(LED, level);
          vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        estado = 2;                       
        break;

      case 2:                             //No hay wifi
        level = 1;
        digitalWrite(LED, level);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        break;

      case 3:                             //Hay Luz
        level = !level;
        digitalWrite(LED, level);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        break;

      case 4:                             //No hay Luz
        level = !level;
        digitalWrite(LED, level);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;
    }
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.print("Stack en salida 'LED': "); Serial.println(uxHighWaterMark);
  }
}

//Función que maneja el pulsador
void wifiCall( void * parameter) {

  //UBaseType_t uxHighWaterMark;


  for (;;) {

    estadoBoton = digitalRead(boton); // leer la entrada del pulsador

    tiempo = millis();

    while (!estadoBoton) {
      ocupado = 0;
      estadoBoton = digitalRead(boton); //  leer la entrada del pulsador
      segundos = (millis() - tiempo) / 1000.0;
      presionaron(String(segundos, 0), 2); // imprimimos el tiempo en segundos que duramos presionando el pulsador
      seguro = 1;
    }

    if (estadoBoton && seguro) {
      Serial.println(segundos);
      seguro = 0;
    }

    if (segundos >= 0.1 && segundos <= 2.0) { ///////////////////////////////////////////////////////////

      Serial.println("Iniciando portal de config");

      WiFiManager wm;                  // Global wm instance
      portal = 0;
      presionaron("PORTAL", 2);

      wm.setConfigPortalTimeout(45);

      if (!wm.startConfigPortal("ESP32-MCMeter")) {
        Serial.println("failed to connect or hit timeout");
        presionaron("NO WIFI", 2);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ocupado = 1;
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        portal = 1;
        presionaron("CONECTADO", 2);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("connected...yeey :)");
        estado = 1;
        ocupado = 1;
      }
      segundos = 0;
    } else {
      ocupado = 1;
    }

    if (segundos >= 3.0 && segundos <= 5.0) { ///////////////////////////////////////////////////////////////
      presionaron("CERO", 2);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      KWH = 0;
      writeFile(SPIFFS, "/recuerdo.txt", (char*)String(KWH, 11).c_str());  //Convierto el string KWH en un Char Array
      wm.resetSettings();
      segundos = 0;
      ocupado = 1;
    } else {
      ocupado = 1;
    }

    if (segundos >= 7.0) { //////////////////////////////////////////////////////////////////////////////////
      writeFile(SPIFFS, "/recuerdo.txt", (char*)String(KWH, 11).c_str());  //Convierto el string KWH en un Char Array
      presionaron("RESET", 2);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      ESP.restart();
    } else {
      ocupado = 1;
    }

    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.print("Stack en salida 'WifiCall': "); Serial.println(uxHighWaterMark);
  }
}

//Función que va mostrando los valores energéticos en pantalla
void pantalla(void * parameter) {

  //UBaseType_t uxHighWaterMark;

  for (;;) {
    if (ocupado == 1) {
      display.clearDisplay();
      // text display tests
      display.setTextSize(1);
      display.setTextColor(SSD1327_WHITE);
      display.setCursor(103, 5);
      if (WiFi.status() == WL_CONNECTED)display.println("WiFi");
      display.setCursor(95, 15);
      display.println(String(Vbat) + "V");
      display.setTextSize(2);
      display.setCursor(0, 15);
      display.println(String(V, 1) + " V");
      display.println(String(I, 1) + " A");
      display.println(String(KW, 1) + " W");
      display.println(String(KWH, 0) + " Kwh");
      display.println(String(KWHT) + " Kwh");
      display.setTextSize(1);
      display.setCursor(80, 108);
      if (energia) {
        display.println("Luz");
      } else {
        display.println("No Luz");
      }

      //display.setCursor(95, 108);
      //display.println(hora);
      display.setCursor(80, 118);
      display.println("http:" + String(httpResponseCode));
      display.display();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.print("Stack en salida 'pantalla': "); Serial.println(uxHighWaterMark);
  }
}

//Función que muestra en pantalla (texto y tamaño)
void presionaron(String text, int tama) {
  display.clearDisplay();
  // text display tests
  display.setTextSize(tama);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(map(text.length(), 1, 12, 54, 0), 60);
  //
  display.println(text);
  display.display();
}
