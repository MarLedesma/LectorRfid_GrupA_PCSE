/**  Bibliotecas ARDUINO a incluir */

#include <WiFi.h>               /** Conexion ESP32 a WiFi */
#include <SPI.h>                /** RFId - Conexion via SPI */
#include <MFRC522.h>            /** RFId - Libreria Lector tarjetas */
#include <PubSubClient.h>       /**  MQTT - Comunicacion con Mosquitto */

/**  Definiciones y Constantes */

const char* wifiSSID = "NombreRedWiFi";   
const char* wifiPswd = "PasswordWiFi"; 

// GPIOs del lector RFId y Leds de acceso
#define SS_PIN    5     /**  RFId - Pin de chip select SPI */
#define RST_PIN   0     /**  RFId - Pin de chip reset MFRC522 */
#define ACC_OK   13     /**  GPIO Led VERDE, Acceso OK */
#define DENEGADO 12     /**  GPIO Led ROJO, Acceso denegado */

/** MQTT - Constantes de acceso al broker Mosquitto */
const char* mqttBrkr = "xx.xx.xx.xx";   //  IP broker mosquitto
const int   mqttPort = pppp;            //  PUERTO del broker
const char* mqttUser = "UsuarioMQTT";   //  Usuario acc. broker
const char* mqttPswd = "ClaveMQTT";     //  Pasword acc. broker

/** MQTT - ZONA DE ACCESO, topicos de suscripcion y clientes */
const char* TOPICO_PUB = "HaciaElServer";  // Topico PUBLICACION

// PROX. 5 DEFINICIONES PONER LA ZONA !! (en este caso es xx).......
const char* ZONACC = "xx";            // Zona del lector "zz"
const char* TOPICO_SUB = "A_ZONAxx";  // Topico SUSCRIPCION
const char* mqttClient = "mqttClixx"; // Id. cliente MQTT
WiFiClient cliZxx;                    // Cli.WiFI (cli + "xx")
PubSubClient client(cliZxx);          // OBJ. Cli. MQTT Mosquitto
// .................................................................

MFRC522 rfid(SS_PIN, RST_PIN);   
MFRC522::MIFARE_Key key;  

/**  SETUP ********************************************************/
void setup() { 
     Serial.begin(9600);  // Inic. bus serial (monitor)
     Serial.println("\n\n\n**********************************");
     Serial.print("PUNTO CONTROL DE ACCESO - Zona: ");     
     Serial.println(ZONACC);
     Serial.println("Lee codigos RFId clásicos MIFARE");
     Serial.println("usando la clave: FF FF FF FF FF FF");
     // RFId: variable clave de lectura RFId: FF FF FF FF FF FF ----
     for (byte i = 0; i < 6; i++)
          key.keyByte[i] = 0xFF;
     
     SPI.begin();         // Inicializar bus SPI
     rfid.PCD_Init();     // RFId: Inicializar MFRC522

     conexionWiFi();      // Conectar ESP32 a WiFi
     conexionBroker();    // Conectar al broker MQTT Mosquitto 
     
     // Inicializa los GPIO de los LED y los apaga
     pinMode(ACC_OK, OUTPUT);
     pinMode(DENEGADO, OUTPUT);   
     digitalWrite(ACC_OK, LOW);   
     digitalWrite(DENEGADO, LOW); 
}

/**  LOOP **********************************************************/
/**  Lee tarjetas y si es tipo MIFARE envia el mensaje MQTT al broker */
void loop() {

  client.loop();            // Loop del cliente MQTT

  // RFId: Resetear el loop si no hay nueva tarjeta presente,
  //       (ahorra tiempo de proceso)
  if ( ! rfid.PICC_IsNewCardPresent())
     return;

  // RFId: Verifica si NO fue leido el NUID de la tarjeta
  if ( ! rfid.PICC_ReadCardSerial())
     return;
    
  if (verificaTarjeta())
     enviaMensajeMQTT();    // Envia el mensaje MQTT

  rfid.PICC_HaltA();        // Detener el PICC
  rfid.PCD_StopCrypto1();   // Detener la encriptacion en PCD
}

/** RUTINAS Y FUNCIONES */
void conexionWiFi(){  /** Conexion del ESP32 a WiFi */
     Serial.print("\n\nConectando a WiFi ...");
     WiFi.begin(wifiSSID, wifiPswd);
     while (WiFi.status() != WL_CONNECTED) {
           delay(500);
           Serial.print(".") ;
     }
     Serial.println("OK");
     Serial.print("CONECTADO A SSID: ");
     Serial.println(WiFi.SSID());
     Serial.print("*IP: ");
     Serial.println(WiFi.localIP());
     Serial.print("Msk: ");
     IPAddress subnet = WiFi.subnetMask();
     Serial.println(subnet);
     Serial.print("GW:  ");
     IPAddress gateway = WiFi.gatewayIP();
     Serial.println(gateway);
     Serial.print("MAC: ");
     byte mac[6];
     char display[32]; 
     WiFi.macAddress(mac);
     sprintf( display , "%02x %02x %02x %02x %02x %02x",
                        mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]);
     Serial.println(display);
     Serial.print("dBm: ");
     long rssi = WiFi.RSSI();
     Serial.println(rssi);
     Serial.print("Stat ");
     Serial.println(WiFi.status());
}     

void conexionBroker(){ /** Conexion al Broker (server) Mosquitto */
     Serial.print("\n\nConectando a Broker MQTT ... ");
     client.setServer(mqttBrkr, mqttPort);  // Conexion al broker
     while (!client.connected()) {
           if (client.connect(mqttClient, mqttUser, mqttPswd )) {
              Serial.println("CONECTADO A MQTT \n\n");
              client.setCallback(respuestaMQTT);   // Func.top.SUB.
              client.subscribe(TOPICO_SUB); // Suscribe a Top. SUB.
           } else {
                  Serial.print("ERROR STATUS ");
                  Serial.println(client.state());
                  delay(2000);
             }
     }
}


/** Tarjeta leida: Es nueva lectura y tipo MIFARE clasico ? */
/** retorna true si esta todo OK, sino false. */
bool verificaTarjeta(){
     Serial.println("***********************");
     Serial.println("NUEVA TARJETA DETECTADA");
     Serial.print(F("PICC tipo: "));
     MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
     Serial.println(rfid.PICC_GetTypeName(piccType));

 /**  RFId: Verifica si es el PICC de un tipo clasico MIFARE */
     if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
          piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
           piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println("NO ES tipo clasico MIFARE");
        return false;
      }

     return true;
}      

/**  MQTT - Displaya nueva tarjeta y envía el mensaje */
void enviaMensajeMQTT() {

     char NUID[4], display[32], mensaje[12]; 

     // Almacena el NUID de la tarjeta en un vector local
     for (byte i = 0; i < 4; i++)
         NUID[i] = rfid.uid.uidByte[i];
  
     // Muestra datos de la tarjeta leida
               
     Serial.println("----------------------------");
     sprintf(display, "ZONA: %s - NUID: %02x %02x %02x %02x",
                       ZONACC, NUID[0], NUID[1], NUID[2], NUID[3]);
     Serial.println(display);
     Serial.println("----------------------------");  

     // Formatea mensaje MQTT y envia al broker
     // mesaje: string ZONA + NUID
     sprintf(mensaje, "%02s%02x%02x%02x%02x\0", 
                       ZONACC, NUID[0], NUID[1], NUID[2], NUID[3]);
     client.publish(TOPICO_PUB, mensaje);
     Serial.print("Mensaje enviado: ");
     Serial.println(mensaje);
}    


void respuestaMQTT(char *TOPICO_SUB, byte *msgResp, unsigned int lmsg) {
/**  MQTT - Rutina declarada en el "client.setCallback" */ 
/**         Recibe respuesta del broker en el tópico correspondiente */
     Serial.println("----------------------------");
     Serial.print("Respuesta al tópico ");
     Serial.println(TOPICO_SUB);
     Serial.print("Mensaje =>");
     for (int i = 0; i < lmsg; i++)
         Serial.print((char) msgResp[i]);
     Serial.println("<=");    

     // Si el mensaje es 0: acceso DENEGADO, si es 1 acceso OK.
     if (msgResp[0] == '0'){
        for (byte i = 0; i < 3; i++) {      // Tres veces 
            digitalWrite(DENEGADO, HIGH);   // prende LED ROJO
            delay(300);                     // espera 0.3 seg.
            digitalWrite(DENEGADO, LOW);    // apaga led
            delay(300);
            }    
     } else {
            if (msgResp[0] == '1'){
               digitalWrite(ACC_OK, HIGH);   // prende LED VERDE
               delay(1500);                  // durante 1.5 seg.
               digitalWrite(ACC_OK, LOW);    // apaga led
               }   
            }  
}
