/*
 * ============================================================
 *  Guante de gestos - ESP32 + 5 sensores flex + KNN
 *  (6 Clases incluidas: Gesto_C, Punio_Cerrado, Mano_Abierta,
 *   Gesto_L, Gesto_OK, Ajustar_Lentes)
 * ============================================================
 */

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ------------------------------------------------------------
//  Configuración de hardware
// ------------------------------------------------------------
const uint8_t NUM_FLEX = 5;
const uint8_t FLEX_PINS[NUM_FLEX] = {36, 39, 34, 35, 32};
const char*   NOMBRE_DEDO[NUM_FLEX] = {"pulgar", "indice", "medio", "anular", "menique"};

const uint8_t LECTURAS_PROMEDIO = 16;   // lecturas ADC promediadas por sensor
const float   ALFA_FILTRO       = 0.35; // filtro exponencial (0-1, menor = más suave)

// ------------------------------------------------------------
//  Clases / gestos (6 clases en total)
// ------------------------------------------------------------
const uint8_t NUM_CLASES = 6;
const char* NOMBRE_GESTO[NUM_CLASES] = {
  "Gesto_C", "Punio_Cerrado", "Mano_Abierta", "Gesto_L", "Gesto_OK", "Ajustar_Lentes"
};

// ------------------------------------------------------------
//  Escala de normalización
// ------------------------------------------------------------
const float NORM_ABIERTO = 0.08;
const float NORM_CERRADO = 0.90;

// ------------------------------------------------------------
//  Dataset base (300 muestras = 6 gestos x 50 muestras), valores x100
//  Orden: pulgar, indice, medio, anular, menique
// ------------------------------------------------------------
const uint16_t BASE_N = 300;
const uint8_t BASE_X[BASE_N][NUM_FLEX] = {
  // Clase 0: Gesto_C
  { 49, 56, 88, 90, 84},
  { 51, 53, 87, 87, 85},
  { 48, 54, 85, 90, 88},
  { 52, 56, 86, 90, 86},
  { 49, 55, 86, 86, 88},
  { 50, 56, 85, 89, 86},
  { 49, 55, 88, 87, 84},
  { 48, 54, 85, 89, 88},
  { 48, 53, 89, 90, 87},
  { 52, 57, 86, 90, 85},
  { 51, 56, 86, 89, 86},
  { 49, 54, 85, 89, 84},
  { 50, 55, 85, 88, 85},
  { 48, 54, 86, 88, 84},
  { 48, 54, 87, 90, 86},
  { 48, 55, 89, 89, 85},
  { 50, 54, 86, 87, 85},
  { 48, 56, 87, 86, 87},
  { 49, 54, 87, 86, 88},
  { 49, 54, 85, 86, 85},
  { 48, 55, 85, 90, 85},
  { 48, 54, 89, 86, 86},
  { 49, 56, 88, 90, 86},
  { 51, 57, 89, 86, 84},
  { 49, 57, 86, 86, 84},
  { 51, 57, 89, 90, 86},
  { 49, 56, 85, 87, 88},
  { 50, 53, 85, 87, 85},
  { 52, 55, 89, 89, 86},
  { 50, 53, 86, 88, 84},
  { 48, 53, 87, 87, 88},
  { 49, 56, 88, 88, 88},
  { 52, 53, 85, 88, 88},
  { 49, 55, 85, 88, 87},
  { 51, 55, 89, 89, 84},
  { 52, 56, 88, 89, 86},
  { 50, 53, 88, 89, 86},
  { 52, 55, 87, 88, 88},
  { 51, 57, 88, 87, 84},
  { 51, 57, 87, 88, 86},
  { 48, 54, 86, 90, 85},
  { 52, 53, 85, 86, 84},
  { 48, 56, 85, 87, 86},
  { 51, 56, 89, 86, 84},
  { 51, 53, 88, 88, 86},
  { 50, 57, 86, 89, 84},
  { 52, 54, 89, 87, 85},
  { 51, 55, 87, 89, 85},
  { 52, 53, 85, 89, 88},
  { 48, 55, 86, 86, 88},
  // Clase 1: Punio_Cerrado
  { 90, 89, 91, 90, 89},
  { 90, 91, 90, 92, 89},
  { 88, 91, 89, 90, 88},
  { 90, 91, 90, 91, 89},
  { 89, 89, 89, 90, 88},
  { 90, 91, 92, 88, 89},
  { 89, 93, 91, 90, 88},
  { 90, 92, 90, 92, 86},
  { 89, 91, 92, 90, 88},
  { 88, 89, 90, 92, 90},
  { 88, 92, 90, 92, 86},
  { 89, 90, 91, 92, 89},
  { 89, 89, 91, 88, 88},
  { 87, 92, 91, 92, 89},
  { 90, 89, 88, 89, 88},
  { 87, 92, 90, 88, 89},
  { 90, 89, 90, 89, 89},
  { 89, 90, 89, 90, 90},
  { 89, 91, 89, 90, 89},
  { 89, 89, 92, 88, 87},
  { 87, 91, 92, 90, 89},
  { 88, 92, 89, 92, 89},
  { 91, 89, 90, 88, 88},
  { 89, 89, 88, 90, 87},
  { 89, 92, 90, 88, 89},
  { 90, 89, 92, 88, 86},
  { 90, 91, 92, 88, 90},
  { 90, 92, 89, 92, 88},
  { 87, 90, 89, 92, 90},
  { 90, 89, 92, 92, 86},
  { 91, 91, 89, 90, 86},
  { 87, 93, 88, 89, 87},
  { 88, 89, 90, 88, 89},
  { 88, 90, 90, 88, 87},
  { 87, 93, 89, 92, 89},
  { 89, 91, 92, 88, 87},
  { 91, 89, 91, 88, 90},
  { 88, 91, 92, 90, 88},
  { 91, 93, 89, 89, 87},
  { 88, 89, 88, 89, 89},
  { 89, 89, 88, 90, 88},
  { 88, 89, 89, 88, 90},
  { 88, 92, 92, 92, 87},
  { 89, 92, 89, 91, 88},
  { 88, 93, 90, 91, 88},
  { 88, 92, 92, 91, 90},
  { 88, 89, 92, 90, 86},
  { 91, 92, 89, 90, 90},
  { 89, 91, 92, 92, 89},
  { 87, 91, 90, 89, 87},
  // Clase 2: Mano_Abierta
  {  7,  8, 11,  6,  7},
  {  8,  7, 10,  8,  9},
  { 11, 10,  9,  9, 11},
  { 10,  7,  9,  6, 11},
  {  8,  7,  9,  7,  7},
  {  8, 10, 10, 10,  9},
  { 10,  9, 11,  6, 10},
  { 11,  6,  9,  7,  9},
  {  9,  8,  8,  6,  9},
  { 10,  6,  8, 10,  8},
  {  9, 10,  9,  7,  8},
  {  8,  7, 11, 10,  9},
  {  7,  9, 11,  9, 10},
  {  8, 10, 10,  8, 10},
  {  9,  9,  8,  8, 10},
  {  9,  9, 11,  8, 10},
  {  8,  6, 11,  9, 10},
  {  9,  8, 10, 10,  8},
  {  9,  8, 11, 10,  7},
  {  9, 10,  8,  6, 11},
  {  8,  6,  9,  7,  9},
  {  8,  6, 10,  6,  7},
  {  8,  6,  9,  9,  7},
  {  8,  8,  7,  6,  7},
  {  7, 10,  7, 10,  9},
  {  7,  7,  7, 10,  7},
  { 11,  9,  8, 10,  8},
  {  8, 10, 11,  9,  9},
  { 11,  6,  7,  7,  8},
  { 10,  8,  7, 10, 11},
  { 10, 10, 10,  7,  7},
  {  9,  6,  8,  8,  9},
  { 10,  7,  7,  9,  8},
  { 10, 10, 10,  7, 11},
  {  7,  9,  9,  7, 10},
  { 11,  6,  8,  9,  8},
  {  7,  8,  9,  8,  9},
  {  8, 10, 11,  6,  7},
  {  9,  6,  9,  6, 10},
  {  9, 10,  9, 10, 10},
  { 10,  6, 11,  7,  9},
  {  7,  7, 10, 10, 10},
  {  8,  9, 10, 10, 11},
  {  9,  7,  8,  8, 10},
  { 10, 10, 10, 10,  8},
  {  7,  9,  8,  6, 10},
  {  8,  7,  9,  9,  8},
  {  8,  9, 10, 10,  8},
  {  8,  8,  9,  8,  7},
  { 10, 10, 10,  7,  9},
  // Clase 3: Gesto_L
  {  8,  9, 88, 85, 86},
  {  8, 11, 86, 88, 84},
  {  6, 11, 87, 87, 86},
  {  7,  9, 90, 89, 85},
  { 10,  8, 86, 86, 88},
  {  9,  7, 88, 85, 85},
  {  7,  7, 86, 88, 86},
  {  7, 10, 89, 87, 86},
  { 10, 10, 87, 88, 88},
  {  6,  7, 88, 87, 87},
  {  9,  9, 87, 87, 86},
  {  9,  9, 89, 86, 86},
  {  8, 10, 86, 88, 84},
  {  9,  9, 87, 86, 88},
  {  8, 11, 88, 89, 87},
  {  7,  9, 88, 86, 84},
  {  9,  7, 86, 89, 87},
  {  7,  8, 87, 88, 85},
  {  6,  7, 90, 87, 86},
  {  7, 10, 86, 87, 87},
  {  8,  8, 89, 86, 87},
  {  9, 11, 88, 85, 88},
  {  8,  7, 90, 85, 84},
  {  6,  8, 87, 88, 87},
  {  6,  9, 86, 88, 86},
  {  9, 10, 86, 88, 85},
  {  7,  9, 86, 87, 87},
  {  9, 10, 89, 89, 88},
  {  6,  9, 88, 86, 88},
  {  8, 11, 87, 87, 84},
  { 10,  8, 88, 87, 88},
  {  6,  9, 88, 85, 88},
  {  6, 10, 90, 86, 87},
  {  9, 11, 89, 89, 85},
  {  9,  8, 88, 89, 87},
  {  7,  9, 87, 87, 88},
  {  7,  8, 89, 85, 84},
  {  8,  7, 90, 89, 84},
  {  6, 11, 87, 87, 86},
  { 10, 10, 86, 87, 85},
  {  8,  8, 86, 88, 84},
  { 10,  7, 87, 87, 87},
  {  9,  8, 86, 89, 88},
  {  7,  9, 86, 89, 86},
  {  8,  9, 89, 86, 85},
  {  6,  7, 88, 86, 87},
  {  7, 10, 90, 89, 87},
  {  7,  9, 89, 87, 88},
  {  6, 11, 89, 89, 86},
  { 10,  7, 90, 85, 86},
  // Clase 4: Gesto_OK
  { 71, 71,  9,  9,  7},
  { 72, 72,  8,  8, 11},
  { 72, 72, 12, 10, 11},
  { 70, 71,  9,  9, 11},
  { 72, 69, 12,  8,  9},
  { 73, 69, 12, 10,  9},
  { 72, 68,  8,  9,  9},
  { 73, 69, 11,  7, 11},
  { 72, 69, 11, 11,  9},
  { 74, 71,  8, 11,  9},
  { 74, 70,  8, 11,  8},
  { 72, 70,  8, 11,  9},
  { 73, 71,  9,  8, 11},
  { 73, 71,  9,  9,  8},
  { 72, 72,  9, 10, 11},
  { 70, 68, 10,  8,  8},
  { 71, 72,  8, 10, 10},
  { 74, 72, 11,  7,  8},
  { 73, 72, 12,  8, 11},
  { 72, 69, 11,  8, 11},
  { 71, 69, 12, 11,  9},
  { 72, 71,  8,  8, 10},
  { 74, 68, 12, 10,  8},
  { 70, 72, 10, 11, 10},
  { 70, 69,  9,  9,  9},
  { 72, 72,  9, 11,  7},
  { 71, 71, 10,  8,  9},
  { 73, 70, 11,  8,  8},
  { 70, 68, 11,  7, 11},
  { 73, 71, 11,  9,  8},
  { 74, 70, 12,  9,  9},
  { 73, 69,  8, 11, 11},
  { 72, 69,  8, 10, 11},
  { 71, 68, 10,  8,  9},
  { 71, 69, 11, 10, 10},
  { 72, 70, 10,  9,  8},
  { 74, 72,  9,  8,  9},
  { 72, 68, 12, 11,  8},
  { 73, 69,  9, 11, 10},
  { 74, 72, 10,  8,  9},
  { 73, 72, 12,  7,  9},
  { 74, 70, 11,  7,  8},
  { 72, 70, 11,  7, 10},
  { 70, 71,  9,  9, 11},
  { 72, 69,  9,  9,  8},
  { 71, 69, 12, 10,  8},
  { 70, 68, 12, 10,  9},
  { 70, 69,  9, 10,  8},
  { 70, 68, 11,  7,  7},
  { 70, 71,  9, 10,  8},
  // Clase 5: Ajustar_Lentes (Nuevas 50 muestras)
  { 89, 88, 10, 91, 88},
  { 87, 88,  7, 86, 87},
  { 88, 86,  8, 89, 86},
  { 86, 89,  8, 89, 85},
  { 86, 87,  8, 91, 89},
  { 89, 87,  9, 89, 89},
  { 88, 90,  9, 88, 89},
  { 88, 88,  7, 89, 89},
  { 87, 89,  9, 90, 87},
  { 88, 86,  8, 88, 87},
  { 85, 88,  9, 92, 88},
  { 87, 90,  7, 90, 91},
  { 86, 89,  8, 91, 87},
  { 88, 88, 10, 87, 86},
  { 88, 87, 11, 90, 86},
  { 89, 89, 11, 89, 87},
  { 88, 90,  9, 92, 89},
  { 88, 87,  7, 88, 89},
  { 88, 86,  9, 90, 90},
  { 92, 89, 10, 90, 89},
  { 91, 85, 10, 87, 87},
  { 88, 88,  8, 92, 89},
  { 89, 89,  8, 88, 87},
  { 90, 86,  8, 91, 87},
  { 88, 86, 11, 87, 87},
  { 89, 88,  8, 89, 87},
  { 88, 89,  9, 88, 88},
  { 88, 89, 10, 88, 87},
  { 89, 88,  9, 91, 89},
  { 88, 88, 10, 88, 91},
  { 88, 87,  9, 88, 89},
  { 88, 88,  8, 88, 88},
  { 89, 88,  9, 87, 88},
  { 89, 89,  9, 89, 88},
  { 91, 87, 10, 89, 91},
  { 88, 89, 11, 90, 87},
  { 85, 90,  9, 91, 86},
  { 88, 88, 10, 90, 86},
  { 90, 88, 11, 92, 88},
  { 85, 86,  7, 89, 89},
  { 89, 88,  7, 89, 86},
  { 89, 87, 10, 88, 87},
  { 87, 88,  9, 89, 89},
  { 90, 90,  9, 90, 88},
  { 87, 86,  8, 87, 91},
  { 89, 86,  9, 88, 86},
  { 87, 87,  9, 88, 88},
  { 87, 88, 10, 86, 89},
  { 88, 87, 10, 87, 88},
  { 87, 89,  8, 90, 90}
};

const uint8_t BASE_Y[BASE_N] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

// ------------------------------------------------------------
//  Dataset propio (grabado con el guante), valores x100
// ------------------------------------------------------------
const uint16_t MAX_USUARIO = 500;
uint8_t  userX[MAX_USUARIO][NUM_FLEX];
uint8_t  userY[MAX_USUARIO];
uint16_t userN = 0;

// ------------------------------------------------------------
//  Parámetros KNN
// ------------------------------------------------------------
const uint8_t MAX_K = 15;
uint8_t K = 5;
float   UMBRAL_DIST = 0.35;   // si el vecino más cercano está más lejos -> "Desconocido"
bool    usarBase = true;      // usar el dataset de los CSV además del propio

// ------------------------------------------------------------
//  Estado
// ------------------------------------------------------------
Preferences prefs;
float rawAbierto[NUM_FLEX];
float rawCerrado[NUM_FLEX];
bool  calibrado = false;

float filtro[NUM_FLEX];
bool  filtroIniciado = false;

enum Modo { REPOSO, PREDICCION, MONITOR };
Modo modo = REPOSO;

const uint8_t CONFIRMACIONES = 3;  // predicciones iguales seguidas para aceptar un gesto
int  gestoCandidato = -2;
uint8_t contadorCandidato = 0;
int  gestoEstable = -2;

char bufferCmd[32];
uint8_t largoCmd = 0;

// ============================================================
//  Lectura de sensores
// ============================================================
void leerCrudo(float crudo[]) {
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    uint32_t suma = 0;
    for (uint8_t j = 0; j < LECTURAS_PROMEDIO; j++) {
      suma += analogRead(FLEX_PINS[i]);
    }
    crudo[i] = suma / (float)LECTURAS_PROMEDIO;
  }
}

void leerFiltrado(float crudo[]) {
  float actual[NUM_FLEX];
  leerCrudo(actual);
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    if (!filtroIniciado) filtro[i] = actual[i];
    else filtro[i] = ALFA_FILTRO * actual[i] + (1.0f - ALFA_FILTRO) * filtro[i];
    crudo[i] = filtro[i];
  }
  filtroIniciado = true;
}

float normalizar(uint8_t i, float crudo) {
  float rango = rawCerrado[i] - rawAbierto[i];
  if (fabsf(rango) < 1.0f) return 0.0f;
  float t = (crudo - rawAbierto[i]) / rango;
  float v = NORM_ABIERTO + t * (NORM_CERRADO - NORM_ABIERTO);
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  return v;
}

void leerNormalizado(float x[], float crudo[]) {
  leerFiltrado(crudo);
  for (uint8_t i = 0; i < NUM_FLEX; i++) x[i] = normalizar(i, crudo[i]);
}

// ============================================================
//  Calibración
// ============================================================
void cuentaRegresiva(uint8_t segundos) {
  for (int s = segundos; s > 0; s--) {
    Serial.printf("  %d...\n", s);
    delay(1000);
  }
}

void promediarPosicion(float salida[], uint16_t duracionMs) {
  float acumulado[NUM_FLEX] = {0};
  float crudo[NUM_FLEX];
  uint16_t n = 0;
  unsigned long inicio = millis();
  while (millis() - inicio < duracionMs) {
    leerCrudo(crudo);
    for (uint8_t i = 0; i < NUM_FLEX; i++) acumulado[i] += crudo[i];
    n++;
    delay(10);
  }
  for (uint8_t i = 0; i < NUM_FLEX; i++) salida[i] = acumulado[i] / n;
}

void guardarCalibracion() {
  prefs.begin("guante", false);
  prefs.putBytes("abierto", rawAbierto, sizeof(rawAbierto));
  prefs.putBytes("cerrado", rawCerrado, sizeof(rawCerrado));
  prefs.putBool("calibrado", true);
  prefs.end();
}

bool cargarCalibracion() {
  prefs.begin("guante", true);
  bool ok = prefs.getBool("calibrado", false);
  if (ok) {
    prefs.getBytes("abierto", rawAbierto, sizeof(rawAbierto));
    prefs.getBytes("cerrado", rawCerrado, sizeof(rawCerrado));
  }
  prefs.end();
  return ok;
}

void calibrar() {
  Modo anterior = modo;
  modo = REPOSO;
  Serial.println(F("\n=== CALIBRACION ==="));
  Serial.println(F("1) Extiende TODOS los dedos (mano abierta) y mantenlos quietos."));
  cuentaRegresiva(3);
  Serial.println(F("  Midiendo..."));
  promediarPosicion(rawAbierto, 2000);

  Serial.println(F("2) Cierra la mano en PUNO y mantenla quieta."));
  cuentaRegresiva(3);
  Serial.println(F("  Midiendo..."));
  promediarPosicion(rawCerrado, 2000);

  Serial.println(F("\nDedo      Abierto  Cerrado  Diferencia"));
  bool todoBien = true;
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    float dif = rawCerrado[i] - rawAbierto[i];
    Serial.printf("%-9s %7.0f  %7.0f  %8.0f", NOMBRE_DEDO[i], rawAbierto[i], rawCerrado[i], dif);
    if (fabsf(dif) < 150) {
      Serial.print(F("   <-- poca diferencia, revisa el sensor/conexion"));
      todoBien = false;
    }
    Serial.println();
  }

  guardarCalibracion();
  calibrado = true;
  filtroIniciado = false;
  Serial.println(todoBien ? F("Calibracion guardada.\n")
                          : F("Calibracion guardada, pero revisa los dedos marcados.\n"));
  modo = anterior;
}

// ============================================================
//  KNN
// ============================================================
struct Vecino {
  float   dist2;
  uint8_t clase;
};

Vecino  vecinos[MAX_K];
uint8_t numVecinos = 0;

void considerarMuestra(const float x[], const uint8_t fila[], uint8_t clase) {
  float d2 = 0;
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    float diff = x[i] - fila[i] / 100.0f;
    d2 += diff * diff;
  }
  if (numVecinos == K && d2 >= vecinos[K - 1].dist2) return;

  int j = (numVecinos < K) ? numVecinos : K - 1;
  while (j > 0 && vecinos[j - 1].dist2 > d2) {
    vecinos[j] = vecinos[j - 1];
    j--;
  }
  vecinos[j].dist2 = d2;
  vecinos[j].clase = clase;
  if (numVecinos < K) numVecinos++;
}

int clasificarKNN(const float x[], float &confianza, float &distMin) {
  numVecinos = 0;
  if (usarBase) {
    for (uint16_t n = 0; n < BASE_N; n++) considerarMuestra(x, BASE_X[n], BASE_Y[n]);
  }
  for (uint16_t n = 0; n < userN; n++) considerarMuestra(x, userX[n], userY[n]);

  confianza = 0;
  distMin = 99;
  if (numVecinos == 0) return -1;

  uint8_t votos[NUM_CLASES] = {0};
  float   sumaDist[NUM_CLASES] = {0};
  for (uint8_t v = 0; v < numVecinos; v++) {
    votos[vecinos[v].clase]++;
    sumaDist[vecinos[v].clase] += sqrtf(vecinos[v].dist2);
  }

  int mejor = 0;
  for (uint8_t c = 1; c < NUM_CLASES; c++) {
    if (votos[c] > votos[mejor] ||
        (votos[c] == votos[mejor] && votos[c] > 0 && sumaDist[c] < sumaDist[mejor])) {
      mejor = c;
    }
  }

  confianza = votos[mejor] / (float)numVecinos;
  distMin = sqrtf(vecinos[0].dist2);
  if (distMin > UMBRAL_DIST) return -1;
  return mejor;
}

const char* nombreClase(int c) {
  if (c >= 0 && c < NUM_CLASES) return NOMBRE_GESTO[c];
  return "Desconocido";
}

// ============================================================
//  Dataset propio
// ============================================================
void imprimirEncabezadoCSV() {
  Serial.println(F("muestra,flex_0,flex_1,flex_2,flex_3,flex_4,gyro_x,gyro_y,gyro_z,clase,gesto"));
}

void imprimirFilaCSV(uint16_t idx, const float x[], uint8_t clase) {
  Serial.printf("%u,%.2f,%.2f,%.2f,%.2f,%.2f,0.000,0.000,0.000,%u,%s\n",
                idx, x[0], x[1], x[2], x[3], x[4], clase, NOMBRE_GESTO[clase]);
}

void grabarGesto(uint8_t clase, uint16_t cantidad) {
  if (!calibrado) {
    Serial.println(F("Primero calibra con el comando: c"));
    return;
  }
  Modo anterior = modo;
  modo = REPOSO;

  Serial.printf("\n=== GRABANDO %s (clase %u), %u muestras ===\n",
                NOMBRE_GESTO[clase], clase, cantidad);
  Serial.println(F("Haz el gesto y mantenlo. Mueve un poco la mano entre muestras para dar variedad."));
  cuentaRegresiva(3);

  float x[NUM_FLEX], crudo[NUM_FLEX];
  filtroIniciado = false;
  for (uint8_t i = 0; i < 5; i++) { leerNormalizado(x, crudo); delay(20); }

  imprimirEncabezadoCSV();
  uint16_t grabadas = 0;
  for (uint16_t n = 0; n < cantidad; n++) {
    if (userN >= MAX_USUARIO) {
      Serial.println(F("# Memoria de muestras llena. Usa 'd' para exportar y 'b' para borrar."));
      break;
    }
    leerNormalizado(x, crudo);
    for (uint8_t i = 0; i < NUM_FLEX; i++) userX[userN][i] = (uint8_t)roundf(x[i] * 100.0f);
    userY[userN] = clase;
    imprimirFilaCSV(n, x, clase);
    userN++;
    grabadas++;
    delay(60);
  }
  Serial.printf("# Listo: %u muestras grabadas. Total propio: %u/%u\n",
                grabadas, userN, MAX_USUARIO);
  Serial.println(F("# Usa 'w' para guardarlas en la flash del ESP32.\n"));
  modo = anterior;
}

void volcarDataset() {
  Serial.println(F("\n# Dataset propio - Guante ESP32 (5 flex, giroscopio en 0)"));
  Serial.println(F("#   flex_0=pulgar | flex_1=indice | flex_2=medio | flex_3=anular | flex_4=menique"));
  Serial.println(F("#   0.0=extendido | 1.0=completamente doblado (flex)"));
  Serial.printf("# Total: %u muestras\n", userN);
  imprimirEncabezadoCSV();
  float x[NUM_FLEX];
  for (uint16_t n = 0; n < userN; n++) {
    for (uint8_t i = 0; i < NUM_FLEX; i++) x[i] = userX[n][i] / 100.0f;
    imprimirFilaCSV(n, x, userY[n]);
  }
  Serial.println(F("# Fin del dataset\n"));
}

void guardarDataset() {
  prefs.begin("dataset", false);
  prefs.putUShort("n", userN);
  if (userN > 0) {
    prefs.putBytes("x", userX, userN * NUM_FLEX);
    prefs.putBytes("y", userY, userN);
  }
  prefs.end();
  Serial.printf("Dataset propio guardado en flash (%u muestras).\n", userN);
}

void cargarDataset() {
  prefs.begin("dataset", true);
  uint16_t n = prefs.getUShort("n", 0);
  if (n > MAX_USUARIO) n = 0;
  if (n > 0) {
    prefs.getBytes("x", userX, n * NUM_FLEX);
    prefs.getBytes("y", userY, n);
  }
  prefs.end();
  userN = n;
}

void borrarDataset() {
  userN = 0;
  prefs.begin("dataset", false);
  prefs.clear();
  prefs.end();
  Serial.println(F("Muestras propias borradas (RAM y flash)."));
}

void resumenDataset() {
  uint16_t cuenta[NUM_CLASES] = {0};
  for (uint16_t n = 0; n < userN; n++) cuenta[userY[n]]++;
  Serial.printf("\nDataset base: %u muestras (%s)\n", BASE_N, usarBase ? "EN USO" : "desactivado");
  Serial.printf("Dataset propio: %u/%u muestras\n", userN, MAX_USUARIO);
  for (uint8_t c = 0; c < NUM_CLASES; c++) {
    Serial.printf("  %u %-14s %u\n", c, NOMBRE_GESTO[c], cuenta[c]);
  }
  Serial.printf("K = %u | umbral = %.2f\n\n", K, UMBRAL_DIST);
}

// ============================================================
//  Modos de tiempo real
// ============================================================
void pasoMonitor() {
  float x[NUM_FLEX], crudo[NUM_FLEX];
  leerNormalizado(x, crudo);
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    Serial.printf("%s:%.2f", NOMBRE_DEDO[i], x[i]);
    Serial.print(i < NUM_FLEX - 1 ? " " : "\n");
  }
}

void pasoPrediccion() {
  float x[NUM_FLEX], crudo[NUM_FLEX];
  leerNormalizado(x, crudo);

  float confianza, distMin;
  int clase = clasificarKNN(x, confianza, distMin);

  if (clase == gestoCandidato) {
    if (contadorCandidato < 255) contadorCandidato++;
  } else {
    gestoCandidato = clase;
    contadorCandidato = 1;
  }
  if (contadorCandidato >= CONFIRMACIONES) gestoEstable = gestoCandidato;

  Serial.printf("[%.2f %.2f %.2f %.2f %.2f]  ->  %-14s conf %3.0f%%  dist %.3f",
                x[0], x[1], x[2], x[3], x[4],
                nombreClase(gestoEstable), confianza * 100.0f, distMin);
  if (clase != gestoEstable) Serial.printf("  (posible: %s)", nombreClase(clase));
  Serial.println();
}

// ============================================================
//  Comandos por el monitor serie
// ============================================================
void mostrarAyuda() {
  Serial.println(F("\n================ COMANDOS ================"));
  Serial.println(F(" c         Calibrar (mano abierta y puno)"));
  Serial.println(F(" m         Monitor de los 5 dedos (sirve con Serial Plotter)"));
  Serial.println(F(" p         Prediccion KNN en tiempo real"));
  Serial.println(F(" s         Detener (modo reposo)"));
  Serial.println(F(" g<clase>  Grabar 50 muestras de un gesto. Ej: g2"));
  Serial.println(F(" g<c> <n>  Grabar n muestras. Ej: g4 80"));
  Serial.println(F(" d         Imprimir dataset propio en CSV"));
  Serial.println(F(" w         Guardar dataset propio en flash"));
  Serial.println(F(" b         Borrar dataset propio"));
  Serial.println(F(" base      Activar/desactivar dataset de los CSV"));
  Serial.println(F(" k<n>      Cambiar K (impar recomendado). Ej: k7"));
  Serial.println(F(" u<valor>  Umbral de 'Desconocido'. Ej: u0.30"));
  Serial.println(F(" i         Informacion del dataset"));
  Serial.println(F("------------------------------------------"));
  for (uint8_t c = 0; c < NUM_CLASES; c++) {
    Serial.printf(" Clase %u = %s\n", c, NOMBRE_GESTO[c]);
  }
  Serial.println(F("==========================================\n"));
}

void procesarComando(char* cmd) {
  while (*cmd == ' ') cmd++;
  int len = strlen(cmd);
  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r')) cmd[--len] = '\0';
  if (len == 0) return;

  if (strcmp(cmd, "base") == 0) {
    usarBase = !usarBase;
    Serial.printf("Dataset base %s\n", usarBase ? "ACTIVADO" : "DESACTIVADO");
    return;
  }

  switch (cmd[0]) {
    case 'h': mostrarAyuda(); break;
    case 'c': calibrar(); break;
    case 'm':
      if (!calibrado) { Serial.println(F("Primero calibra con: c")); break; }
      modo = MONITOR; break;
    case 'p':
      if (!calibrado) { Serial.println(F("Primero calibra con: c")); break; }
      gestoCandidato = -2; contadorCandidato = 0; gestoEstable = -2;
      modo = PREDICCION;
      Serial.println(F("Prediccion en tiempo real. Escribe 's' para detener."));
      break;
    case 's': modo = REPOSO; Serial.println(F("Detenido.")); break;
    case 'd': volcarDataset(); break;
    case 'w': guardarDataset(); break;
    case 'b': borrarDataset(); break;
    case 'i': resumenDataset(); break;
    case 'g': {
      char* fin;
      long clase = strtol(cmd + 1, &fin, 10);
      if (fin == cmd + 1 || clase < 0 || clase >= NUM_CLASES) {
        Serial.println(F("Uso: g<clase> [cantidad]. Ej: g2  o  g2 80"));
        break;
      }
      long cantidad = strtol(fin, NULL, 10);
      if (cantidad <= 0) cantidad = 50;
      if (cantidad > MAX_USUARIO) cantidad = MAX_USUARIO;
      grabarGesto((uint8_t)clase, (uint16_t)cantidad);
      break;
    }
    case 'k': {
      long nk = strtol(cmd + 1, NULL, 10);
      if (nk >= 1 && nk <= MAX_K) {
        K = (uint8_t)nk;
        Serial.printf("K = %u\n", K);
      } else {
        Serial.printf("K debe estar entre 1 y %u\n", MAX_K);
      }
      break;
    }
    case 'u': {
      float nu = atof(cmd + 1);
      if (nu > 0.0f) {
        UMBRAL_DIST = nu;
        Serial.printf("Umbral = %.2f\n", UMBRAL_DIST);
      }
      break;
    }
    default:
      Serial.println(F("Comando no reconocido. Escribe h para ayuda."));
  }
}

void leerSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n') {
      bufferCmd[largoCmd] = '\0';
      procesarComando(bufferCmd);
      largoCmd = 0;
    } else if (largoCmd < sizeof(bufferCmd) - 1) {
      bufferCmd[largoCmd++] = ch;
    }
  }
}

// ============================================================
//  setup / loop
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  analogReadResolution(12);   // 0 - 4095
  for (uint8_t i = 0; i < NUM_FLEX; i++) pinMode(FLEX_PINS[i], INPUT);

  calibrado = cargarCalibracion();
  cargarDataset();

  Serial.println(F("\nGuante de gestos ESP32 - KNN con 5 sensores flex"));
  if (calibrado) Serial.println(F("Calibracion cargada desde flash."));
  else           Serial.println(F("Sin calibracion: escribe 'c' para calibrar."));
  resumenDataset();
  mostrarAyuda();
}

void loop() {
  leerSerial();

  if (modo == MONITOR) {
    pasoMonitor();
    delay(500); // Pausa de 500 ms entre cada lectura
  } else if (modo == PREDICCION) {
    pasoPrediccion();
    delay(1000); // Pausa de 500 ms entre cada predicción
  }
}