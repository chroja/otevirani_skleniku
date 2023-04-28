#include <OneWire.h>           // knihovna OneWire pro DS18B20
#include <DallasTemperature.h> // knihovna pro DS18B20

// nastavte piny pro DS18B20 a relé pro DC motor
#define ONE_WIRE_BUS 2
#define MOTOR_OPEN_PIN 3
#define MOTOR_CLOSE_PIN 4

// inicializace DS18B20 a relé pro DC motor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// nastavte teploty pro otevření a zavření okna
const int OPEN_TEMP = 23;
const int CLOSE_TEMP = 22;

// časové intervaly pro otevírání a zavírání okna
const unsigned long OPEN_TIME = 110000;  // 110 sekund
const unsigned long CLOSE_TIME = 110000; // 110 sekund

// proměnné pro časování otevírání a zavírání okna
unsigned long motorStartTime = 0;
bool motorRunning = false;
bool motorOpening = false;

void setup() {
  Serial.begin(9600); // inicializace sériové linky
  sensors.begin();    // inicializace DS18B20
  pinMode(MOTOR_OPEN_PIN, OUTPUT);  // nastavení relé pro otevření okna jako výstup
  pinMode(MOTOR_CLOSE_PIN, OUTPUT); // nastavení relé pro zavření okna jako výstup
  digitalWrite(MOTOR_OPEN_PIN, LOW);  // vypnutí relé pro otevření okna
  digitalWrite(MOTOR_CLOSE_PIN, LOW); // vypnutí relé pro zavření okna
}

void loop() {
  static unsigned long lastTempTime = 0; // proměnná pro poslední měření teploty
  unsigned long currentTime = millis();  // aktuální čas v milisekundách
  
  // měření teploty jednou za minutu
  if (currentTime - lastTempTime >= 60000) {
    sensors.requestTemperatures(); // měření teploty
    float tempC = sensors.getTempCByIndex(0); // získání teploty v stupních Celsia
    
    Serial.print("Teplota: ");
    Serial.print(tempC);
    Serial.print(" °C");

    // rozhodnutí, zda se má okno otevřít nebo zavřít
    if (tempC >= OPEN_TEMP && !motorRunning) {
      Serial.println(" - Otevírám okno");
      digitalWrite(MOTOR_OPEN_PIN, HIGH); // zapnutí relé pro otevření okna
      motorStartTime = currentTime; // uložení času spuštění motoru
      motorRunning = true; // nastavení příznaku, že motor běží
      motorOpening = true; // nastavení příznaku, že se okno otevírá
    } else if (tempC < CLOSE_TEMP && !motorRunning) {
      Serial.println(" - Zavírám okno");
      digitalWrite(MOTOR_CLOSE_PIN, HIGH); // zapnutí relé pro zavření okna
      motorStartTime = currentTime; // uložení času spuštění motoru
      motorRunning = true; // nastavení příznaku, že motor běží
      motorOpening = false; // nastavení příznaku, že se okno zavírá
    }
    
    lastTempTime = currentTime; // uložení času posledního měření teploty
  }
  
  // ovládání motoru pro otevírání nebo zavírání okna
  if (motorRunning && currentTime - motorStartTime >= (motorOpening ? OPEN_TIME : CLOSE_TIME)) {
    Serial.println(" - Motor zastaven");
    digitalWrite(motorOpening ? MOTOR_OPEN_PIN : MOTOR_CLOSE_PIN, LOW); // vypnutí relé pro ovládání motoru
    motorRunning = false; // nastavení příznaku, že motor je zastaven
  }
  
  // výpis stavu programu na sériovou linku
  Serial.print(" - ");
  Serial.print(motorRunning ? "Motor běží" : "Motor zastaven");
  Serial.print(motorOpening ? ", otevírání okna" : ", zavírání okna");
  Serial.println();
  
  delay(500); // pauza mezi opakováním smyčky
}
