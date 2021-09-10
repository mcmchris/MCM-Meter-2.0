# MCM-Meter-2.0
Se trata de un medidor energético desarrollado entorno al microcontrolador ESP32 (Adafruit ESP32 Feather) capaz de medir energía en sistemas domésticos monofásicos de 120 o 240 voltios (fase partida) monitoreando dos potenciales con un solo medidor de corriente (CT no invasivo) y un sensor de voltaje (PT), la información capturada se transmite a un servidor montado en una Raspberry Pi con Home Assistant que te permitirá monitorear los datos en tiempo real, desde un ordenador o teléfono móvil desde cualquier lugar del mundo.

## English
It is an energy meter developed around the ESP32 microcontroller (Adafruit ESP32 Feather) capable of measuring energy in 120 or 240 volt single-phase domestic systems (split phase) monitoring two potentials with a single current meter (non-invasive CT) and a voltage sensor (PT), the information captured is transmitted to a server mounted on a Raspberry Pi with Home Assistant that will allow you to monitor the data in real time, from a computer or mobile phone from anywhere in the world.

## Instrucciones para preparar el entorno de trabajo:
* Instalar el IDE de [Arduino](https://www.arduino.cc/en/software)
* Instalar el paquete de tarjetas del ESP32 copiando este URL en preferencias del IDE: https://dl.espressif.com/dl/package_esp32_index.json
* Desplegar tu servidor en Home Assistant, [Tutorial](https://youtu.be/gT-4OKOa-1c)
* Clonar este repositorio
* Instalar las librerías ZIP adjuntadas
* Abrir el código de Arduino
* Modificar la variable **Clavis** con un Token de larga duración generado en tu Home Assistant
* Modificar la variable **serverName** con "http://(IP de tu Raspberry Pi):8123/api/states/sensor.medidor_ai", Ej: "http://192.168.100.7:8123/api/states/sensor.medidor_ai"
* Conectar tu ESP32 y cargar el programa en la placa

## English
## Instructions to prepare the work environment:
* Install the [Arduino] IDE (https://www.arduino.cc/en/software)
* Install the ESP32 card package by copying this URL in IDE preferences: https://dl.espressif.com/dl/package_esp32_index.json
* Deploy your server in Home Assistant, [Tutorial] (https://youtu.be/gT-4OKOa-1c)
* Clone this repository
* Install the attached ZIP libraries
* Open the Arduino code
* Modify the **Clavis** variable with a long-term Token generated in your Home Assistant
* Modify the variable **serverName** with "http://(IP of your Raspberry Pi):8123/api/states/sensor.meter_ai", Ex: "http://192.168.100.7:8123/api/states/sensor.meter_ai "
* Connect your ESP32 and load the program on the board

## Seguir el [video tutorial](https://youtu.be/efSfnv4YqtY) para configurar el Home Assistant | Follow the [video tutorial](https://youtu.be/efSfnv4YqtY) to configure the Home Assistant

* Recomiendo configurar una IP estática para tu Raspberry Pi en las configuraciones del Router.
* Copiar y pegar estos datos en el **configuration.yaml**:

* I recommend setting a static IP for your Raspberry Pi in the Router settings.
* Copy and paste this data in the **configuration.yaml**:
```
influxdb:
  host: a0d7b954-influxdb
  port: 8086
  database: homeassistant
  username: homeassistant
  password: *********** (aquí poner una clave)
  max_retries: 3
  default_measurement: state
  
sensor:
  - platform: rest
    name: Medidor AI
    json_attributes:
      - corriente
      - voltaje
      - potencia
      - energia
      - fp
      - bateria
    resource: http://(IP de tu Raspberry Pi):8123/api/states/sensor.medidor_ai
    value_template: "OK"
  - platform: template
    sensors:
      bateria:
        friendly_name: "bateria"
        value_template: "{{ state_attr('sensor.medidor_ai', 'bateria') }}"
        unit_of_measurement: "V"
      corriente:
        friendly_name: "corriente"
        value_template: "{{ state_attr('sensor.medidor_ai', 'corriente') }}"
        unit_of_measurement: "A"
      voltaje:
        friendly_name: "voltaje"
        value_template: "{{ state_attr('sensor.medidor_ai', 'voltaje') }}"
        unit_of_measurement: "V"
      potencia:
        friendly_name: "potencia"
        value_template: "{{ state_attr('sensor.medidor_ai', 'potencia') }}"
        unit_of_measurement: "W"
      energia:
        friendly_name: "energia"
        value_template: "{{ state_attr('sensor.medidor_ai', 'energia') }}"
        unit_of_measurement: "KW/h"
      fp:
        friendly_name: "fp"
        value_template: "{{ state_attr('sensor.medidor_ai', 'fp') }}"
```

