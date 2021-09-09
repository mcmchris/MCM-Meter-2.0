# MCM-Meter-2.0
Se trata de un medidor energético desarrollado entorno al microcontrolador ESP32 (Adafruit ESP32 Feather) capaz de medir energía en sistemas domésticos monofásicos de 120 o 240 voltios (fase partida) monitoreando dos potenciales con un solo medidor de corriente (CT no invasivo) y un sensor de voltaje (PT), la información capturada se transmite a un servidor montado en una Raspberry Pi con Home Assistant que te permitirá monitorear los datos en tiempo real, desde un ordenador o teléfono móvil desde cualquier lugar del mundo.

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

## Seguir el video tutorial para configurar el Home Assistant

* Recomiendo configurar una IP estática para tu Raspberry Pi en las configuraciones del Router.
* Datos del **configuration.yaml**:

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


