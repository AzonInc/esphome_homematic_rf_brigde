/*
 *  rawuartudplistener.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include <atomic>
#include <functional>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esphome/components/socket/socket.h"
#include "radiomoduleconnector.h"

namespace esphome::hm_rf_bridge {

class RawUartUdpListener : FrameHandler {
 public:
  static constexpr uint16_t PORT = 3008;

 private:
  static constexpr size_t MAX_PACKET_SIZE = 1500 - 28;  // Ethernet MTU minus IP and UDP header

  RadioModuleConnector *_radioModuleConnector;
  std::atomic<uint> _remoteAddress;
  std::atomic<ushort> _remotePort;
  std::atomic<bool> _connectionStarted;
  std::atomic<int> _counter;
  std::atomic<int> _endpointConnectionIdentifier;
  uint64_t _lastReceivedKeepAlive{0};
  std::unique_ptr<socket::Socket> _socket;
  SemaphoreHandle_t _sendMutex{nullptr};
  TaskHandle_t _tHandle = NULL;
  std::function<void()> _onConnectionChanged;
  // Packet buffers live in the (heap allocated) listener, not on the 4 KB task stacks
  unsigned char _rxBuffer[MAX_PACKET_SIZE];  // UDP task only
  unsigned char _txBuffer[MAX_PACKET_SIZE];  // guarded by _sendMutex

  void handlePacket(unsigned char *data, size_t length, uint32_t addr, uint16_t port);
  void sendMessage(unsigned char command, unsigned char *buffer, size_t len);
  void setConnectionStarted(bool started);

 public:
  RawUartUdpListener(RadioModuleConnector *radioModuleConnector);

  void handleFrame(unsigned char *buffer, uint16_t len);

  bool isConnected();
  // Called from the UDP task whenever isConnected() changes; set before start()
  void setOnConnectionChanged(std::function<void()> callback) { _onConnectionChanged = std::move(callback); }

  bool start();

  void _udpQueueHandler();
};

}  // namespace esphome::hm_rf_bridge
