// kompilace R3 uno


// Teplotní čidlo DS18B20

// připojení knihoven
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h> //watchdog - slouží k restartování MCU pro zamrznutí programu
// studium watchdoga https://create.arduino.cc/projecthub/rafitc/what-is-watchdog-timer-fffe20
//studium millis() https://www.programmingelectronics.com/arduino-sketch-with-millis-instead-of-delay/

// nastavení čísla vstupního pinu
#define pinCidlaDS 4
// vytvoření instance oneWireDS z knihovny OneWire
OneWire oneWireDS(pinCidlaDS);
// vytvoření instance senzoryDS z knihovny DallasTemperature
DallasTemperature senzoryDS(&oneWireDS);
float teplota = 0.0;
float korekce = 0.0;
int chybaCteniTeploty = 0;
int chybaCteniTeplotyMax = 5;
unsigned long dalsiCteniTeploty = 0;
unsigned long periodaCteniTeploty = 60000; // 60 sec v ms

unsigned long posledniMillis = 0;
unsigned long zacatekZmenyStavuOtevtit = 0;
unsigned long zacatekZmenyStavuZavrit = 0;
unsigned long delkaZmenyStavu = 110000; //110 sec v ms

//definice pinů relé
#define otevrit 5
#define zavrit 3

float teplotaZavreno = 20.0;
float teplotaOtevreno = 24.0;


bool otevreno = false; //stav - změna probehne až po dokončení procesu
bool otevirani = false; //proces
bool zavirani = false; //proces
bool ctiTeplotuPovoleno = true;

#define zapnuto LOW //
#define vypnuto HIGH //



void setup() {
  // komunikace přes sériovou linku rychlostí 9600 baud
  Serial.begin(9600);
  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();
  pinMode(otevrit, OUTPUT);
  pinMode(zavrit, OUTPUT);
  ctiTeplotu(); //volání fce pro čtení teploty

  wdt_enable(WDTO_4S); //nastaví watchdog, že musí být restartován každé 4 sec, pokud bude delay více jak 4 sec tak se restartuje MCU!
  //proto nepoužívat delay (max nějaký ms), ale všechno tahat přes millis()
}

void loop() {
  if (posledniMillis <= millis()){
    posledniMillis = millis (); //uchovává hodnotu millis posledního loopu    
  }
  else {
    Restart("Přetekla proměnná pro čas. Nutný restart. Hodnota: ", posledniMillis); //pravidelný restart po přetečení proměnné pro čas
  }
  if (dalsiCteniTeploty <= millis()) { //pokud je další čtení teploty větší, než je millis, tak ukonči funkci
    ctiTeplotu(); //volání fce pro čtení teploty
  }
  

  if ((teplota <= teplotaZavreno) && (otevreno == true)) { //pokud je teplota nižší než teplota pro zavřeno a zároveň je otevřeno, tak zavolej fci zavři
    zavri();
  }
  if ((teplota >= teplotaOtevreno) && (otevreno == false)) { //pokud je teplota vyšší než teplota pro otevřeno a zároveň není otevřeno, tak zavolej fci otevři
    otevri();
  }

  wdt_reset(); //restart watchdoga. Lze přidat i do časově náročných fcí kde by hrozilo resetování MCU. 
}

//******************* funkce *******************//

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void ctiTeplotu(){
  if (ctiTeplotuPovoleno == false) { //pokud je zakázáno čtení teploty, tak ukonči funkci (bez čtení teploty).
    return;
  }

  senzoryDS.requestTemperatures(); // načtení informací ze všech připojených čidel na daném pinu
  float teplotaBezKorekce = senzoryDS.getTempCByIndex(0); //ulozeni teploty do promene
  // výpis teploty na sériovou linku, při připojení více čidel
  // na jeden pin můžeme postupně načíst všechny teploty
  // pomocí změny čísla v závorce (0) - pořadí dle unikátní adresy čidel

  Serial.println("Teplota bez korekce je: " + String(teplotaBezKorekce) + "°C.");
  if (( teplotaBezKorekce > -127) && ( teplotaBezKorekce < 85)){ //kontrola teploty jestli není chybně přečtena
      chybaCteniTeploty = 0; //nastavení počítadla chyb na 0
      teplota = teplotaBezKorekce + korekce; //ke čtené teplotě to přičte korekci čidla
      Serial.println(F("\n--- Měření teploty je ok. ---\n"));
      Serial.println("Teplota s korekcí je: " + String(teplota) + "°C\n\n");

      if(otevreno){
        Serial.println("Je otevřeno.");
      }
      else if (!otevreno){
        Serial.println("Je zavřeno.");
      }
  }

  else if (chybaCteniTeploty < chybaCteniTeplotyMax){ //pokud je špatně přečtena teplota a není dosaženo maximum posobě jdoucích chybových měření, tak to přičte 1 k počítadlu
      Serial.println(F("Měření teploty neproběhlo korektně."));
      Serial.println("Hodnota chyby čtení teploty je: " + String(chybaCteniTeploty + 1) + " / " + String(chybaCteniTeplotyMax));
      chybaCteniTeploty++;
  }
  else{ //pokud je počítadlo na maximu povolených chyb, tak restartuje MCU
      Restart("Hodně chybových měření po sobe. Celkem: ", chybaCteniTeploty);
  }
  dalsiCteniTeploty = millis() + periodaCteniTeploty;
}

void Restart(String Message, int Value){

    Serial.println();
    Serial.print(F("Důvod restartování zařízení: "));   Serial.print(Message);  Serial.println(Value);
    Serial.println(F("--------------------------------------------------------------------------------------"));
    Serial.println(F("----------------------------------------RESTART DEVICE--------------------------------"));
    Serial.println(F("--------------------------------------------------------------------------------------"));
    Serial.println();   Serial.println();

    resetFunc(); //call reset
}

void otevri (){
  if (otevirani == false){ //začátek otevírání - poprvé vyvolaná fce otevri
    Serial.println("otevirani zahajeno");
    ctiTeplotuPovoleno = false; //vypnutí čtení teploty po dobu otevírání
    zacatekZmenyStavuOtevtit = millis(); //uložení začátku otevírání
  }
  if ((zacatekZmenyStavuOtevtit + delkaZmenyStavu) >= millis()){ //pokud je současný čas menší než čas začátku a délky trvání, tak to padne sem
    if(otevirani == false){ //pokud je otevírání false, tak nastaví na true - což začne pohyb. Zároveň nastaví otevírání na true aby to už do tohoto ifu nepadlo
      otevirani = true;
      digitalWrite(otevrit, zapnuto);
    }
  }
  else if (otevirani == true){ //pokud není splněna první podmínka (s millis), tak znamená, že už uběhl požadovaný čas a spadne to sem
    digitalWrite(otevrit, vypnuto); //vypne motor
    otevirani = false; // nastaví, že proces je dokončen
    otevreno = true; //řiká, že je otevřeno
    ctiTeplotuPovoleno = true; //povolí čtení teploty
    Serial.println("otevirani ukonceno");
  }  
}

void zavri (){
  if (zavirani == false){ //začátek zavírání - poprvé vyvolaná fce zavri
    Serial.println("zavirani zahajeno"); 
    ctiTeplotuPovoleno = false; //vypnutí čtení teploty po dobu zavítání
    zacatekZmenyStavuZavrit = millis(); //uložení začátku zavírání
  }
  if ((zacatekZmenyStavuZavrit + delkaZmenyStavu) >= millis()){ //pokud je současný čas menší než čas začátku a délky trvání, tak to padne sem
    if(zavirani == false){ //pokud je zavirani false, tak nastaví na true - což začne pohyb. Zároveň nastaví zavírání na true aby to už do tohoto ifu nepadlo
      zavirani = true;
      digitalWrite(zavrit, zapnuto);
    }
  }
  else if (zavirani == true){ //pokud není splněna první podmínka (s millis), tak znamená, že už uběhl požadovaný čas a spadne to sem
    digitalWrite(zavrit, vypnuto); //vypne motor
    zavirani = false; // nastaví, že proces je dokončen
    otevreno = false; //řiká, že není otevřeno, tudíž je zavřeno
    ctiTeplotuPovoleno = true; //povolí čtení teploty
    Serial.println("zavirani ukonceno");
  }  
}
