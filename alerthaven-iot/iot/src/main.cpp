#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ThingSpeak.h>

// ================= CONFIGURAÇÕES =================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "alerthaven/eventos";
const char* MQTT_CLIENT_ID = "ESP32-AlertHaven";

// Configurações ThingSpeak
const unsigned long THINGSPEAK_CHANNEL_ID = 2969337;
const char* THINGSPEAK_API_KEY = "T7ACXEHAAT3BNS7B";

// ================= SENSORES =====================
const int PIN_SENSOR_CHUVA = 34;     // GPIO34 (ADC1_CH6) - Pino ADC válido
const int PIN_ANEMOMETRO = 15;       // GPIO15 (Entrada digital)
const int PUBLISH_INTERVAL = 20000;  // Intervalo de publicação em ms

const int ID_IOT = 3232321;

Adafruit_MPU6050 mpu;

// ================= VARIÁVEIS ====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

enum Evento { 
  ALAGAMENTO = 1,    // Valores numéricos para ThingSpeak
  TEMPESTADE = 2, 
  TORNADO = 3, 
  ONDA_DE_CALOR = 4, 
  TERREMOTO = 5, 
  NENHUM = 0 
};

Evento ultimoEvento = NENHUM;
unsigned long lastRotationTime = 0;
int rotationCount = 0;

// ================= FUNÇÕES ======================
void setupWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5s...");
      delay(5000);
    }
  }
}

// Função de interrupção para o anemômetro
void IRAM_ATTR countRotation() {
  if (millis() - lastRotationTime > 10) { // Debounce
    rotationCount++;
    lastRotationTime = millis();
  }
}


//DETECCAO DE EVENTOS PRODUTIVA
/*
Evento detectarEvento() {
  // Leitura do sensor de chuva (0-4095 em ESP32)
  int chuva = analogRead(PIN_SENSOR_CHUVA);
  
  // Calcular velocidade do vento (RPM)
  float vento = 0;
  if (rotationCount > 0) {
    unsigned long elapsed = millis() - lastRotationTime;
    vento = (rotationCount / 2.0) * (60000.0 / elapsed); // RPM
    rotationCount = 0;
  }
  
  // Leitura do MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  Serial.println("Leituras - Chuva: " + String(chuva) + 
                 " | Vento: " + String(vento) + " RPM" +
                 " | Temp: " + String(temp.temperature) + "C" +
                 " | Accel: " + String(a.acceleration.x));

  // Lógica de detecção de eventos (ajuste os valores conforme necessário)
  if (chuva > 3000 && vento > 500) return TEMPESTADE;
  if (vento > 800) return TORNADO;
  if (chuva > 3500) return ALAGAMENTO;
  if (temp.temperature > 40) return ONDA_DE_CALOR;
  if (abs(a.acceleration.x) > 3.0 || abs(a.acceleration.y) > 3.0) return TERREMOTO;
  
  return NENHUM;
}
*/

//DETECCAO DE EVENTOS SIMULADA
Evento detectarEvento() {
  // Simulação de leituras de sensores
  static unsigned long lastSimulationTime = 0;
  static int simulationPhase = 0;
  
  // Avança a fase de simulação a cada 10 segundos
  if (millis() - lastSimulationTime > 10000) {
    simulationPhase = (simulationPhase + 1) % 6; // 6 fases (0-5)
    lastSimulationTime = millis();
  }

  // Variáveis simuladas
  int chuva = 0;
  float vento = 0;
  float temperatura = 25.0;
  float aceleracaoX = 0.0;
  float aceleracaoY = 0.0;

  // Configurar valores simulados baseados na fase atual
  switch(simulationPhase) {
    case 0: // Condições normais
      chuva = random(500, 1000);
      vento = random(10, 50);
      temperatura = random(20, 30);
      break;
      
    case 1: // Alagamento
      chuva = random(3500, 4095);
      vento = random(30, 80);
      temperatura = random(22, 28);
      break;
      
    case 2: // Tempestade
      chuva = random(3000, 4000);
      vento = random(500, 700);
      temperatura = random(18, 25);
      break;
      
    case 3: // Tornado
      chuva = random(1000, 2000);
      vento = random(800, 1000);
      temperatura = random(20, 28);
      aceleracaoX = random(30, 50)/10.0;
      break;
      
    case 4: // Onda de calor
      chuva = random(0, 500);
      vento = random(0, 30);
      temperatura = random(40, 45);
      break;
      
    case 5: // Terremoto
      chuva = random(500, 1500);
      vento = random(10, 100);
      temperatura = random(20, 30);
      aceleracaoX = random(40, 80)/10.0;
      aceleracaoY = random(40, 80)/10.0;
      break;
  }

  // Adicionar pequenas variações aleatórias
  chuva += random(-50, 50);
  vento += random(-20, 20);
  temperatura += random(-5, 5)/10.0;
  aceleracaoX += random(-10, 10)/10.0;
  aceleracaoY += random(-10, 10)/10.0;

  // Garantir limites físicos
  chuva = constrain(chuva, 0, 4095);
  vento = constrain(vento, 0, 1000);
  temperatura = constrain(temperatura, -10, 50);

  // Mostrar valores simulados no Serial Monitor
  Serial.println("\n=== Leitura dos Sensores Simulados ===");
  Serial.println("Fase de Simulação: " + String(simulationPhase));
  Serial.println("Chuva: " + String(chuva) + " (0-4095)");
  Serial.println("Vento: " + String(vento) + " RPM");
  Serial.println("Temperatura: " + String(temperatura) + " °C");
  Serial.println("Aceleração X: " + String(aceleracaoX) + " m/s²");
  Serial.println("Aceleração Y: " + String(aceleracaoY) + " m/s²");

  // Lógica de detecção de eventos
  if (chuva > 3000 && vento > 500) {
    Serial.println(">>> Detecção: TEMPESTADE <<<");
    return TEMPESTADE;
  }
  if (vento > 800) {
    Serial.println(">>> Detecção: TORNADO <<<");
    return TORNADO;
  }
  if (chuva > 3500) {
    Serial.println(">>> Detecção: ALAGAMENTO <<<");
    return ALAGAMENTO;
  }
  if (temperatura > 40) {
    Serial.println(">>> Detecção: ONDA DE CALOR <<<");
    return ONDA_DE_CALOR;
  }
  if (abs(aceleracaoX) > 3.0 || abs(aceleracaoY) > 3.0) {
    Serial.println(">>> Detecção: TERREMOTO <<<");
    return TERREMOTO;
  }

  Serial.println(">>> Nenhum evento detectado <<<");
  return NENHUM;
}

void enviarDados(Evento evento) {
  // Enviar para MQTT
  JsonDocument doc;
  doc["device"] = ID_IOT;
  doc["evento"] = static_cast<int>(evento);
  doc["timestamp"] = millis();
  
  char payload[256];
  serializeJson(doc, payload);

  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.println("MQTT enviado: " + String(payload));
  } else {
    Serial.println("Falha no envio MQTT!");
  }

  // Enviar para ThingSpeak
  ThingSpeak.setField(1, ID_IOT);
  ThingSpeak.setField(2, static_cast<float>(evento));
  ThingSpeak.setField(3, analogRead(PIN_SENSOR_CHUVA));
  
  int status = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_KEY);
  if (status == 200) {
    Serial.println("Dados enviados para ThingSpeak!");
  } else {
    Serial.println("Erro ThingSpeak: " + String(status));
  }
}

void maintainConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconectando WiFi...");
    setupWiFi();
  }
  
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar WiFi
  setupWiFi();
  
  // Inicializar MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  
  // Inicializar ThingSpeak
  ThingSpeak.begin(espClient);
  
  // Inicializar MPU6050
  if (!mpu.begin()) {
    Serial.println("Falha ao iniciar MPU6050!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println("MPU6050 inicializado!");
  
  // Configurar pinos
  pinMode(PIN_SENSOR_CHUVA, INPUT);
  pinMode(PIN_ANEMOMETRO, INPUT_PULLUP);
  
  // Configurar interrupção para o anemômetro
  attachInterrupt(digitalPinToInterrupt(PIN_ANEMOMETRO), countRotation, FALLING);
  
  Serial.println("Sistema iniciado!");
}

void loop() {
  maintainConnections();
  mqttClient.loop();
  delay(10000);

  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck >= 2000) {
    Evento eventoAtual = detectarEvento();
    
    if (eventoAtual != NENHUM && eventoAtual != ultimoEvento) {
      enviarDados(eventoAtual);
      ultimoEvento = eventoAtual;
      
      Serial.print("Evento detectado: ");
      switch(eventoAtual) {
        case ALAGAMENTO: Serial.println("ALAGAMENTO"); break;
        case TEMPESTADE: Serial.println("TEMPESTADE"); break;
        case TORNADO: Serial.println("TORNADO"); break;
        case ONDA_DE_CALOR: Serial.println("ONDA_DE_CALOR"); break;
        case TERREMOTO: Serial.println("TERREMOTO"); break;
      }
    } else if (eventoAtual == NENHUM) {
      ultimoEvento = NENHUM; // Resetar quando não houver evento
    }
    
    lastCheck = millis();
  }
}