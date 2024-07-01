TRABAJO PRACTICO PROGRAMACION Y COMUNICACION S.E.
=================================================

El objetivo de este trabajo practico es realizar un prototipo basico
de un sistema de control de acceso por identificacion
de tarjetas RFId.

La implementacion se hizo bajo entorno LINUX MINT 21.3;
La programación del microcontrolador ESP32 se llevo a
cabo en Arduino IDE y los tres modulos de la API en lenguaje C.

El microcontrolador ESP32, lee las tarjetas mediante
un modulo RFId (MFRC522); luego envia un mensaje MQTT
en el topico "AL_SERVER" a un servidor MariaDB
(base de datos mySQL) por medio de la API dividida en tres modulos.

El primer modulo de la API está escuchando permanentemente
al topico "AL_SERVER". Este modulo queda ejecutandose en el
equipo que hace las veces de servidor como servicio (permanente).
Una vez que recibe un mensaje al topico lo decodifica de la siguiente manera:
El mensaje consta de 10 caracteres; toma por un lado
los dos de la izquierda y los almacena en una variable "zona",
y los ocho restantes que conforman la identificacion de la tarjeta,
se almacenan en la variable NUID

Luego pasa al segundo módulo las dos tareas de SQL propiamente dichas;
la primera tarea será consultar a la tabla de accesos si la tarjeta
esta autorizada. El resultado de esta operacion se guarda en
una variable "resultado"
La segunda tarea que realiza este modulo es insertar un registro
con los datos del intento de acceso a una tabla tipo "log"
donde se graban los datos de zona, NUID, resultado (1 o 0, autorizado
o no) y fecha y hora de la operación)

El primer módulo recibe el resultado de autorizacion (1 o 0)
y llama al tercer modulo, que publicará en el topico que corresponde
a la zona este resultado. Es decir, que si la zona es por ejemplo
la "03", la publicacion se cursará al topico "A_ZONA03" y el mensaje
será el resultado obtenido de la tabla de autorizaciones.

Cuando el microcontrolador recibe el resultado a su topico, verfica el
mismo. Si es 1 enciende un led verde por 1.5 seg. Si es 0 (acceso
denegado) enciende y apaga un led rojo tres veces (0.3 seg.).
Cada una de los encendidos del led estan acompañados
por el sonido de un buzzer.


PROGRAMA DEL MICROCONTROLADOR (ESP32):
--------------------------------------

Se encarga de leer las tarjetas por medio de un modulo
RFId. Envia un mensaje MQTT al servidor de base de datos
conteniendo los datos de "zona" (area) e identificacion
de la tarjeta (NUID). Cada conjunto lector pertenece a una
zona; el sistema se hace escalable ya que modificando solo
unas pocas definiciones en el programa se pueden implementar
muchas "zonas" con poco esfuerzo de programacion.

**API EN LENGUAJE C**
La API que vincula los mensajes entre MQTT y MSQL pueden descargarse 
del siguiente repositorio: 
Links: github.com/ronarg100/TP_API_MQTT_SQL



         




