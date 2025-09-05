#include  <Arduino.h>
#include  <ESPAsyncWebServer.h>
#include  <SPIFFS.h>
#include <DHT.h>
#include  <WebSocketsServer.h>

#define DHTPIN 33     // Pin donde está conectado el sensor DHT
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);


float temp = dht.readTemperature(); // Lee temperatura real


const uint8_t ledPin = 2;
const uint8_t potPin = 32;
const uint16_t dataTxTimeInterval = 500; //ms - Intervalo de actualización de datos.

AsyncWebServer server(80);
WebSocketsServer websockets(81);

void webSocketEvent(uint8_t, WStype_t, uint8_t *, size_t);
void notFound(AsyncWebServerRequest*);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  pinMode(ledPin,OUTPUT);
  pinMode(potPin,INPUT);

  WiFi.softAP("ESP32 DASHBOARD", "");
  Serial.println("\nsoftAP");
  Serial.println(WiFi.softAPIP());

  if(!SPIFFS.begin(true))
  {
    Serial.println("Error al montar SPIFFS");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
      {request->send(SPIFFS, "/index.html", "text/html");});
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
    {request->send(SPIFFS, "/styles.css", "text/css");});
  server.on("/icon.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {request->send(SPIFFS, "/icon.png", "image/x-icon");});
  server.on("/Escudo.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {request->send(SPIFFS, "/Escudo.png", "image/x-icon");});
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {request->send(SPIFFS, "/logo.png", "image/x-icon");});

  server.onNotFound(notFound);
  server.begin();

  websockets.begin();
  websockets.onEvent(webSocketEvent);
}

void loop() {
  // put your main code here, to run repeatedly:
  websockets.loop();

  static uint32_t prevMillis = 0;
  if(millis() - prevMillis >= dataTxTimeInterval)
  {
    prevMillis = millis();
    float temp = dht.readTemperature();
    int potValue = analogRead(potPin);
    potValue = map(potValue, 0, 4095, 0, 100);

    String data = "{\"temperature\":\""+ String(temp) + 
                  "\",\"potentiometer\":\""+ String(potValue) + 
                  "\"}";
    
    websockets.broadcastTXT(data);
    //Serial.println(data);
  }


}

// Callbacks
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{
  switch(type)
  {
    case WS_DISCONNECTED: Serial.printf("[%u] Desconectado!\n", num); break;

    case WS_CONNECTED:
    {
      IPAddress ip= websockets.remoteIP(num);
      Serial.printf("[%u] Conectado en %d.%d.%d.%d url: %s\n", num, ip[0], ip[1, ip[2]]);
      websockets.sendTXT(num, "Conectado al servidor");
    }break;

    case WStype_TEXT:
    {
      Serial.printf("[%u] Mensaje Recibido: %s\n", num, payload);
      String msg = String((char*)(payload));

    }break;
  }
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Página no encontrada!");
}
