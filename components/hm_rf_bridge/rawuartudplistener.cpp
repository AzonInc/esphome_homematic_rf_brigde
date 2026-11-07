/*
 *  rawuartudplistener.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "rawuartudplistener.h"
#include "hmframe.h"
#include <string.h>
#include <cerrno>
#include <esp_timer.h>
#include "esphome/core/log.h"

namespace esphome::hm_rf_bridge {

static const char *const TAG = "hm_rf_bridge.raw_uart";

static void udp_task(void *parameter) { ((RawUartUdpListener *) parameter)->_udpQueueHandler(); }

RawUartUdpListener::RawUartUdpListener(RadioModuleConnector *radioModuleConnector)
    : _radioModuleConnector(radioModuleConnector) {
  atomic_init(&_connectionStarted, false);
  atomic_init(&_remotePort, (ushort) 0);
  atomic_init(&_remoteAddress, 0u);
  atomic_init(&_counter, 0);
  atomic_init(&_endpointConnectionIdentifier, 1);
}

void RawUartUdpListener::handlePacket(unsigned char *data, size_t length, uint32_t addr, uint16_t port) {
  unsigned char response_buffer[3];

  if (length < 4) {
    ESP_LOGW(TAG, "Received invalid raw-uart packet, length %d", length);
    return;
  }

  if (data[0] != 0 && (addr != atomic_load(&_remoteAddress) || port != atomic_load(&_remotePort))) {
    ESP_LOGW(TAG, "Received raw-uart packet from invalid address.");
    return;
  }

  if (*((uint16_t *) (data + length - 2)) != htons(HMFrame::crc(data, length - 2))) {
    ESP_LOGW(TAG, "Received raw-uart packet with invalid crc.");
    return;
  }

  _lastReceivedKeepAlive = esp_timer_get_time();

  switch (data[0]) {
    case 0:                               // connect
      if (length == 5 && data[2] == 1) {  // protocol version 1
        atomic_fetch_add(&_endpointConnectionIdentifier, 2);
        atomic_store(&_remotePort, (ushort) 0);
        setConnectionStarted(false);
        atomic_store(&_remoteAddress, addr);
        atomic_store(&_remotePort, port);
        _radioModuleConnector->setLED(true, true, false);
        response_buffer[0] = 1;
        response_buffer[1] = data[1];
        sendMessage(0, response_buffer, 2);
      } else if (length == 6 && data[2] == 2) {
        int endpointConnectionIdentifier = atomic_load(&_endpointConnectionIdentifier);

        if (data[3] == 0) {
          endpointConnectionIdentifier += 2;
          atomic_store(&_endpointConnectionIdentifier, endpointConnectionIdentifier);
          setConnectionStarted(false);
        } else if (data[3] != (endpointConnectionIdentifier & 0xff)) {
          // Unknown identifier (we rebooted, or the client missed our previous answer): start a new
          // connection. Rejecting it would lock the client out for good, because it keeps retrying with
          // the same identifier from a fresh port and every retry refreshes the keep alive timeout.
          ESP_LOGD(TAG, "Reconnect with unknown endpoint identifier %d (expected %d), starting new connection", data[3],
                   endpointConnectionIdentifier & 0xff);
          endpointConnectionIdentifier += 2;
          atomic_store(&_endpointConnectionIdentifier, endpointConnectionIdentifier);
          setConnectionStarted(false);
        }

        atomic_store(&_remotePort, (ushort) 0);
        atomic_store(&_remoteAddress, addr);
        atomic_store(&_remotePort, port);
        _radioModuleConnector->setLED(true, true, false);
        response_buffer[0] = 2;
        response_buffer[1] = data[1];
        response_buffer[2] = endpointConnectionIdentifier;
        sendMessage(0, response_buffer, 3);
      } else {
        ESP_LOGW(TAG, "Received invalid raw-uart connect packet, length %d", length);
        return;
      }
      break;

    case 1:  // disconnect
      atomic_store(&_remotePort, (ushort) 0);
      setConnectionStarted(false);
      atomic_store(&_remoteAddress, 0u);
      _radioModuleConnector->setLED(false, false, false);
      break;

    case 2:  // keep alive
      break;

    case 3:  // LED
      if (length != 5) {
        ESP_LOGW(TAG, "Received invalid raw-uart LED packet, length %d", length);
        return;
      }

      _radioModuleConnector->setLED(data[2] & 1, data[2] & 2, data[2] & 4);
      break;

    case 4:  // Reset
      if (length != 4) {
        ESP_LOGW(TAG, "Received invalid raw-uart reset packet, length %d", length);
        return;
      }

      _radioModuleConnector->resetModule();
      break;

    case 5:  // Start connection
      if (length != 4) {
        ESP_LOGW(TAG, "Received invalid raw-uart startconn packet, length %d", length);
        return;
      }

      setConnectionStarted(true);
      ESP_LOGI(TAG, "Connection started");

      break;

    case 6:  // End connection
      if (length != 4) {
        ESP_LOGW(TAG, "Received invalid raw-uart endconn packet, length %d", length);
        return;
      }

      setConnectionStarted(false);
      break;

    case 7:  // Frame
      if (length < 5) {
        ESP_LOGW(TAG, "Received invalid raw-uart frame packet, length %d", length);
        return;
      }

      _radioModuleConnector->sendFrame(&data[2], length - 4);
      break;

    default:
      ESP_LOGW(TAG, "Received invalid raw-uart packet with unknown type %d", data[0]);
      break;
  }
}

bool RawUartUdpListener::isConnected() { return atomic_load(&_connectionStarted); }

void RawUartUdpListener::setConnectionStarted(bool started) {
  if (_connectionStarted.exchange(started) != started && _onConnectionChanged)
    _onConnectionChanged();
}

void RawUartUdpListener::sendMessage(unsigned char command, unsigned char *buffer, size_t len) {
  uint16_t port = atomic_load(&_remotePort);
  uint32_t address = atomic_load(&_remoteAddress);

  if (!port)
    return;

  struct sockaddr_in to {};
  to.sin_family = AF_INET;
  to.sin_addr.s_addr = address;
  to.sin_port = htons(port);

  // Called from the UART task (frames) and the UDP task (responses, keep alives); the mutex also
  // guards _txBuffer, which is a member because the task stacks are too small for it
  xSemaphoreTake(_sendMutex, portMAX_DELAY);
  _txBuffer[0] = command;
  _txBuffer[1] = (unsigned char) atomic_fetch_add(&_counter, 1);

  if (len)
    memcpy(_txBuffer + 2, buffer, len);

  *((uint16_t *) (_txBuffer + len + 2)) = htons(HMFrame::crc(_txBuffer, len + 2));

  _socket->sendto(_txBuffer, len + 4, 0, reinterpret_cast<struct sockaddr *>(&to), sizeof(to));
  xSemaphoreGive(_sendMutex);
}

void RawUartUdpListener::handleFrame(unsigned char *buffer, uint16_t len) {
  if (!atomic_load(&_connectionStarted))
    return;

  if (len > MAX_PACKET_SIZE - 4) {
    ESP_LOGW(TAG, "Received oversized frame from radio module, length %d", len);
    return;
  }

  sendMessage(7, buffer, len);
}

bool RawUartUdpListener::start() {
  _socket = socket::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (_socket == nullptr) {
    ESP_LOGE(TAG, "Could not create socket");
    return false;
  }

  // The receive timeout drives the keep alive handling in the UDP task
  struct timeval timeout {
    .tv_sec = 0, .tv_usec = 100000
  };
  _socket->setsockopt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in bind_addr {};
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind_addr.sin_port = htons(PORT);
  if (_socket->bind(reinterpret_cast<struct sockaddr *>(&bind_addr), sizeof(bind_addr)) != 0) {
    ESP_LOGE(TAG, "Could not bind to port %u: errno %d", PORT, errno);
    _socket = nullptr;
    return false;
  }

  _sendMutex = xSemaphoreCreateMutex();
  _radioModuleConnector->setFrameHandler(this, false);
  xTaskCreate(udp_task, "hm_rf_bridge_udp", 4096, this, 15, &_tHandle);
  return true;
}

void RawUartUdpListener::_udpQueueHandler() {
  int64_t nextKeepAliveSentOut = esp_timer_get_time();

  for (;;) {
    struct sockaddr_in from {};
    socklen_t from_len = sizeof(from);
    ssize_t len =
        _socket->recvfrom(_rxBuffer, sizeof(_rxBuffer), reinterpret_cast<struct sockaddr *>(&from), &from_len);
    if (len > 0 && from.sin_family == AF_INET) {
      handlePacket(_rxBuffer, len, from.sin_addr.s_addr, ntohs(from.sin_port));
    }

    if (atomic_load(&_remotePort) != 0) {
      int64_t now = esp_timer_get_time();

      if (now > _lastReceivedKeepAlive + 5000000) {  // 5 sec
        atomic_store(&_remotePort, (ushort) 0);
        atomic_store(&_remoteAddress, 0u);
        setConnectionStarted(false);
        _radioModuleConnector->setLED(true, false, false);
        ESP_LOGW(TAG, "Connection timed out");
      }

      if (now > nextKeepAliveSentOut) {
        nextKeepAliveSentOut = now + 1000000;  // 1sec
        sendMessage(2, NULL, 0);
      }
    }
  }
}

/*
Index 0 - Type: 0-Connect, 1-Disconnect, 2-KeepAlive, 3-LED, 4-StartConn, 5-StopConn, 6-Reset, 7-Frame
Index 1 - Counter
Index 2..n-2 - Payload
Index n-2,n-1 - CRC16

Payload:
  Keepalive: Empty
  Connect: 1 Byte: Protocol version
  LED: 1 Byte: Bit 0 R, Bit 1 G, Bit 2 B
  Reset: Empty
  Frame: Frame-Data
*/

}  // namespace esphome::hm_rf_bridge
