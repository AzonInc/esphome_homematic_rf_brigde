/*
 *  radiomoduleconnector_utils.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *
 *  Copyright 2021 Alexander Reinert
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include <cstdio>
#include <string>
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

// Own implementation, esphome::format_hex_pretty was removed in ESPHome 2026.11
static inline std::string hm_format_hex_pretty(const unsigned char *data, size_t length) {
  std::string ret;
  ret.reserve(length * 3 + 8);
  char buf[4];
  for (size_t i = 0; i < length; i++) {
    snprintf(buf, sizeof(buf), i == 0 ? "%02X" : ".%02X", data[i]);
    ret += buf;
  }
  ret += " (" + std::to_string(length) + ")";
  return ret;
}

#define sem_take(__sem, __timeout) (xSemaphoreTake(__sem, __timeout * 1000 / portTICK_PERIOD_MS) == pdTRUE)
#define sem_give(__sem) xSemaphoreGive(__sem)
#define sem_init(__sem) __sem = xSemaphoreCreateBinary();

#define log_frame(__text, __buffer, __len) \
  ESP_LOGD(TAG, __text); \
  ESP_LOGD(TAG, "%s", (hm_format_hex_pretty(__buffer, __len)).c_str());
// ESP_LOG_BUFFER_HEX_LEVEL(TAG, __buffer, __len, ESP_LOG_DEBUG);
