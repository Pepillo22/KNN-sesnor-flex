# Integración del Gesto "Ajustar Lentes" (Dedo Medio)

Este paquete contiene todo lo necesario para integrar el gesto de **Ajustar Lentes** (dedo medio extendido empujando los lentes, los otros 4 dedos doblados en puño) en el firmware del ESP32 ([guante_knn_flex.ino](../guante_knn_flex.ino)) y en el simulador ([simulador_guante_knn.ino](../simulador_guante_knn.ino)).

---

## 1. Contenido de esta carpeta

1. **`dataset_gesto_lentes.csv`**: Dataset de 50 muestras normalizadas con la misma estructura y formato de columnas que los demás datasets del proyecto (5 sensores flex + 3 ejes giroscopio + 3 ejes acelerómetro).
2. **`muestras_arduino.txt`**: Las 50 muestras formateadas en C++ (valores `x100` enteros) listas para ser copiadas directamente dentro del array `BASE_X` en Arduino.
3. **`actualizar_firmware.py`**: Script automatizado en Python para compilar los CSVs que quieras y generar el bloque de código C++ automáticamente.

---

## 2. Anatomía del Gesto

| Dedo | Estado Físico | Valor Normalizado (0.0 a 1.0) | Valor Entero (x100) |
|:---|:---|:---:|:---:|
| **Pulgar** | Doblado | ~0.88 | `88` |
| **Índice** | Doblado | ~0.88 | `88` |
| **Medio**  | **Extendido (punta en el lente)** | **~0.09** | **`9`** |
| **Anular** | Doblado | ~0.89 | `89` |
| **Meñique**| Doblado | ~0.88 | `88` |

---

## 3. Pasos para Integrarlo en `guante_knn_flex.ino`

Si deseas agregarlo como una **6.ª clase** (conservando las 5 actuales):

### Paso A: Actualizar el número de clases y los nombres
Busca alrededor de la línea 45 de `guante_knn_flex.ino`:
```cpp
// ANTES:
const uint8_t NUM_CLASES = 5;
const char* NOMBRE_GESTO[NUM_CLASES] = {
  "Gesto_C", "Punio_Cerrado", "Mano_Abierta", "Gesto_L", "Gesto_OK"
};

// MODIFICAR A:
const uint8_t NUM_CLASES = 6;
const char* NOMBRE_GESTO[NUM_CLASES] = {
  "Gesto_C", "Punio_Cerrado", "Mano_Abierta", "Gesto_L", "Gesto_OK", "Ajustar_Lentes"
};
```

### Paso B: Actualizar el tamaño del dataset base
Busca la constante `BASE_N` (alrededor de la línea 63):
```cpp
// ANTES (5 gestos x 50 muestras):
const uint16_t BASE_N = 250;

// MODIFICAR A (6 gestos x 50 muestras):
const uint16_t BASE_N = 300;
```

### Paso C: Agregar las 50 filas a `BASE_X`
Al final del array `BASE_X[BASE_N][NUM_FLEX] = { ... };`, coloca una coma `,` después de la muestra 249 y pega las 50 líneas que vienen dentro del archivo `muestras_arduino.txt`.

### Paso D: Agregar las etiquetas de clase a `BASE_Y`
Al final del array `BASE_Y[BASE_N] = { ... };`, agrega 50 veces el número `5`:
```cpp
// Al final de BASE_Y, agregar:
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
```

---

## 4. Verificación y Prueba

1. Compila y sube el firmware a tu ESP32.
2. Abre el Monitor Serie a **115200 baudios** con **Nueva Línea**.
3. Envía `c` para calibrar si no lo has hecho.
4. Envía `p` para predicción en vivo.
5. Extiende el dedo medio con los otros cuatro dedos cerrados (pose de acomodar los lentes): en pantalla aparecerá:
   ```
   [0.88 0.88 0.09 0.89 0.88]  ->  Ajustar_Lentes conf 100%  dist 0.024
   ```
