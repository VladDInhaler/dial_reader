#include <Arduino.h>

// --- TIMING UND CONFIGURATION ---
constexpr int TELEFON_PIN = A0;
constexpr int SCHWELLENWERT = 250;

// Wählscheiben-Timing
constexpr int ENTPRELL_ZEIT = 25;
constexpr int MIN_IMPULS_ZEIT = 20;
constexpr int MAX_IMPULS_ZEIT = 90;
constexpr int WAHL_PAUSE = 600;
constexpr int SENDE_TIMEOUT = 2500;

// --- PROTOCOL CHARACTERS ---
constexpr char PROTOCOL_START = 0x02;   // STX (Start of Text)
constexpr char PROTOCOL_END = 0x03;     // ETX (End of Text)

// --- VARIABLEN ---
int adcWert = 1023;
int impulsCount = 0;
bool kontaktWarZu = false;

unsigned long flankenWechselZeit = 0;
unsigned long letzteFlankeZu = 0;
unsigned long letzteZifferZeit = 0;

String currentNumber = "";
bool sendeBereit = false;

void sendNumber(const String& number) {
    Serial.write(PROTOCOL_START);
    Serial.print(number);
    Serial.write(PROTOCOL_END);
    Serial.flush();
}

void setup() {
    Serial.begin(9600);
    pinMode(TELEFON_PIN, INPUT);
}

void loop() {
    // 1. Filter: Gleitender Mittelwert
    adcWert = (adcWert * 3 + analogRead(TELEFON_PIN)) / 4;
    bool kontaktIstZu = (adcWert < SCHWELLENWERT);

    // 2. Flankenerkennung mit Zeitmessung
    if (kontaktIstZu != kontaktWarZu) {
        unsigned long jetzt = millis();
        unsigned long phasenDauer = jetzt - flankenWechselZeit;

        if (phasenDauer > ENTPRELL_ZEIT) {
            if (kontaktIstZu) {
                letzteFlankeZu = jetzt;
                sendeBereit = false;
            } else {
                unsigned long impulsDauer = jetzt - letzteFlankeZu;

                if (impulsDauer >= MIN_IMPULS_ZEIT && impulsDauer <= MAX_IMPULS_ZEIT) {
                    impulsCount++;
                }
            }
            flankenWechselZeit = jetzt;
            kontaktWarZu = kontaktIstZu;
        }
    }

    // 3. Ziffer fertig ausgewertet?
    if (impulsCount > 0 && (millis() - flankenWechselZeit > WAHL_PAUSE)) {
        int ziffer = (impulsCount >= 10) ? 0 : impulsCount;
        currentNumber += String(ziffer);
        letzteZifferZeit = millis();
        sendeBereit = true;
        impulsCount = 0;
    }

    // 4. Gesamte Nummer senden
    if (sendeBereit && (millis() - letzteZifferZeit > SENDE_TIMEOUT)) {
        sendNumber(currentNumber);
        currentNumber = "";
        sendeBereit = false;
    }
}
