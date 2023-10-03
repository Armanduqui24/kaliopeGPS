// Visual Micro is in vMicro>General>Tutorial Mode
//
/*
    Name:       proyecto_gps_new.ino
    Created:	11/09/2023 05:56:07 p. m.
    Author:     DESKTOP-V5Q8Q9E\armanduqui2408
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 rtc;

SoftwareSerial sim808(2, 3);  //Rx, Tx  //Especificamos los puertos del modulo sim 808

//Definimos variables globales.
String imei, url, fecha, hora, lattitude, longitude, altitud, velocidad, grados, presicion, satelites, location = "3D", estado = "1", combustible = "1";
String day, month, year, hour, minute, second;
String data[20];
int i = 0;
char c;
unsigned long previousMillis = 0;
long interval = 60000;

// The setup() function runs once each time the micro-controller starts
void setup() {

  //Abrimos la comunicacion serial con la pc y esperamos la apertura del puerto
  Serial.begin(115200);
  while (!Serial) {
    //Espera por coneccion
    ;
  }
  Serial.println("Serial PC conectado...");
  delay(1000);

  //Abre la comunicacion serial para el sim808 y espera por la apertura
  sim808.begin(9600);
  while (!sim808) {
    //Espera por coneccion
    ;
  }
  Serial.println("Serial SIM 808 conectado...");
  delay(1000);

  //Iniciamos el Reloj en tiempo Real.
  if (!rtc.begin()) {
    Serial.println("Modulo RTC no encontrado");
    while (1)
      ;
  } else {
    Serial.println("Modulo RTC iniciado...");
  }
  //Solo se usa la primera vez para poner en horario el RTC, despues se comenta la linea
  //rtc.adjust(DateTime(__DATE__, __TIME__));

  //Enviamos at si el modulo contesta ok, el modulo esta listo para recibir los demas comandos
  sim808.println("AT");
  delay(10000);

  //Habilita todas las funciones en modulo por si estan desactivadas
  sim808.println("AT+CFUN=1");

  //Habilitamos la menera en que el modulo va a recibir los comandos por si no esta configurado le decimos que los comandos van en texto plano
  sim808.println("AT+CMGF=1");

  //Cuando mandamos comandos at el modulo nos responde con el mismo comando que le mandamos mas la respuesta, este comando desactiva el ECO.
  sim808.println("ATE0");

  //Comando para indicarle al modulo que se conecte a la red.
  sim808.println("AT+CREG?");

  //Prendemos el gps
  sim808.println("AT+CGNSPWR=1");

  //Activamos el GPRS es decir le decimos al modulo que inicie la coneccion de datos con la red telcel
  sim808.println("AT+CGATT=1");

  //Introducimos la APP de telcel para la coneccion a internet
  sim808.println("AT+CSTT=\"internet.itelcel.com\"");

  //Iniciamos la coneccion inalambrica de datos con la red
  sim808.println("AT+CIICR");

  //Obtenemos la IP que nos asigno la red, no nos sirve para nada solamente para saber que ya estamos conectados a la red.
  //Serial.println("Tu IP es: " + leerRespuestaModulo("AT+CIFSR"));
}

// Add the main program code into the continuous loop() function
void loop() {

  //Sirve para las lecturas que pongamos en el monitor serial las mande directo al modulo sim y viceversa
  while (sim808.available()) {
    Serial.println(sim808.readString());
  }
  while (Serial.available()) {
    sim808.println(Serial.readString());
  }


  //Ponemos la funcion millis de esta manera no frenamos el procesador, siempre que se cumpla el tiempo de interval entra en el if y ejecuta el codigo
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;

    //Iterador para el bucle for
    int i = 0;

    //Obtenemos el imei del modulo por medio de la funcion leer modulo imei.
    imei = leerImeiModulo("AT+GSN");

    //Instanciamos objeto el RTC y le pedimos la hora y la fecha con el rtc.now
    DateTime datosRTC = rtc.now();

    //En la instancia anterior nos regresa una array, con estos metodos que ya tiene la libreria, le pedimos que nos pase el dia, el mes, el año y luego los ordenamos.
    day = datosRTC.day();
    month = datosRTC.month();
    year = datosRTC.year();
    fecha = day + "-" + month + "-" + year;

    //Lo mismo de arriba, pero con la hora, minutos, y segundos.
    hour = datosRTC.hour();
    minute = datosRTC.minute();
    second = datosRTC.second();
    hora = hour + ":" + minute + ":" + second;


    //Aqui estubo un poco complicado, por que el sim 808 nos regresa la informacion como una maquina de escribir es decir letra por letra, entonces tenemos que ir cachando
    //cada letra, almacenarla en una variable, y luego, concatenarla, con la que ya tenemos, de tal manera que nos regresa una secuencia completo dependiendo del comando 
    //que le mandemos al modulo, para este caso de obtener los datos gps, nos regresa deparados por , entonces aprovechando cada que encuentre una , creamos un indice de
    //del array de esta manera al final del bucle, ya tenemos todos los datos separados, por indices de array, para luego meterlos en una variable, lo de la variable
    //se puede quitar y ingresar el dato directo desde el array, pero es mas claro si lo dejamos con la variable.

    //Inicialisamos los indices del array en este caso solo ocupamos hasta el indice 15, pero se puede ajustar.
    for (i = 0; i < 15; i++)
      data[i] = ""; //Iniciamos indice a vacio

    //Podemos usar otro iterador, pero como ya no se ocupa el del bucle for iniciamos este mismo a 0 para no gastar mas memoria.  
    i = 0;
    //Serial.println("----Command given----");
    sim808.flush();
    long int time = millis(); //Obtenemos datos del millis.
    sim808.println("AT+CGNSINF"); //Le mandamos el comando de datos gps al modulo.

    while ((time + 1000) > millis()) { //Le damos 1 segundo de retardo para que se quede escunchando la respuesta del modulo.
      while (sim808.available()) {  //Si esta disponible el sim.
        c = sim808.read(); //c es igual a el caracter que leemos del sim 808
        if (c != ',') { //Entramos a los indices del array si es diferente de , concatenamos en el mismo indice, si no subimos el indice en el else, y volvemos a repetir
          data[i] += c;
          delay(2);
        } else
          i++;
      }
      if (i == 15) //Si i es mayor al tamaño de los datos que necesitamos salimos del ciclo por que el array ya esta lleno y no necesitamos mas datos.
        break;
    }

    //Guardamos datos del array en las variables que necesitamos.
    lattitude = data[3];
    longitude = data[4];
    altitud = data[5];
    velocidad = data[6];
    grados = data[7];
    presicion = data[12];
    satelites = data[14];

    //Mostramos los datos el monitor, pero no es necesario, se puede comentar, solo para el develop.
    Serial.println(fecha);
    Serial.println(hora);
    Serial.println(lattitude);
    Serial.println(longitude);
    Serial.println(altitud);
    Serial.println(velocidad);
    Serial.println(grados);
    Serial.println(presicion);
    Serial.println(satelites);
    Serial.println(location);
    Serial.println(imei);

    //Serial.println("----Response Print-1--");
    //delay(4000);


      //Como usamos el protocolo http tenemos que hacer nuestra url, con los datos que vamos a mandar al servidor, aqui la hacemos.
      //Esto es lo que necesitamos mandar url del archivo que va a recibir los datos en el servidor, luego ?d1= datos 1, los datos gps, estado, y combustible, separados por una ,.
      //Asi deve quedar: url?d1=fecha,hora,latitud,longitud,altitud,velocidad,grados,presicion,satelites,location,estado,combustible&d2=imei
      //http://162.241.93.228/gps_recibe_datos.php?d1=02-10-2023,4:52:00,19.855050,-100.995028,2453,0.5,127,1.10,10,3,1,1&d2=869170032340854
      //Lo tenemos que trabajar de esta manera por que si concatenamos en una sola liena, saturamos la memoria del arduino y no nos manda la informacion.
      url = "http://162.241.93.228/gps_recibe_datos.php?d1=";
      url += fecha; //+ "," + hora + "," + lattitude + "," + longitude; // + "," + altitud + "," + velocidad + "," + grados + "," + presicion + "," + satelites + "," + location + "," + estado + "," + combustible;
      url += ",";
      url += hora;
      url += ",";
      url += lattitude;
      url += ",";
      url += longitude;
      url += ",";
      url += altitud;
      url += ",";
      url += velocidad;
      url += ",";
      url += grados;
      url += ",";
      url += presicion;
      url += ",";
      url += satelites;
      url += ",";
      url += location;
      url += ",";
      url += estado;
      url += ",";
      url += combustible;
      url += "&d2=";
      url += imei;

      //Mostramos la url, solo es para ver si arduino genero la url bien.
      Serial.println(url);

      //Prendemos el GPS
      sendATcommand("AT+CFUN=1", "OK", 2000);

      //AT+CGATT = 1 Connect modem is attached to GPRS to a network. AT+CGATT = 0, modem is not attached to GPRS to a network
      sendATcommand("AT+CGATT=1", "OK", 2000);

      //Connection type: GPRS - bearer profile 1
      sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);

      //sets the APN settings for your network provider.
      sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet.itelcel.com\"", "OK", 2000);

      //enable the GPRS - enable bearer 1
      sendATcommand("AT+SAPBR=1,1", "OK", 2000);

      //Init HTTP service
      sendATcommand("AT+HTTPINIT", "OK", 2000);
      sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 1000);

      //Set the HTTP URL sim800.print("AT+HTTPPARA="URL","http://ahmadssd.000webhostapp.com/gpsdata.php?lat=222&lng=222"\r");

      sim808.print("AT+HTTPPARA=\"URL\",\"");
      sim808.print(url);
      sendATcommand("\"", "OK", 1000);

      //Set up the HTTP action
      sendATcommand("AT+HTTPACTION=0", "0,200", 1000);

      //Terminate the HTTP service
      sendATcommand("AT+HTTPTERM", "OK", 1000);

      //shuts down the GPRS connection. This returns "SHUT OK".
      sendATcommand("AT+CIPSHUT", "SHUT OK", 1000);
//    }
  }
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {

  //Lo que hacemos con esta funcion es enviar un comando al modulo sim 808, nos responde esperamos la respuesta en el tiempo
  //proporcionado, y corroboramos la respuesta.

  uint8_t x = 0, answer = 0;
  char response[100];
  unsigned long previous;

  //Initialice the string
  memset(response, '\0', 100);
  delay(100);

  //Clean the input buffer
  while (sim808.available() > 0) sim808.read();

  if (ATcommand[0] != '\0') {
    //Send the AT command
    sim808.println(ATcommand);
  }

  x = 0;
  previous = millis();

  //this loop waits for the answer with time out
  do {
    //if there are data in the UART input buffer, reads it and checks for the asnwer
    if (sim808.available() != 0) {
      response[x] = sim808.read();
      //Serial.print(response[x]);
      x++;
      // check if the desired answer (OK) is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));

  Serial.println(response);
  return answer;
}

String leerImeiModulo(String AtComand) {
  //Se puede mejor, pero por ahora funciona, el modulo sim nos regresa la informacion del imei letra por letra, entonces lo tenemos que recibir
  //y guardar en una variable C, lo dificil fue limpiar la respuesta por que el gps nos responde con un salto de linea y OK por eso lo limpiamos
  //con los if, al final conseguimos el imei limpio y lo regresamos.

  String respuesa = "";

  sim808.flush();
  long int time = millis();
  sim808.println(AtComand);

  while ((time + 1000) > millis()) {
    while (sim808.available()) {
      c = sim808.read();

      if (c != '\n') {
        if (c != 'O') {
          if (c != 'K') {
            respuesa = respuesa + c;
            delay(5);
          }
        }
      }
    }
  }
  return respuesa;
}

String leerRespuestaModulo(String AtComand) {
  //Por ahora, no usamos esta funcion, pero basicamente, nos regresa la respuesta completo del modulo.
  String respuesa = "";

  sim808.flush();
  long int time = millis();
  sim808.println(AtComand);

  while ((time + 1000) > millis()) {
    while (sim808.available()) {
      c = sim808.read();
      respuesa = respuesa + c;
      delay(5);
    }
  }
  return respuesa;
}
