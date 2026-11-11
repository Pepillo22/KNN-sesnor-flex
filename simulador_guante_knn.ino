/*
 * ============================================================
 *  SIMULADOR INTERACTIVO - Guante de gestos con KNN (ESP32)
 * ============================================================
 *
 *  Permite probar el clasificador KNN SIN tener el guante armado.
 *  Hay dos fuentes de datos para los 5 dedos:
 *
 *   1) VIRTUAL (por defecto): una "mano virtual" que controlas
 *      escribiendo comandos en el Monitor Serie.
 *      Funciona en cualquier ESP32 sin nada conectado.
 *
 *   2) SENSORES: lee los pines analógicos. Sirve con los sensores
 *      flex reales o con 5 potenciómetros deslizantes en Wokwi
 *      (usa el archivo diagram.json incluido).
 *
 *  Pines (iguales al esquema del guante):
 *    Pulgar GPIO36 | Índice GPIO39 | Medio GPIO34 | Anular GPIO35 | Meñique GPIO32
 *
 *  Monitor Serie: 115200 baudios, fin de línea "Nueva línea".
 *  Escribe  ayuda  para ver todos los comandos.
 * ============================================================
 */

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ------------------------------------------------------------
//  Configuración
// ------------------------------------------------------------
const uint8_t NUM_FLEX = 5;
const uint8_t FLEX_PINS[NUM_FLEX] = {36, 39, 34, 35, 32};
const char*   NOMBRE_DEDO[NUM_FLEX] = {"pulgar", "indice", "medio", "anular", "menique"};

const uint8_t NUM_CLASES = 7;
const char* NOMBRE_GESTO[NUM_CLASES] = {
  "Gesto_C", "Punio_Cerrado", "Mano_Abierta", "Gesto_L", "Gesto_OK", "Ajustar_Lentes", "Cuernos_Rock"
};

const float NORM_ABIERTO = 0.08;   // valor de dedo extendido en el dataset
const float NORM_CERRADO = 0.90;   // valor de dedo doblado (puño) en el dataset

const uint8_t LECTURAS_PROMEDIO = 16;
const float   ALFA_FILTRO = 0.35;

// ------------------------------------------------------------
//  Dataset base (tus CSV), valores x100
// ------------------------------------------------------------
const uint16_t BASE_N = 350;
const uint8_t BASE_X[BASE_N][NUM_FLEX] = {
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
{89, 88, 10, 91, 88},
  {87, 88,  7, 86, 87},
  {88, 86,  8, 89, 86},
  {86, 89,  8, 89, 85},
  {86, 87,  8, 91, 89},
  {89, 87,  9, 89, 89},
  {88, 90,  9, 88, 89},
  {88, 88,  7, 89, 89},
  {87, 89,  9, 90, 87},
  {88, 86,  8, 88, 87},
  {85, 88,  9, 92, 88},
  {87, 90,  7, 90, 91},
  {86, 89,  8, 91, 87},
  {88, 88, 10, 87, 86},
  {88, 87, 11, 90, 86},
  {89, 89, 11, 89, 87},
  {88, 90,  9, 92, 89},
  {88, 87,  7, 88, 89},
  {88, 86,  9, 90, 90},
  {92, 89, 10, 90, 89},
  {91, 85, 10, 87, 87},
  {88, 88,  8, 92, 89},
  {89, 89,  8, 88, 87},
  {90, 86,  8, 91, 87},
  {88, 86, 11, 87, 87},
  {89, 88,  8, 89, 87},
  {88, 89,  9, 88, 88},
  {88, 89, 10, 88, 87},
  {89, 88,  9, 91, 89},
  {88, 88, 10, 88, 91},
  {88, 87,  9, 88, 89},
  {88, 88,  8, 88, 88},
  {89, 88,  9, 87, 88},
  {89, 89,  9, 89, 88},
  {91, 87, 10, 89, 91},
  {88, 89, 11, 90, 87},
  {85, 90,  9, 91, 86},
  {88, 88, 10, 90, 86},
  {90, 88, 11, 92, 88},
  {85, 86,  7, 89, 89},
  {89, 88,  7, 89, 86},
  {89, 87, 10, 88, 87},
  {87, 88,  9, 89, 89},
  {90, 90,  9, 90, 88},
  {87, 86,  8, 87, 91},
  {89, 86,  9, 88, 86},
  {87, 87,  9, 88, 88},
  {87, 88, 10, 86, 89},
  {88, 87, 10, 87, 88},
  {87, 89,  8, 90, 90}
,
{ 82, 10, 89, 92, 10 },
{ 90, 11, 92, 89,  9 },
{ 90,  9, 89, 89, 10 },
{ 89,  8, 92, 91,  6 },
{ 90,  9, 92, 90,  9 },
{ 89,  8, 88, 91, 12 },
{ 89,  8, 88, 88,  9 },
{ 90, 12, 86, 93, 11 },
{ 88, 10, 88, 87, 10 },
{ 84,  6, 87, 91,  8 },
{ 90,  8, 94, 91,  9 },
{ 91,  6, 90, 88,  9 },
{ 88, 10, 89, 88,  7 },
{ 88,  7, 94, 90,  7 },
{ 88,  9, 90, 92,  7 },
{ 88,  8, 90, 94, 11 },
{ 88,  8, 86, 93, 10 },
{ 91,  7, 93, 88,  6 },
{ 91, 10, 91, 89, 11 },
{ 90,  7, 90, 94,  8 },
{ 87,  7, 89, 94,  8 },
{ 87,  8, 87, 91, 10 },
{ 92,  9, 88, 91,  7 },
{ 88,  9, 89, 87, 10 },
{ 86,  8, 85, 90,  8 },
{ 84, 10, 91, 93,  8 },
{ 91, 10, 91, 89, 10 },
{ 82,  7, 91, 95,  9 },
{ 84, 12, 94, 95,  8 },
{ 87,  9, 91, 94,  9 },
{ 90,  8, 90, 84, 11 },
{ 87,  9, 90, 90,  8 },
{ 88,  8, 89, 92,  7 },
{ 85, 11, 92, 87, 10 },
{ 87, 11, 90, 90,  7 },
{ 84,  7, 88, 89,  8 },
{ 90,  9, 92, 91,  9 },
{ 89,  9, 92, 90,  8 },
{ 89,  8, 91, 89, 10 },
{ 92,  7, 88, 88, 10 },
{ 87,  8, 89, 88,  9 },
{ 86,  9, 91, 92,  8 },
{ 88,  8, 90, 88, 10 },
{ 87, 10, 90, 92,  9 },
{ 83,  7, 95, 90,  8 },
{ 88,  9, 90, 94, 10 },
{ 86,  8, 90, 91, 10 },
{ 90, 10, 90, 92, 10 },
{ 86, 10, 89, 92, 11 },
{ 86, 10, 91, 91,  8 }};
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6
};

// ------------------------------------------------------------
//  Dataset propio (grabado durante la simulación)
// ------------------------------------------------------------
const uint16_t MAX_USUARIO = 500;
uint8_t  userX[MAX_USUARIO][NUM_FLEX];
uint8_t  userY[MAX_USUARIO];
uint16_t userN = 0;

// ------------------------------------------------------------
//  KNN
// ------------------------------------------------------------
const uint8_t MAX_K = 15;
uint8_t K = 5;
float   UMBRAL_DIST = 0.35;
bool    usarBase = true;

struct Vecino {
  float    dist2;
  uint8_t  clase;
  uint16_t indice;
  bool     propio;
};
Vecino  vecinos[MAX_K];
uint8_t numVecinos = 0;

// ------------------------------------------------------------
//  Estado de la simulación
// ------------------------------------------------------------
enum Fuente { VIRTUAL, SENSORES };
Fuente fuente = VIRTUAL;

float manoVirtual[NUM_FLEX] = {0.08, 0.08, 0.08, 0.08, 0.08};
float ruido = 0.02;                       // desviación del ruido simulado
float centroide[NUM_CLASES][NUM_FLEX];    // "gesto típico" de cada clase

float rawAbierto[NUM_FLEX];
float rawCerrado[NUM_FLEX];
float filtro[NUM_FLEX];
bool  filtroIniciado = false;

Preferences prefs;

bool enVivo = false;
int  gestoCandidato = -2, gestoEstable = -2;
uint8_t contadorCandidato = 0;
const uint8_t CONFIRMACIONES = 3;

char bufferCmd[64];
uint8_t largoCmd = 0;

// ============================================================
//  Utilidades
// ============================================================
float limitar01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

// Ruido aproximadamente gaussiano con desviación "sigma"
float ruidoGauss(float sigma) {
  float s = 0;
  for (uint8_t i = 0; i < 12; i++) s += random(0, 10001) / 10000.0f;
  return (s - 6.0f) * sigma;
}

const char* nombreClase(int c) {
  if (c >= 0 && c < NUM_CLASES) return NOMBRE_GESTO[c];
  return "Desconocido";
}

void imprimirBarra(float v) {
  const uint8_t ANCHO = 20;
  uint8_t llenos = (uint8_t)roundf(limitar01(v) * ANCHO);
  Serial.print('|');
  for (uint8_t i = 0; i < ANCHO; i++) Serial.print(i < llenos ? '#' : '.');
  Serial.print('|');
}

void imprimirMano(const float x[]) {
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    Serial.printf("  %-8s ", NOMBRE_DEDO[i]);
    imprimirBarra(x[i]);
    Serial.printf(" %.2f\n", x[i]);
  }
}

void calcularCentroides() {
  float suma[NUM_CLASES][NUM_FLEX] = {{0}};
  uint16_t cuenta[NUM_CLASES] = {0};
  for (uint16_t n = 0; n < BASE_N; n++) {
    for (uint8_t i = 0; i < NUM_FLEX; i++) suma[BASE_Y[n]][i] += BASE_X[n][i] / 100.0f;
    cuenta[BASE_Y[n]]++;
  }
  for (uint8_t c = 0; c < NUM_CLASES; c++)
    for (uint8_t i = 0; i < NUM_FLEX; i++)
      centroide[c][i] = cuenta[c] ? suma[c][i] / cuenta[c] : 0.5f;
}

// ============================================================
//  Fuentes de datos
// ============================================================
void leerSensores(float x[]) {
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    uint32_t s = 0;
    for (uint8_t j = 0; j < LECTURAS_PROMEDIO; j++) s += analogRead(FLEX_PINS[i]);
    float crudo = s / (float)LECTURAS_PROMEDIO;
    filtro[i] = filtroIniciado ? ALFA_FILTRO * crudo + (1 - ALFA_FILTRO) * filtro[i] : crudo;

    float rango = rawCerrado[i] - rawAbierto[i];
    float t = (fabsf(rango) < 1.0f) ? 0.0f : (filtro[i] - rawAbierto[i]) / rango;
    x[i] = limitar01(NORM_ABIERTO + t * (NORM_CERRADO - NORM_ABIERTO));
  }
  filtroIniciado = true;
}

void leerVirtual(float x[]) {
  for (uint8_t i = 0; i < NUM_FLEX; i++) x[i] = limitar01(manoVirtual[i] + ruidoGauss(ruido));
}

void leerMano(float x[]) {
  if (fuente == VIRTUAL) leerVirtual(x);
  else leerSensores(x);
}

// ============================================================
//  KNN
// ============================================================
void considerar(const float x[], const uint8_t fila[], uint8_t clase, uint16_t idx, bool propio) {
  float d2 = 0;
  for (uint8_t i = 0; i < NUM_FLEX; i++) {
    float d = x[i] - fila[i] / 100.0f;
    d2 += d * d;
  }
  if (numVecinos == K && d2 >= vecinos[K - 1].dist2) return;
  int j = (numVecinos < K) ? numVecinos : K - 1;
  while (j > 0 && vecinos[j - 1].dist2 > d2) { vecinos[j] = vecinos[j - 1]; j--; }
  vecinos[j].dist2 = d2;
  vecinos[j].clase = clase;
  vecinos[j].indice = idx;
  vecinos[j].propio = propio;
  if (numVecinos < K) numVecinos++;
}

int clasificarKNN(const float x[], float &confianza, float &distMin, uint8_t votos[]) {
  numVecinos = 0;
  if (usarBase) for (uint16_t n = 0; n < BASE_N; n++) considerar(x, BASE_X[n], BASE_Y[n], n, false);
  for (uint16_t n = 0; n < userN; n++) considerar(x, userX[n], userY[n], n, true);

  float sumaDist[NUM_CLASES] = {0};
  for (uint8_t c = 0; c < NUM_CLASES; c++) votos[c] = 0;
  confianza = 0; distMin = 99;
  if (numVecinos == 0) return -1;

  for (uint8_t v = 0; v < numVecinos; v++) {
    votos[vecinos[v].clase]++;
    sumaDist[vecinos[v].clase] += sqrtf(vecinos[v].dist2);
  }
  int mejor = 0;
  for (uint8_t c = 1; c < NUM_CLASES; c++) {
    if (votos[c] > votos[mejor] ||
        (votos[c] == votos[mejor] && votos[c] > 0 && sumaDist[c] < sumaDist[mejor])) mejor = c;
  }
  confianza = votos[mejor] / (float)numVecinos;
  distMin = sqrtf(vecinos[0].dist2);
  if (distMin > UMBRAL_DIST) return -1;
  return mejor;
}

// ============================================================
//  Comandos de la simulación
// ============================================================
void cmdClasificar() {
  float x[NUM_FLEX];
  leerMano(x);
  float conf, dmin;
  uint8_t votos[NUM_CLASES];
  int c = clasificarKNN(x, conf, dmin, votos);

  Serial.println(F("\n--- Lectura de la mano ---"));
  imprimirMano(x);

  Serial.printf("\n--- Los %u vecinos mas cercanos ---\n", numVecinos);
  for (uint8_t v = 0; v < numVecinos; v++) {
    Serial.printf("  %2u) %-14s distancia %.3f  (%s #%u)\n", v + 1,
                  NOMBRE_GESTO[vecinos[v].clase], sqrtf(vecinos[v].dist2),
                  vecinos[v].propio ? "propio" : "base", vecinos[v].indice);
  }

  Serial.println(F("\n--- Votacion ---"));
  for (uint8_t k = 0; k < NUM_CLASES; k++) {
    if (votos[k] == 0) continue;
    Serial.printf("  %-14s ", NOMBRE_GESTO[k]);
    for (uint8_t v = 0; v < votos[k]; v++) Serial.print("[*]");
    Serial.printf(" %u voto(s)\n", votos[k]);
  }

  Serial.printf("\n>>> RESULTADO: %s  (confianza %.0f%%, distancia minima %.3f)\n",
                nombreClase(c), conf * 100, dmin);
  if (c < 0 && numVecinos > 0)
    Serial.printf("    El vecino mas cercano esta lejos (umbral %.2f): gesto no reconocido.\n", UMBRAL_DIST);
  Serial.println();
}

void cmdGesto(int c) {
  if (c < 0 || c >= NUM_CLASES) {
    Serial.println(F("Uso: gesto <0-6>"));
    return;
  }
  for (uint8_t i = 0; i < NUM_FLEX; i++) manoVirtual[i] = centroide[c][i];
  fuente = VIRTUAL;
  Serial.printf("Mano virtual en posicion: %s\n", NOMBRE_GESTO[c]);
  imprimirMano(manoVirtual);
}

void cmdAnimar(int destino) {
  if (destino < 0 || destino >= NUM_CLASES) {
    Serial.println(F("Uso: animar <0-6>"));
    return;
  }
  fuente = VIRTUAL;
  float inicio[NUM_FLEX];
  memcpy(inicio, manoVirtual, sizeof(inicio));
  const uint8_t PASOS = 20;
  Serial.printf("\nTransicion hacia %s:\n", NOMBRE_GESTO[destino]);
  for (uint8_t p = 0; p <= PASOS; p++) {
    float t = p / (float)PASOS;
    for (uint8_t i = 0; i < NUM_FLEX; i++)
      manoVirtual[i] = inicio[i] + (centroide[destino][i] - inicio[i]) * t;
    float x[NUM_FLEX], conf, dmin;
    uint8_t votos[NUM_CLASES];
    leerMano(x);
    int c = clasificarKNN(x, conf, dmin, votos);
    Serial.printf("  %3.0f%%  [%.2f %.2f %.2f %.2f %.2f]  ->  %-14s dist %.3f\n",
                  t * 100, x[0], x[1], x[2], x[3], x[4], nombreClase(c), dmin);
    delay(120);
  }
  Serial.println();
}

void cmdAzar() {
  fuente = VIRTUAL;
  for (uint8_t i = 0; i < NUM_FLEX; i++) manoVirtual[i] = random(0, 101) / 100.0f;
  Serial.println(F("Mano virtual en posicion aleatoria:"));
  imprimirMano(manoVirtual);
  Serial.println(F("Escribe 'clasificar' para ver que gesto detecta el KNN."));
}

// Evalúa el KNN con muestras simuladas de cada gesto y muestra la matriz de confusión
void cmdPrueba(int porClase) {
  if (porClase <= 0) porClase = 20;
  if (porClase > 200) porClase = 200;
  uint16_t matriz[NUM_CLASES][NUM_CLASES + 1] = {{0}};
  uint16_t aciertos = 0, total = 0;

  for (uint8_t c = 0; c < NUM_CLASES; c++) {
    for (int n = 0; n < porClase; n++) {
      float x[NUM_FLEX], conf, dmin;
      uint8_t votos[NUM_CLASES];
      for (uint8_t i = 0; i < NUM_FLEX; i++) x[i] = limitar01(centroide[c][i] + ruidoGauss(ruido));
      int pred = clasificarKNN(x, conf, dmin, votos);
      matriz[c][pred < 0 ? NUM_CLASES : pred]++;
      if (pred == c) aciertos++;
      total++;
    }
  }

  Serial.printf("\n=== PRUEBA AUTOMATICA: %d muestras por gesto, ruido %.3f, K=%u ===\n",
                porClase, ruido, K);
  Serial.println(F("Filas = gesto real | Columnas = lo que predijo el KNN"));
  const char* cols[NUM_CLASES + 1] = {"C", "Pun", "Abi", "L", "OK", "Len", "Rck", "Desc"};
  Serial.printf("%15s", "");
  for (uint8_t p = 0; p <= NUM_CLASES; p++) Serial.printf("%5s ", cols[p]);
  Serial.println();
  for (uint8_t c = 0; c < NUM_CLASES; c++) {
    Serial.printf("  %-13s", NOMBRE_GESTO[c]);
    for (uint8_t p = 0; p <= NUM_CLASES; p++) Serial.printf("%5u ", matriz[c][p]);
    Serial.println();
  }
  Serial.printf("\nPrecision: %u/%u = %.1f%%\n", aciertos, total, 100.0f * aciertos / total);
  Serial.println(F("Tip: sube el ruido (ej. 'ruido 0.15') y repite la prueba para ver como baja.\n"));
}

void cmdGrabar(int clase, int cantidad) {
  if (clase < 0 || clase >= NUM_CLASES) {
    Serial.println(F("Uso: grabar <clase 0-6> [cantidad]"));
    return;
  }
  if (cantidad <= 0) cantidad = 20;
  Serial.printf("\nGrabando %d muestras como %s desde la fuente %s...\n",
                cantidad, NOMBRE_GESTO[clase], fuente == VIRTUAL ? "VIRTUAL" : "SENSORES");
  Serial.println(F("muestra,flex_0,flex_1,flex_2,flex_3,flex_4,gyro_x,gyro_y,gyro_z,clase,gesto"));
  int grabadas = 0;
  for (int n = 0; n < cantidad && userN < MAX_USUARIO; n++) {
    float x[NUM_FLEX];
    leerMano(x);
    for (uint8_t i = 0; i < NUM_FLEX; i++) userX[userN][i] = (uint8_t)roundf(x[i] * 100);
    userY[userN] = clase;
    Serial.printf("%d,%.2f,%.2f,%.2f,%.2f,%.2f,0.000,0.000,0.000,%d,%s\n",
                  n, x[0], x[1], x[2], x[3], x[4], clase, NOMBRE_GESTO[clase]);
    userN++;
    grabadas++;
    delay(fuente == SENSORES ? 60 : 5);
  }
  Serial.printf("Grabadas %d. Total propio: %u/%u\n\n", grabadas, userN, MAX_USUARIO);
}

void cmdDatos() {
  uint16_t cuenta[NUM_CLASES] = {0};
  for (uint16_t n = 0; n < userN; n++) cuenta[userY[n]]++;
  Serial.printf("\nDataset base: %u muestras (%s)\n", BASE_N, usarBase ? "EN USO" : "desactivado");
  Serial.printf("Dataset propio: %u/%u muestras\n", userN, MAX_USUARIO);
  for (uint8_t c = 0; c < NUM_CLASES; c++)
    Serial.printf("  %u %-14s propias: %u\n", c, NOMBRE_GESTO[c], cuenta[c]);
  Serial.printf("Fuente: %s | K = %u | umbral = %.2f | ruido = %.3f\n\n",
                fuente == VIRTUAL ? "VIRTUAL" : "SENSORES", K, UMBRAL_DIST, ruido);
}

void cmdExportar() {
  Serial.println(F("\n# Dataset propio (simulacion)"));
  Serial.println(F("muestra,flex_0,flex_1,flex_2,flex_3,flex_4,gyro_x,gyro_y,gyro_z,clase,gesto"));
  for (uint16_t n = 0; n < userN; n++) {
    Serial.printf("%u,%.2f,%.2f,%.2f,%.2f,%.2f,0.000,0.000,0.000,%u,%s\n", n,
                  userX[n][0] / 100.0f, userX[n][1] / 100.0f, userX[n][2] / 100.0f,
                  userX[n][3] / 100.0f, userX[n][4] / 100.0f, userY[n], NOMBRE_GESTO[userY[n]]);
  }
  Serial.println(F("# Fin\n"));
}

void cuentaRegresiva(uint8_t s) {
  for (int i = s; i > 0; i--) { Serial.printf("  %d...\n", i); delay(1000); }
}

void promediarPosicion(float salida[]) {
  float acc[NUM_FLEX] = {0};
  uint16_t n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    for (uint8_t i = 0; i < NUM_FLEX; i++) acc[i] += analogRead(FLEX_PINS[i]);
    n++;
    delay(10);
  }
  for (uint8_t i = 0; i < NUM_FLEX; i++) salida[i] = acc[i] / n;
}

void cmdCalibrar() {
  Serial.println(F("\n=== CALIBRACION (fuente SENSORES) ==="));
  Serial.println(F("1) Mano abierta (en Wokwi: todos los potenciometros al minimo)"));
  cuentaRegresiva(3);
  promediarPosicion(rawAbierto);
  Serial.println(F("2) Puno cerrado (en Wokwi: todos al maximo)"));
  cuentaRegresiva(3);
  promediarPosicion(rawCerrado);
  for (uint8_t i = 0; i < NUM_FLEX; i++)
    Serial.printf("  %-8s abierto %6.0f  cerrado %6.0f\n", NOMBRE_DEDO[i], rawAbierto[i], rawCerrado[i]);
  prefs.begin("guante", false);
  prefs.putBytes("abierto", rawAbierto, sizeof(rawAbierto));
  prefs.putBytes("cerrado", rawCerrado, sizeof(rawCerrado));
  prefs.putBool("calibrado", true);
  prefs.end();
  filtroIniciado = false;
  Serial.println(F("Calibracion guardada.\n"));
}

void pasoEnVivo() {
  float x[NUM_FLEX], conf, dmin;
  uint8_t votos[NUM_CLASES];
  leerMano(x);
  int c = clasificarKNN(x, conf, dmin, votos);
  if (c == gestoCandidato) { if (contadorCandidato < 255) contadorCandidato++; }
  else { gestoCandidato = c; contadorCandidato = 1; }
  if (contadorCandidato >= CONFIRMACIONES) gestoEstable = gestoCandidato;

  Serial.printf("[%.2f %.2f %.2f %.2f %.2f] -> %-14s conf %3.0f%% dist %.3f\n",
                x[0], x[1], x[2], x[3], x[4], nombreClase(gestoEstable), conf * 100, dmin);
}

void mostrarAyuda() {
  Serial.println(F("\n================= SIMULADOR KNN - COMANDOS ================="));
  Serial.println(F(" MANO VIRTUAL"));
  Serial.println(F("  gesto <0-6>           Poner la mano en un gesto del dataset"));
  Serial.println(F("  dedo <0-4> <0-1>      Mover un dedo. Ej: dedo 1 0.9"));
  Serial.println(F("  mano a b c d e        Fijar los 5 dedos. Ej: mano 0.1 0.1 0.9 0.9 0.9"));
  Serial.println(F("  azar                  Posicion aleatoria"));
  Serial.println(F("  ruido <valor>         Ruido de los sensores simulados. Ej: ruido 0.05"));
  Serial.println(F("  ver                   Mostrar la mano actual"));
  Serial.println(F(" KNN"));
  Serial.println(F("  clasificar            Clasificar y explicar paso a paso"));
  Serial.println(F("  vivo / parar          Prediccion continua en tiempo real"));
  Serial.println(F("  animar <0-6>          Transicion animada hacia un gesto"));
  Serial.println(F("  prueba [n]            Evaluar precision con n muestras por gesto"));
  Serial.println(F("  k <n>                 Cambiar K. Ej: k 7"));
  Serial.println(F("  umbral <valor>        Distancia maxima para aceptar un gesto"));
  Serial.println(F(" DATOS"));
  Serial.println(F("  grabar <clase> [n]    Grabar n muestras de la fuente actual"));
  Serial.println(F("  datos / exportar      Resumen / imprimir dataset propio en CSV"));
  Serial.println(F("  borrar                Borrar dataset propio"));
  Serial.println(F("  base                  Activar/desactivar dataset de los CSV"));
  Serial.println(F(" FUENTE"));
  Serial.println(F("  fuente virtual        Usar la mano virtual"));
  Serial.println(F("  fuente sensores       Usar pines analogicos (flex o potenciometros)"));
  Serial.println(F("  calibrar              Calibrar la fuente de sensores"));
  Serial.println(F("------------------------------------------------------------"));
  for (uint8_t c = 0; c < NUM_CLASES; c++) Serial.printf("  Clase %u = %s\n", c, NOMBRE_GESTO[c]);
  Serial.println(F("============================================================\n"));
}

// ============================================================
//  Intérprete de comandos
// ============================================================
void procesarComando(char* linea) {
  for (char* p = linea; *p; p++) *p = tolower(*p);
  char* cmd = strtok(linea, " \t\r");
  if (!cmd) return;
  char* a1 = strtok(NULL, " \t\r");
  char* a2 = strtok(NULL, " \t\r");

  if (enVivo && strcmp(cmd, "parar") != 0) enVivo = false;

  if      (!strcmp(cmd, "ayuda") || !strcmp(cmd, "h")) mostrarAyuda();
  else if (!strcmp(cmd, "gesto"))      cmdGesto(a1 ? atoi(a1) : -1);
  else if (!strcmp(cmd, "dedo")) {
    int d = a1 ? atoi(a1) : -1;
    if (d < 0 || d >= NUM_FLEX || !a2) { Serial.println(F("Uso: dedo <0-4> <0.0-1.0>")); return; }
    manoVirtual[d] = limitar01(atof(a2));
    fuente = VIRTUAL;
    imprimirMano(manoVirtual);
  }
  else if (!strcmp(cmd, "mano")) {
    float v[NUM_FLEX];
    char* tok = a1;
    uint8_t n = 0;
    if (tok) { v[n++] = atof(tok); }
    if (a2) { v[n++] = atof(a2); }
    while (n < NUM_FLEX && (tok = strtok(NULL, " \t\r"))) v[n++] = atof(tok);
    if (n < NUM_FLEX) { Serial.println(F("Uso: mano a b c d e  (5 valores de 0 a 1)")); return; }
    for (uint8_t i = 0; i < NUM_FLEX; i++) manoVirtual[i] = limitar01(v[i]);
    fuente = VIRTUAL;
    imprimirMano(manoVirtual);
  }
  else if (!strcmp(cmd, "azar"))       cmdAzar();
  else if (!strcmp(cmd, "ruido")) {
    if (a1) ruido = fabsf(atof(a1));
    Serial.printf("Ruido = %.3f\n", ruido);
  }
  else if (!strcmp(cmd, "ver")) {
    float x[NUM_FLEX];
    if (fuente == VIRTUAL) imprimirMano(manoVirtual);
    else { leerMano(x); imprimirMano(x); }
  }
  else if (!strcmp(cmd, "clasificar") || !strcmp(cmd, "c")) cmdClasificar();
  else if (!strcmp(cmd, "vivo")) {
    enVivo = true; gestoCandidato = gestoEstable = -2; contadorCandidato = 0;
    Serial.println(F("Prediccion en vivo. Escribe cualquier comando o 'parar' para detener."));
  }
  else if (!strcmp(cmd, "parar"))      { enVivo = false; Serial.println(F("Detenido.")); }
  else if (!strcmp(cmd, "animar"))     cmdAnimar(a1 ? atoi(a1) : -1);
  else if (!strcmp(cmd, "prueba"))     cmdPrueba(a1 ? atoi(a1) : 20);
  else if (!strcmp(cmd, "k")) {
    int nk = a1 ? atoi(a1) : 0;
    if (nk >= 1 && nk <= MAX_K) { K = nk; Serial.printf("K = %u\n", K); }
    else Serial.printf("K debe estar entre 1 y %u\n", MAX_K);
  }
  else if (!strcmp(cmd, "umbral")) {
    if (a1 && atof(a1) > 0) UMBRAL_DIST = atof(a1);
    Serial.printf("Umbral = %.2f\n", UMBRAL_DIST);
  }
  else if (!strcmp(cmd, "grabar"))     cmdGrabar(a1 ? atoi(a1) : -1, a2 ? atoi(a2) : 20);
  else if (!strcmp(cmd, "datos"))      cmdDatos();
  else if (!strcmp(cmd, "exportar"))   cmdExportar();
  else if (!strcmp(cmd, "borrar"))     { userN = 0; Serial.println(F("Dataset propio borrado.")); }
  else if (!strcmp(cmd, "base")) {
    usarBase = !usarBase;
    Serial.printf("Dataset base %s\n", usarBase ? "ACTIVADO" : "DESACTIVADO");
  }
  else if (!strcmp(cmd, "fuente")) {
    if (a1 && !strcmp(a1, "sensores")) { fuente = SENSORES; filtroIniciado = false; }
    else if (a1 && !strcmp(a1, "virtual")) fuente = VIRTUAL;
    Serial.printf("Fuente: %s\n", fuente == VIRTUAL ? "VIRTUAL" : "SENSORES");
  }
  else if (!strcmp(cmd, "calibrar"))   cmdCalibrar();
  else Serial.println(F("Comando no reconocido. Escribe 'ayuda'."));
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
  analogReadResolution(12);
  randomSeed(esp_random());

  // Calibración por defecto para potenciómetros: 0 = extendido, 4095 = doblado
  for (uint8_t i = 0; i < NUM_FLEX; i++) { rawAbierto[i] = 0; rawCerrado[i] = 4095; }
  prefs.begin("guante", true);
  if (prefs.getBool("calibrado", false)) {
    prefs.getBytes("abierto", rawAbierto, sizeof(rawAbierto));
    prefs.getBytes("cerrado", rawCerrado, sizeof(rawCerrado));
  }
  prefs.end();

  calcularCentroides();

  Serial.println(F("\n*** SIMULADOR DEL GUANTE DE GESTOS (KNN) ***"));
  Serial.println(F("Prueba rapida:  gesto 3   ->   clasificar"));
  cmdDatos();
  mostrarAyuda();
}

void loop() {
  leerSerial();
  static unsigned long ultimo = 0;
  if (enVivo && millis() - ultimo >= 250) {
    ultimo = millis();
    pasoEnVivo();
  }
}
