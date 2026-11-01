# **ESPHome Homematic RF Bridge (HB‑RF‑ETH Port)**

[English version](README_EN.md)

Dieses Projekt portiert die **HB‑RF‑ETH‑Firmware** auf **ESPHome** und erlaubt den Betrieb von Homematic‑Funkmodulen wie **HM‑MOD‑RPI‑PCB** oder **RPI‑RF‑MOD** auf einem ESPHome‑basierten ESP32‑Gateway – eingebunden als External ESPHome Component.

---

## Kompatibilität

Erfolgreich auf einem WT32‑ETH01 getestet und auf dem ESP32‑basierten Zigbee‑Gateway ZB‑GW03 produktiv im Einsatz.
Es funktioniert auch mit der Beispieldatei auf dem [HB-RF-ETH Platine](https://github.com/alexreinert/PCB).

---

## Installation & Nutzung

### 1. External Component einbinden

```yaml
external_components:
  - source: github://zsisamci/esphome_homematic_rf_brigde
```

---

### 2. UART konfigurieren

```yaml
uart:
  id: hm_uart
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200
```
---

### 3. Reset und LED-Outputs konfigurieren

```yaml
output:
  - platform: gpio
    pin: GPIO33
    id: reset_pin
    inverted: True  # HM‑MOD‑RPI‑PCB

  - platform: ledc
    pin: GPIO04
    id: green_led

  - platform: ledc
    pin: GPIO14
    id: blue_led
    
  - platform: ledc
    pin: GPIO32
    id: red_led
```

LEDs nur beim RPI‑RF‑MOD erforderlich.

---

### 4. Bridge aktivieren

```yaml
hm_rf_bridge:
  uart_id: hm_uart
  reset_output: reset_pin
  red_led: red_led
  green_led: green_led
  blue_led: blue_led
  connected:    
    name: "CCU Connected"
  radio_module_type:
    name: HM Radio Module Type
  firmware_version:
    name: "HM Module Firmware Version"
  serial:
    name: "HM Module Serial"
  sgtin:
    name: "HM Module SGTIN"
```

LEDs und Sensoren sind optional.

---

### 5. Optional MDNS konfigurieren

```yaml
mdns:
  services:
    - service: "_raw-uart"     
      protocol: "_udp"
      port: 3008
```

---

## Wokwi-Simulation

Unter **examples/wokwi-sim** liegt ein komplettes Wokwi‑Projekt, mit dem sich die Firmware direkt simulieren lässt. Zusätzlich enthält der Ordner eine Simulation des Homematic‑Funkmoduls, sodass UART‑Kommunikation ohne echte Hardware getestet werden kann.

## Devcontainer

Devcontainer mit ESPHome und wasi-sdk (benötigt für die Wokwi-Simulation).