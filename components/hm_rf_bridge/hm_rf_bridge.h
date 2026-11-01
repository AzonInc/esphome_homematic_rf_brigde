#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_ESP32
#include "radiomoduleconnector.h"
#include "rawuartudplistener.h"

namespace esphome::hm_rf_bridge {

class HmRFBridge : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::ETHERNET; }

  void set_reset_output(output::BinaryOutput *reset) { this->reset_ = reset; }
  void set_red_led(output::BinaryOutput *red) { this->red_ = red; }
  void set_green_led(output::BinaryOutput *green) { this->green_ = green; }
  void set_blue_led(output::BinaryOutput *blue) { this->blue_ = blue; }

  void set_connected_sensor(binary_sensor::BinarySensor *sensor) { this->connected_sensor_ = sensor; }
  void set_radio_module_type_sensor(text_sensor::TextSensor *sensor) { this->radio_module_type_sensor_ = sensor; }
  void set_firmware_version_sensor(text_sensor::TextSensor *sensor) { this->firmware_version_sensor_ = sensor; }
  void set_serial_sensor(text_sensor::TextSensor *sensor) { this->serial_sensor_ = sensor; }
  void set_sgtin_sensor(text_sensor::TextSensor *sensor) { this->sgtin_sensor_ = sensor; }

 protected:
  RadioModuleConnector *radio_module_connector_{nullptr};
  RawUartUdpListener *raw_uart_udp_listener_{nullptr};

  output::BinaryOutput *reset_{nullptr};
  output::BinaryOutput *red_{nullptr};
  output::BinaryOutput *green_{nullptr};
  output::BinaryOutput *blue_{nullptr};

  binary_sensor::BinarySensor *connected_sensor_{nullptr};
  text_sensor::TextSensor *radio_module_type_sensor_{nullptr};
  text_sensor::TextSensor *firmware_version_sensor_{nullptr};
  text_sensor::TextSensor *serial_sensor_{nullptr};
  text_sensor::TextSensor *sgtin_sensor_{nullptr};

  // Detection results, kept for dump_config()
  const char *radio_module_type_{"unknown"};
  char firmware_version_[12]{};  // "255.255.255"
  char serial_[11]{};
  char sgtin_[25]{};
};

}  // namespace esphome::hm_rf_bridge

#endif
