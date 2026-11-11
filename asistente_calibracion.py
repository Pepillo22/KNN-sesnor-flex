#!/usr/bin/env python3
"""
Asistente interactivo paso a paso para calibración y mapeo de dedos.
Con pre-chequeo de hardware, prevención de duplicados y detección de cables flojos.
"""
import sys
import time
import subprocess
import serial
import serial.tools.list_ports

PORT = "/dev/cu.usbserial-0001"
BAUD = 115200

DEDOS = [
    ("Pulgar", "👍 Dobla ÚNICAMENTE el dedo PULGAR y mantenlo doblado"),
    ("Índice", "👆 Dobla ÚNICAMENTE el dedo ÍNDICE y mantenlo doblado"),
    ("Medio",  "🖕 Dobla ÚNICAMENTE el dedo MEDIO y mantenlo doblado"),
    ("Anular", "💍 Dobla ÚNICAMENTE el dedo ANULAR y mantenlo doblado"),
    ("Meñique","🤙 Dobla ÚNICAMENTE el dedo MEÑIQUE y mantenlo doblado")
]

def encontrar_puerto():
    ports = [p.device for p in serial.tools.list_ports.comports() if "usbserial" in p.device or "USB" in p.description]
    return ports[0] if ports else PORT

def leer_raw(ser, duracion=1.5):
    t0 = time.time()
    lecturas = {}
    while time.time() - t0 < duracion:
        line = ser.readline().decode("utf-8", errors="replace").strip()
        if "RAW:" in line:
            partes = line.split("RAW:")[1].strip().split()
            for p in partes:
                if "=" in p:
                    try:
                        pin, val = p.split("=")
                        pin = int(pin)
                        val = float(val)
                        if pin not in lecturas:
                            lecturas[pin] = []
                        lecturas[pin].append(val)
                    except:
                        pass
    promedios = {}
    for pin, vals in lecturas.items():
        if vals:
            promedios[pin] = sum(vals) / len(vals)
    return promedios

def main():
    print("=" * 65)
    print("  🧙 ASISTENTE INTELIGENTE DE MAPEO Y CALIBRACIÓN DE DEDOS")
    print("=" * 65)
    
    port = encontrar_puerto()
    try:
        ser = serial.Serial(port, BAUD, timeout=1.0)
    except Exception as e:
        print(f"\n❌ Error al abrir {port}: {e}")
        print("Asegúrate de haber cerrado 'probar_guante.py' o el Monitor Serie de Arduino.\n")
        return

    time.sleep(1.5)
    ser.reset_input_buffer()
    ser.write(b"m\n")
    time.sleep(0.5)

    # --- FASE 0: PRE-CHEQUEO DE HARDWARE ---
    print("\n[1/3] Verificando conexión de los 5 sensores en la placa...")
    time.sleep(0.5)
    chequeo = leer_raw(ser, 1.5)
    if not chequeo:
        print("❌ No se recibieron lecturas del ESP32.")
        ser.write(b"s\n")
        ser.close()
        return

    print("\nEstado actual de los pines analógicos del ESP32:")
    alertas = 0
    for pin in sorted(chequeo.keys()):
        val = chequeo[pin]
        if val < 50:
            estado = "⚠️ EN CERO / CASI CERO (¿cable suelto o pin equivocado?)"
            alertas += 1
        elif val > 4000:
            estado = "⚠️ SATURADO EN 4095 (¿en corto con 3.3V?)"
            alertas += 1
        else:
            estado = "🟢 ACTIVO (recibiendo señal analógica)"
        print(f"  GPIO {pin:2d} -> ADC: {val:6.1f} | {estado}")

    if alertas > 0:
        print("\nOJO: Algunos pines no están recibiendo señal.")
        print("Verifica que los cables de alimentación (3.3V y GND) y las resistencias")
        print("del divisor de tensión hagan buen contacto en la protoboard/guante.")
        continuar = input("\n¿Quieres continuar de todos modos? (s/n): ").strip().lower()
        if continuar != 's':
            ser.write(b"s\n")
            ser.close()
            return

    input("\n👉 Colócate el guante y presiona ENTER para medir línea base...")

    # --- FASE 1: LÍNEA BASE ---
    print("\n[2/3] Mantén la MANO COMPLETAMENTE ABIERTA y quieta:")
    for s in range(3, 0, -1):
        print(f"  {s}...")
        time.sleep(1.0)
    
    base_abierto = leer_raw(ser, 2.0)
    print("Línea base registrada con éxito.")

    # --- FASE 2: DEDO POR DEDO SIN DUPLICADOS ---
    print("\n[3/3] Identificación dedo por dedo (los pines asignados no se repetirán):")
    pines_asignados = {}
    pines_libres = set(base_abierto.keys())

    for nombre, instruccion in DEDOS:
        while True:
            print("\n-------------------------------------------------------------")
            print(f"🎯 DEDO: {nombre.upper()}")
            print(instruccion)
            input("👉 Presiona ENTER cuando tengas el dedo bien doblado...")

            print("  Midiendo variación...")
            muestra = leer_raw(ser, 1.8)
            
            # Calcular delta solo para pines que aún no han sido asignados a otro dedo
            deltas = {}
            for pin in pines_libres:
                if pin in muestra and pin in base_abierto:
                    deltas[pin] = abs(muestra[pin] - base_abierto[pin])
            
            if not deltas:
                print("  ❌ No hay más pines disponibles.")
                break

            mejor_pin = max(deltas, key=deltas.get)
            delta = deltas[mejor_pin]
            
            print(f"\n  Variaciones en pines disponibles:")
            for p in sorted(deltas.keys()):
                print(f"    GPIO {p:2d}: delta = {deltas[p]:.0f}")

            if delta >= 80:
                print(f"  ✅ ¡Asignado! {nombre} -> GPIO {mejor_pin} (cambio: {delta:.0f} puntos)")
                pines_asignados[nombre] = mejor_pin
                pines_libres.remove(mejor_pin)
                break
            else:
                print(f"  ⚠️ Poca variación (máx fue {delta:.0f} en GPIO {mejor_pin}).")
                opc = input("  ¿Reintentar este dedo (r) o forzar este pin (f)?: ").strip().lower()
                if opc == 'f':
                    pines_asignados[nombre] = mejor_pin
                    pines_libres.remove(mejor_pin)
                    break
                else:
                    print("  Reintentando...")

    ser.write(b"s\n")
    ser.close()

    print("\n" + "=" * 65)
    print("🎉 RESUMEN FINAL DEL MAPEO DETECTADO:")
    print("=" * 65)
    orden_nuevo = []
    for nombre, _ in DEDOS:
        pin = pines_asignados.get(nombre, "?")
        orden_nuevo.append(pin)
        print(f"  {nombre:8s} -> GPIO {pin}")

    if all(isinstance(p, int) for p in orden_nuevo) and len(set(orden_nuevo)) == 5:
        print(f"\nArray para Arduino: {{{', '.join(str(p) for p in orden_nuevo)}}}")
        aplicar = input("\n¿Deseas que actualice tu código y lo flashee a tu ESP32 automáticamente? (s/n): ").strip().lower()
        if aplicar == 's':
            ino_path = "/Users/lic.ing.jesusolvera/Documents/GUANTE GESTOS/guante_knn_flex/guante_knn_flex.ino"
            with open(ino_path, 'r') as f:
                c = f.read()
            import re
            c = re.sub(r'const uint8_t FLEX_PINS\[NUM_FLEX\] = \{[^}]+\};',
                       f'const uint8_t FLEX_PINS[NUM_FLEX] = {{{", ".join(str(p) for p in orden_nuevo)}}};', c)
            with open(ino_path, 'w') as f:
                f.write(c)
            with open("/Users/lic.ing.jesusolvera/Documents/GUANTE GESTOS/Proyecto.txt", 'w') as f:
                f.write(c)

            print("Compilando y subiendo por USB...")
            subprocess.run([
                "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli",
                "compile", "--fqbn", "esp32-bluepad32:esp32:esp32",
                "/Users/lic.ing.jesusolvera/Documents/GUANTE GESTOS/guante_knn_flex"
            ], check=True)
            subprocess.run([
                "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli",
                "upload", "-p", port, "--fqbn", "esp32-bluepad32:esp32:esp32",
                "/Users/lic.ing.jesusolvera/Documents/GUANTE GESTOS/guante_knn_flex"
            ], check=True)
            print("\n🚀 ¡LISTO! Firmware actualizado con tu mapeo físico real.")
    else:
        print("\n⚠️ No se pudieron asignar 5 pines únicos diferentes. Revisa los cables marcados en cero.")

if __name__ == "__main__":
    main()
