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

#include <algorithm>
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

// Logs a frame as hex in chunks through a stack buffer (no heap allocation)
static inline void hm_log_frame(const char *tag, const char *text, const unsigned char *buffer, size_t len) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
  static constexpr size_t CHUNK_SIZE = 32;
  char hex[esphome::format_hex_pretty_size(CHUNK_SIZE)];
  ESP_LOGD(tag, "%s (%u bytes)", text, (unsigned) len);
  for (size_t pos = 0; pos < len; pos += CHUNK_SIZE) {
    size_t chunk_len = std::min(CHUNK_SIZE, len - pos);
    ESP_LOGD(tag, "%s", esphome::format_hex_pretty_to(hex, buffer + pos, chunk_len, '.'));
  }
#endif
}

#define sem_take(__sem, __timeout) (xSemaphoreTake(__sem, __timeout * 1000 / portTICK_PERIOD_MS) == pdTRUE)
#define sem_give(__sem) xSemaphoreGive(__sem)
#define sem_init(__sem) __sem = xSemaphoreCreateBinary();

#define log_frame(__text, __buffer, __len) hm_log_frame(TAG, __text, __buffer, __len)
// ESP_LOG_BUFFER_HEX_LEVEL(TAG, __buffer, __len, ESP_LOG_DEBUG);
