//
// Copyright (c) 2024 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace visionary {

/**
 * This class is intended to handle nework links for dgram on windows and linux - especially the broadcast case
 */
class NetLink final
{
private:
  static constexpr uint32_t    SOCKET_TIMEOUT_MS = 100;
  static constexpr char const* BROADCAST_IP      = "255.255.255.255";
  struct SocketData;                   // keep socket specific data in compile unit
  std::shared_ptr<SocketData> m_pData; // change to unique_ptr when switching to c++14 or beyond
  bool                        m_init      = false;
  bool                        m_broadcast = false;
  uint32_t                    m_netmask   = 0xFFFFFFFF;

public:
  explicit NetLink(std::string const& localIp,
                   uint8_t            prefix,
                   uint16_t           port,
                   std::string const& remoteIp = BROADCAST_IP);
  ~NetLink(void);

  int write(std::vector<uint8_t> const& buffer);
  int read(std::vector<uint8_t>& buffer);

  uint32_t localAddr(void);
  uint32_t networkMask(void);
  uint32_t remoteAddr(void);

  static uint32_t    string2addr(std::string const& ip);
  static std::string addr2string(uint32_t addr);
};

} // namespace visionary
