#!/usr/bin/env python3
"""
Script utilitario para convertir datasets CSV a bloques C++ listos para Arduino/ESP32.
Permite ver el array BASE_X y BASE_Y generado de cualquier gesto.
"""
import os
import csv

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CSV_FILE = os.path.join(SCRIPT_DIR, "dataset_gesto_lentes.csv")

def generar_codigo():
    if not os.path.exists(CSV_FILE):
        print(f"Error: no se encontró {CSV_FILE}")
        return

    with open(CSV_FILE, "r") as f:
        rows = [r for r in csv.DictReader([l for l in f if not l.startswith("#")])]

    print(f"// --- BLOQUE C++ PARA 'Ajustar_Lentes' ({len(rows)} muestras) ---")
    print("// Pega esto dentro de BASE_X en tu archivo .ino:\n")
    for i, r in enumerate(rows):
        f0 = int(round(float(r["flex_0"]) * 100))
        f1 = int(round(float(r["flex_1"]) * 100))
        f2 = int(round(float(r["flex_2"]) * 100))
        f3 = int(round(float(r["flex_3"]) * 100))
        f4 = int(round(float(r["flex_4"]) * 100))
        sep = "," if i < len(rows) - 1 else ""
        print(f"  {{ {f0:2d}, {f1:2d}, {f2:2d}, {f3:2d}, {f4:2d} }}{sep}")

if __name__ == "__main__":
    generar_codigo()
