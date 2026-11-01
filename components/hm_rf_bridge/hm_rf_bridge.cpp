#include "hm_rf_bridge.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include <cstdio>
#include <cstring>
#include "esphome/components/uart/uart_component_esp_idf.h"
#include "radiomoduledetector.h"

namespace esphome::hm_rf_bridge {

static const char *const TAG = "hm_rf_bridge";

void HmRFBridge::setup() {
  // The schema only accepts the native ESP-IDF UART, see __init__.py
  auto *uart_component = static_cast<uart::IDFUARTComponent *>(this->parent_);
  this->radio_module_connector_ =
      new RadioModuleConnector(this->reset_, static_cast<uart_port_t>(uart_component->get_hw_serial_number()), 256);
  this->radio_module_connector_->addLed(this->red_, this->green_, this->blue_);
  this->radio_module_connector_->start();

  RadioModuleDetector detector;
  detector.detectRadioModule(this->radio_module_connector_);

  switch (detector.getRadioModuleType()) {
    case RADIO_MODULE_NONE:
      this->radio_module_type_ = "No Radio Module";
      break;
    case RADIO_MODULE_HM_MOD_RPI_PCB:
      this->radio_module_type_ = "HM-MOD-RPI-PCB";
      break;
    case RADIO_MODULE_RPI_RF_MOD:
      this->radio_module_type_ = "RPI-RF-MOD";
      break;
    default:
      this->radio_module_type_ = "unknown radio module";
      break;
  }
  if (this->radio_module_type_sensor_ != nullptr)
    this->radio_module_type_sensor_->publish_state(this->radio_module_type_);

  if (detector.getRadioModuleType() == RADIO_MODULE_NONE) {
    this->radio_module_connector_->stop();
    delete this->radio_module_connector_;
    this->radio_module_connector_ = nullptr;
    this->mark_failed(LOG_STR("Radio module could not be detected"));
    return;
  }

  const uint8_t *version = detector.getFirmwareVersion();
  snprintf(this->firmware_version_, sizeof(this->firmware_version_), "%u.%u.%u", version[0], version[1], version[2]);
  strncpy(this->serial_, detector.getSerial(), sizeof(this->serial_) - 1);
  strncpy(this->sgtin_, detector.getSGTIN(), sizeof(this->sgtin_) - 1);

  ESP_LOGI(TAG, "Detected %s, firmware %s, serial %s, SGTIN %s", this->radio_module_type_, this->firmware_version_,
           this->serial_, this->sgtin_);

  if (this->firmware_version_sensor_ != nullptr)
    this->firmware_version_sensor_->publish_state(this->firmware_version_);
  if (this->serial_sensor_ != nullptr)
    this->serial_sensor_->publish_state(this->serial_);
  if (this->sgtin_sensor_ != nullptr)
    this->sgtin_sensor_->publish_state(this->sgtin_);

  this->raw_uart_udp_listener_ = new RawUartUdpListener(this->radio_module_connector_);
  this->raw_uart_udp_listener_->start();

  this->disable_loop();
}

void HmRFBridge::update() {
  if (this->connected_sensor_ == nullptr || this->raw_uart_udp_listener_ == nullptr)
    return;

  bool connected = this->raw_uart_udp_listener_->isConnected();
  if (!this->connected_sensor_->has_state() || connected != this->connected_sensor_->state)
    this->connected_sensor_->publish_state(connected);
}

void HmRFBridge::dump_config() {
  auto *uart_component = static_cast<uart::IDFUARTComponent *>(this->parent_);
  ESP_LOGCONFIG(TAG,
                "HomeMatic RF Bridge:\n"
                "  UART number: %u\n"
                "  Radio module: %s\n"
                "  Firmware version: %s\n"
                "  Serial: %s\n"
                "  SGTIN: %s\n"
                "  Red LED: %s\n"
                "  Green LED: %s\n"
                "  Blue LED: %s",
                uart_component->get_hw_serial_number(), this->radio_module_type_, this->firmware_version_, this->serial_,
                this->sgtin_, YESNO(this->red_ != nullptr), YESNO(this->green_ != nullptr),
                YESNO(this->blue_ != nullptr));
  LOG_UPDATE_INTERVAL(this);
  LOG_BINARY_SENSOR("  ", "Connected", this->connected_sensor_);
  LOG_TEXT_SENSOR("  ", "Radio Module Type", this->radio_module_type_sensor_);
  LOG_TEXT_SENSOR("  ", "Firmware Version", this->firmware_version_sensor_);
  LOG_TEXT_SENSOR("  ", "Serial", this->serial_sensor_);
  LOG_TEXT_SENSOR("  ", "SGTIN", this->sgtin_sensor_);
}

}  // namespace esphome::hm_rf_bridge

#endif
