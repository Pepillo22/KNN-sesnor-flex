# Gesto Cuernos / Rock

Se agregó como clase **5 (`Cuernos_Rock`)** en el simulador y en el firmware.
La pose esperada, en el orden pulgar, índice, medio, anular, meñique, es:

```text
doblado, extendido, doblado, doblado, extendido
```

En la escala del proyecto (`0.0` extendido, `1.0` doblado), la plantilla es
aproximadamente `[0.88, 0.09, 0.90, 0.90, 0.09]`.

## Archivos

- `dataset_gesto_rock.csv`: 50 muestras iniciales **sintéticas**.
- `muestras_arduino.txt`: las mismas muestras como valores enteros x100 para
  el array `BASE_X`.

Las muestras sintéticas solo sirven como plantilla de prueba; las lecturas
reales pueden variar según el guante y la calibración. En el firmware del
ESP32, calibra primero (`c`), mantén la pose y graba muestras propias con
`g5 50`; guárdalas en flash con `w`. Para el simulador, la pose está disponible
con `gesto 5`.

El clasificador usa los cinco sensores flex. No usa los valores de giroscopio
ni acelerómetro incluidos como columnas de relleno en el CSV.
