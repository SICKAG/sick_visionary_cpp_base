//
// Copyright (c) 2024 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "NetLink.h"

namespace visionary {

enum ColaVersion
{
  COLA_1 = 1,
  COLA_2 = 2
};

enum AuthVersion
{
  SUL1 = 1,
  SUL2 = 2
};

/**
 * This struct carries informations reported from devices responding to the scan broadcast
 */
struct DeviceInfo
{
  ColaVersion colaVersion;

  std::string deviceIdent;
  std::string serialNumber;
  std::string orderNumber;
  AuthVersion authVersion;

  std::string macAddress;
  uint16_t    colaPort;
  std::string ipAddress;
  std::string networkMask;
  std::string gateway;
  bool        dhcpEnabled;
  uint32_t    reconfigurationTimeMs;
};

constexpr uint16_t AUTOIP_PORT{30718};
constexpr uint32_t AUTOIP_TIMEOUT{10000};

/**
 * This class is intended to find devices in the network and set a new IP address
 */
class VisionaryAutoIP
{
private:
  using ByteBuffer = std::vector<uint8_t>;

  struct MacAddress
  {
    uint8_t macAddress[6];
  };

  static DeviceInfo parseAutoIPXml(std::stringstream& rStringStream);
  static DeviceInfo parseAutoIPBinary(const ByteBuffer& receivedBuffer);

  static MacAddress  convertMacToStruct(const std::string& basicString);
  static std::string convertMacToString(const MacAddress& macAddress);

  NetLink m_link;

public:
  /**
   * Constructor for the AutoIP class
   * @param interfaceIp ip address of the interface on which the scan should be performed
   * @param prefix the network prefix length in the CIDR manner
   */
  explicit VisionaryAutoIP(std::string const& interfaceIp, uint8_t prefix);

  /**
   * Runs an AutoIP scan and returns a list of DeviceInfo objects
   * @return a list of DeviceInfo objects
   */
  std::vector<DeviceInfo> scan(void);

  /**
   * Assigns a new ip configuration to a device based on its MAC address
   * @param destinationMac MAC address of the device on to which to assign the new configuration
   * @param colaVer cola version of the device (defines also the AutoIP protocol version)
   * @param ipAddr new ip address of the device
   * @param ipMask new network mask of the device
   * @param ipGateway new gateway of the device
   * @param dhcpEnabled true if dhcp should be enabled
   * @param reconfigurationTimeoutMs timeout in ms to be waited after assign until the device should be reachable again
   * @return
   */
  bool assign(const std::string& destinationMac,
              ColaVersion        colaVer,
              const std::string& ipAddr                = "192.168.1.10",
              const std::string& ipMask                = "255.255.255.0",
              const std::string& ipGateway             = "0.0.0.0",
              bool               dhcpEnabled           = false,
              uint32_t           reconfigurationTimeMs = 5000);
};

} // namespace visionary
