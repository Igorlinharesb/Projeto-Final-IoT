//LDR
#define LDR A0
#define AUTH D1
#define LAMP D0

int luz;
int autent;

// *Incluindo bibliotecas do DHT*
#include <DHT.h>
#include <Blynk.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>

// *Adicionando conexão com o protocolo Wi-Fi*
// Autenticando ao ThingSpeak e Blynk
String apiKey = "R42VDGDQK5MEXWYP";               //  <-- seu Write API key do site ThingSpeak
char auth[] = "73d6f080daac4ed9a5e036ebed2029d5"; //  <--authtoken do blynk
char ssid[] =  "REDEIOT";             // <-- substitua com o ssid e senha da rede Wifi
char pass[] =  "iotnet18";
const char* server = "api.thingspeak.com";

#define DHTPIN 0  //<-- pin onde o dht11 está conectado d3 = 0

DHT dht(DHTPIN, DHT11);
BlynkTimer timer;

WiFiClient client;

extern "C" {
#include <espnow.h> //Biblioteca do ESP-Now
}

//Estruturas de dados que será enviada
struct DADOS {
  uint16_t valor = 0;
  uint32_t tempo = 0;
};

//LED na porta digital D1
#define LED D1

//Modo de operação do protocolo
#define MASTER 1 //Pareia e envia os dados
#define SLAVE 2 //É pareado e recebe os dados
#define MASTER_SLAVE 3 //Pareia, envia e recebe dados

//Definindo o modo de operação
#define MODO SLAVE

//Métodos úteis para o protocolo ESP-Now
void IniciaESPNow() { //Inicia ESP-Now
  if (esp_now_init() != 0) {
    Serial.println("Falha ao iniciar protocolo ESP-Now; Reiniciando...");
    ESP.restart();
    delay(1);
  }

  esp_now_set_self_role(MODO); //Setando modo de operação
}

void Recebeu(uint8_t *mac, uint8_t *data, uint8_t len) { //Callback chamado sempre que recebe um novo pacote
  char MACmestre[6];
  sprintf(MACmestre, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("Recepcao do ESP de MAC: ");
  Serial.println(MACmestre);

  DADOS dados;
  memcpy(&dados, data, sizeof(dados));
  WidgetLED led1(V8);
  if (dados.valor == 1) {
    Serial.print("Valor Recebido: ");
    Serial.print(dados.valor);
    Serial.print(", Tempo Recebido: ");
    Serial.println(dados.tempo);
    autent = 1;
    digitalWrite(LED, HIGH);//Escreve o valor recebido na porta
    led1.on();
    Blynk.notify("Bem vindo Sr. Rayon");

  } else {
    Serial.print("Valor Recebido: ");
    Serial.print(dados.valor);
    Serial.print(", Tempo Recebido: ");
    Serial.println(dados.tempo);
    digitalWrite(LED, LOW);
    led1.off();
    Blynk.notify("Até mais Sr. Rayon!");
    autent = 0;
  }
  
}
void readLDR() {
  luz = analogRead(LDR);
  Serial.print("Luminosidade: ");
  Serial.println(luz);

  if (luz <= 900) {
    digitalWrite(LAMP, HIGH);
  } else {
    digitalWrite(LAMP, LOW);
  }
  Blynk.virtualWrite(V7, luz);
}

void sendSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
}

void setup() {
  // LED LDR
  pinMode (LAMP, OUTPUT);
  pinMode (LED, OUTPUT);

  IniciaESPNow();

  //Setando Callback de recebimento do ESP-Now
  esp_now_register_recv_cb(Recebeu);

  //funçoes do DHT
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  // Executa a função sendSensor a cada 1000ms
  timer.setInterval(1000L, sendSensor);
  Serial.println("Connecting to ");
  Serial.println(ssid);


  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {
  //LDR
  if (autent == 1){
    readLDR();
  }
  // DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    return;
  }

  Blynk.run();
  timer.run();
  if (client.connect(server, 80)) { //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
    postStr += "&field1="; //<-- atenção, esse é o campo 1 que você escolheu no canal do ThingSpeak
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celcius, Humidity: ");
    Serial.print(h);
    Serial.println("%. Send to Thingspeak.");
  }
  client.stop();
  Serial.println("Waiting...");
  // thingspeak needs minimum 15 sec delay between updates, i've set it to 20 seconds
  delay(5000);
}
