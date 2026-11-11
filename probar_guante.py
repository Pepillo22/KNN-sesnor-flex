#!/usr/bin/env python3
"""
Panel de prueba y monitor en tiempo real para el Guante de Gestos ESP32.
"""
import sys
import time
import serial
import serial.tools.list_ports

PORT = "/dev/cu.usbserial-0001"
BAUD = 115200

GESTOS_INFO = {
    "Gesto_C": "🤏 Gesto C",
    "Punio_Cerrado": "✊ Puño Cerrado",
    "Mano_Abierta": "✋ Mano Abierta",
    "Gesto_L": "👆 Gesto L",
    "Gesto_OK": "👌 Gesto OK",
    "Ajustar_Lentes": "👓 Ajustar Lentes (Dedo Medio)",
    "Cuernos_Rock": "🤘 Rocksito / Cuernos",
    "Desconocido": "❓ Desconocido"
}

def encontrar_puerto():
    ports = [p.device for p in serial.tools.list_ports.comports() if "usbserial" in p.device or "USB" in p.description]
    if ports:
        return ports[0]
    return PORT

def main():
    port = encontrar_puerto()
    print("=" * 60)
    print("  🧤 PANEL INTERACTIVO - GUANTE DE GESTOS ESP32")
    print("=" * 60)
    print(f"Conectando a {port} a {BAUD} baudios...")
    
    try:
        ser = serial.Serial(port, BAUD, timeout=1.0)
    except Exception as e:
        print(f"❌ Error al abrir el puerto {port}: {e}")
        print("Verifica que el cable USB esté bien conectado y que el Monitor Serie de Arduino IDE esté cerrado.")
        return

    time.sleep(1.0)
    ser.reset_input_buffer()

    print("\n✅ Conexión establecida con el ESP32.")
    print("Menú de opciones:")
    print("  [1] Ver lectura en vivo de los 5 dedos (Monitor)")
    print("  [2] Probar predicción de gestos en tiempo real")
    print("  [3] Iniciar calibración guiada (Mano abierta -> Puño)")
    print("  [4] Ver información del dataset y clases")
    print("  [q] Salir")
    print("=" * 60)

    try:
        while True:
            opcion = input("\nSelecciona una opción (1/2/3/4/q): ").strip().lower()
            if opcion == 'q':
                break
            elif opcion == '1':
                ser.write(b"m\n")
                print("\n📊 Monitor de dedos activo. Presiona Ctrl+C para regresar al menú.\n")
                try:
                    while True:
                        line = ser.readline().decode("utf-8", errors="replace").strip()
                        if line and "pulgar:" in line:
                            print(f"\r  {line}", end="", flush=True)
                except KeyboardInterrupt:
                    ser.write(b"s\n")
                    print("\n\n⏹️ Monitor detenido.")
            elif opcion == '2':
                ser.write(b"p\n")
                print("\n🧠 Modo predicción activo. Haz gestos con el guante (Ctrl+C para salir):\n")
                try:
                    while True:
                        line = ser.readline().decode("utf-8", errors="replace").strip()
                        if line and "->" in line:
                            partes = line.split("->")
                            sensores = partes[0].strip()
                            resto = partes[1].strip()
                            nombre_gesto = resto.split()[0]
                            icono = GESTOS_INFO.get(nombre_gesto, f"✨ {nombre_gesto}")
                            print(f"  {sensores}  ==>  \033[1;32m{icono}\033[0m  ({resto})")
                except KeyboardInterrupt:
                    ser.write(b"s\n")
                    print("\n\n⏹️ Predicción detenida.")
            elif opcion == '3':
                ser.write(b"c\n")
                print("\n🎯 Modo calibración iniciado en el ESP32. Sigue las instrucciones:")
                try:
                    while True:
                        line = ser.readline().decode("utf-8", errors="replace").strip()
                        if line:
                            print(f"  [ESP32] {line}")
                            if "Calibracion guardada" in line:
                                break
                except KeyboardInterrupt:
                    ser.write(b"s\n")
                    print("\nCalibración cancelada.")
            elif opcion == '4':
                ser.write(b"i\n")
                time.sleep(0.3)
                while ser.in_waiting:
                    line = ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        print(f"  {line}")
    finally:
        ser.write(b"s\n")
        ser.close()
        print("\nPuerto serie cerrado.")

if __name__ == "__main__":
    main()
