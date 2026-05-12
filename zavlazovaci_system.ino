#define BLYNK_TEMPLATE_ID "TMPL4ejJo7M01"
#define BLYNK_TEMPLATE_NAME "Automatický zavlažovací systém"
#define BLYNK_AUTH_TOKEN "sRGPPeDNAyDM1NIBOho5rClSX4XYAbkl"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// --- NASTAVENÍ WIFI ---
char ssid[] = "";
char pass[] = "";

// --- NASTAVENÍ PINŮ A KALIBRACE ---
const int SENSOR_PIN = 34; // Senzor (žlutý kabel)
const int RELAY_PIN = 25;  // Relé (hnědý kabel)

const int HODNOTA_SUCHO = 2600; 
const int HODNOTA_MOKRO = 1250; 
const int HRANICE_ZALEVANI = 30; // 30 %

BlynkTimer timer;
bool pauzaZalevani = false; // Proměnná pro hlídání, zda právě probíhá zalévání nebo vsakování

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Pro jistotu vypnuto
  
  Serial.println("--- Připojuji se k Wi-Fi a Blynku ---");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Nastavíme časovač, aby četl data každé 2 vteřiny (místo původního delay v loopu)
  timer.setInterval(2000L, cteniSenzoru);
}

void loop() {
  Blynk.run(); // Udržuje spojení s cloudem
  timer.run(); // Stará se o časování bez blokování
}

// Funkce, která se spouští každé 2 vteřiny
void cteniSenzoru() {
  int surovaHodnota = analogRead(SENSOR_PIN);
  int vlhkostProcenta = map(surovaHodnota, HODNOTA_SUCHO, HODNOTA_MOKRO, 0, 100);
  vlhkostProcenta = constrain(vlhkostProcenta, 0, 100);

  Serial.print("Vlhkost půdy: ");
  Serial.print(vlhkostProcenta);
  Serial.println(" %");

  // ODESLÁNÍ DAT DO DASHBOARDU
  Blynk.virtualWrite(V0, vlhkostProcenta); // Posíláme vlhkost na "Virtuální pin 0"

  // LOGIKA ZAVLAŽOVÁNÍ
  // Pokud je vlhkost nízká a zrovna nezaléváme/nečekáme na vsáknutí
  if (vlhkostProcenta < HRANICE_ZALEVANI && !pauzaZalevani) {
    Serial.println("⚠️ Půda je příliš suchá! Zapínám čerpadlo...");
    digitalWrite(RELAY_PIN, HIGH);
    Blynk.virtualWrite(V1, 1); // Pošleme do dashboardu info (na Pin V1), že čerpadlo běží
    
    pauzaZalevani = true; // Zablokujeme další spouštění
    
    // Za 3000 ms (3 vteřiny) se automaticky zavolá funkce vypniCerpadlo
    timer.setTimeout(3000L, vypniCerpadlo); 
  }
}

// Funkce pro vypnutí čerpadla
void vypniCerpadlo() {
  Serial.println("Vypínám čerpadlo. Čekám 7 vteřin, než se voda vsákne...");
  digitalWrite(RELAY_PIN, LOW);
  Blynk.virtualWrite(V1, 0); // Info pro dashboard: čerpadlo stojí

  // Za 7000 ms se zavolá funkce povolZalevani
  timer.setTimeout(7000L, povolZalevani);
}

// Funkce, která ukončí cyklus vsakování
void povolZalevani() {
  Serial.println("Voda by se měla vsáknout. Povoluji další měření.");
  pauzaZalevani = false;
  Serial.println("-----------------------------------");
}

// Funkce, která se automaticky zavolá, když změníš stav Switche (V1) v Blynku
BLYNK_WRITE(V1) {
  int stavTlacitka = param.asInt(); // Přečte, jestli je Switch zapnutý (1) nebo vypnutý (0)

  if (stavTlacitka == 1) {
    Serial.println("👋 Manuální start čerpadla z webu!");
    digitalWrite(RELAY_PIN, HIGH); // Zapne relé
  } else {
    Serial.println("👋 Manuální vypnutí čerpadla z webu!");
    digitalWrite(RELAY_PIN, LOW);  // Vypne relé
  }
}