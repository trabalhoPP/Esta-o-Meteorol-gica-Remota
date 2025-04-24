#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// === DEFINIÇÕES ===
#define DHTPIN D4         // GPIO2 - pino do DHT11
#define DHTTYPE DHT11
#define LEDPIN D2         // GPIO4 - LED
#define LDRPIN A0         // Pino analógico para LDR

const char* ssid = "iPhone de Julia A";
const char* password = "chimichurri";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;

// === CONECTA AO WI-FI ===
void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// === CALLBACK DO MQTT ===
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == "estacao/led") {
    if (msg == "ON") {
      digitalWrite(LEDPIN, HIGH);
      Serial.println("LED ligado via MQTT");
    } else if (msg == "OFF") {
      digitalWrite(LEDPIN, LOW);
      Serial.println("LED desligado via MQTT");
    }
  }
}

// === CONECTA AO BROKER MQTT ===
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado!");
      client.subscribe("estacao/led");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LEDPIN, OUTPUT);
  dht.begin();
  Serial.begin(115200);    // Apenas para debug no Monitor Serial
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int ldr = analogRead(LDRPIN);

    if (!isnan(t) && !isnan(h)) {
      String dados = "{\"temperatura\":" + String(t) + ",\"umidade\":" + String(h) + ",\"luminosidade\":" + String(ldr) + "}";
      Serial.println("Enviando via MQTT: " + dados);
      client.publish("estacao/dados", dados.c_str());
    } else {
      Serial.println("Falha na leitura do DHT11");
    }
  }
}
