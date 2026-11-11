#!/usr/bin/env python3
"""
Diagnóstico de Pines y Sensores Flex para el Guante ESP32.
Muestra en tiempo real qué dedo responde cuando mueves la mano.
"""
import sys
import time
import serial

PORT = "/dev/cu.usbserial-0001"
BAUD = 115200

def barra(val):
    val = max(0.0, min(1.0, val))
    llenos = int(round(val * 20))
    return "[" + "#" * llenos + "." * (20 - llenos) + f"] {val:.2f}"

def main():
    print("=" * 65)
    print("  🧤 DIAGNÓSTICO EN TIEMPO REAL DE SENSORES Y PINES")
    print("=" * 65)
    print(f"Conectando a {PORT}...")
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
    except Exception as e:
        print(f"Error: {e}")
        return

    time.sleep(1.5)
    ser.reset_input_buffer()
    ser.write(b"m\n")

    print("\nInstrucción:")
    print("  Mueve UN SOLO DEDO a la vez para identificar qué sensor responde:")
    print("  Pulgar -> Índice -> Medio -> Anular -> Meñique\n")
    print("-" * 65)

    try:
        while True:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if "pulgar:" in line:
                # Format: pulgar:0.10 indice:0.20 medio:0.80 anular:0.90 menique:0.85
                parts = line.split()
                vals = {}
                for p in parts:
                    if ":" in p:
                        k, v = p.split(":", 1)
                        try:
                            # handle possible raw format
                            vals[k] = float(v.split("(")[0])
                        except:
                            pass
                
                if len(vals) >= 5:
                    out = (
                        f"\rPUL:{barra(vals.get('pulgar',0))} | "
                        f"IND:{barra(vals.get('indice',0))} | "
                        f"MED:{barra(vals.get('medio',0))} | "
                        f"ANU:{barra(vals.get('anular',0))} | "
                        f"MEN:{barra(vals.get('menique',0))}"
                    )
                    sys.stdout.write(f"\r{' ' * 100}\r{out}")
                    sys.stdout.flush()
    except KeyboardInterrupt:
        ser.write(b"s\n")
        ser.close()
        print("\n\nDiagnóstico detenido.")

if __name__ == "__main__":
    main()
