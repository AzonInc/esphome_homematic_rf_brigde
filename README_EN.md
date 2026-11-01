# **ESPHome Homematic RF Bridge (HB‑RF‑ETH Port)**

This project ports the **HB‑RF‑ETH firmware** to **ESPHome**, enabling the use of Homematic radio modules such as **HM‑MOD‑RPI‑PCB** or **RPI‑RF‑MOD** on an ESPHome-based ESP32 gateway – integrated as an external ESPHome component.

---

## Compatibility

Successfully tested on a WT32‑ETH01 and running in production on the ESP32-based Zigbee gateway ZB‑GW03.
It also works with the example configuration for the [HB-RF-ETH board](https://github.com/alexreinert/PCB).

---

## Installation & Usage

### 1. Add the External Component

```yaml
external_components:
  - source: github://zsisamci/esphome_homematic_rf_brigde
```

---

### 2. Configure UART

```yaml
uart:
  id: hm_uart
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200
```

---

### 3. Configure Reset and LED Outputs

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

LEDs are only required for the RPI‑RF‑MOD.

---

### 4. Enable the Bridge

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

LEDs and sensors are optional.

---

### 5. Optional: Configure MDNS

```yaml
mdns:
  services:
    - service: "_raw-uart"     
      protocol: "_udp"
      port: 3008
```

---

## Wokwi Simulation

Under **examples/wokwi-sim** you will find a complete Wokwi project that allows you to simulate the firmware directly. The folder also includes a simulation of the Homematic radio module, so UART communication can be tested without real hardware.

## Devcontainer

Devcontainer with ESPHome and wasi-sdk (required for the Wokwi simulation).
