//
// Copyright (c) 2024 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "VisionaryAutoIP.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>

// Boost library used for parseXML function
#if defined(__GNUC__)         // GCC compiler
#  pragma GCC diagnostic push // Save warning levels for later restoration
#  pragma GCC diagnostic ignored "-Wpragmas"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wparentheses"
#  pragma GCC diagnostic ignored "-Wcast-align"
#  pragma GCC diagnostic ignored "-Wstrict-overflow"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#if defined(__GNUC__)        // GCC compiler
#  pragma GCC diagnostic pop // Restore previous warning levels
#endif

#include "NetLink.h"
#include "VisionaryEndian.h"

namespace visionary {

namespace pt = boost::property_tree;

const uint8_t kRplScanColaB = 0x90; // replied by sensors; with information like device name, serial number, IP,...
const uint8_t kRplNetscan   = 0x95; // replied by sensors; with information like device name, serial number, IP,...
const uint8_t kRplIpconfig  = 0x91; // replied by sensor; confirmation to IP change

VisionaryAutoIP::VisionaryAutoIP(std::string const& interfaceIp, uint8_t prefix)
  : m_link(interfaceIp, prefix, AUTOIP_PORT)
{
}

std::vector<DeviceInfo> VisionaryAutoIP::scan(void)
{
  uint8_t                         b[4];
  std::unordered_set<std::string> deviceMACs{};

  // Init Random generator
  std::random_device         rd;
  std::default_random_engine mt(rd());
  uint32_t                   teleIdCounter = mt();
  std::vector<DeviceInfo>    deviceList;

  // AutoIP Discover Packet
  ByteBuffer autoIpPacket;
  autoIpPacket.push_back(0x10); // CMD
  autoIpPacket.push_back(0x00); // reserved
  // length of datablock
  autoIpPacket.push_back(0x00);
  autoIpPacket.push_back(0x08);
  // Mac address
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  // curtelegramID
  uint32_t curtelegramID = teleIdCounter++;
  writeUnalignBigEndian(b, sizeof(b), curtelegramID);
  autoIpPacket.insert(autoIpPacket.end(), b, b + 4u);
  // Additional UCP Scan information
  autoIpPacket.push_back(0x01); // Indicates that telegram is a CoLa Scan telegram
  autoIpPacket.push_back(0x00);
  // IpAddreass
  uint32_t ipAddr = m_link.localAddr();
  writeUnalignBigEndian(b, sizeof(b), ipAddr);
  autoIpPacket.insert(autoIpPacket.end(), b, b + 4u);
  // IpAddreass
  uint32_t networkMask = m_link.networkMask();
  writeUnalignBigEndian(b, sizeof(b), networkMask);
  autoIpPacket.insert(autoIpPacket.end(), b, b + 4u);

  // broadcast packet
  m_link.write(autoIpPacket);

  // Check for answers to Discover Packet
  const std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  while (true)
  {
    ByteBuffer                                  receiveBuffer(1500);
    const std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
    if ((now - startTime) > std::chrono::milliseconds(AUTOIP_TIMEOUT))
    {
      if (deviceList.size() == 0)
        std::cerr << __FUNCTION__ << " Timeout" << '\n';
      break;
    }
    if (m_link.read(receiveBuffer) > 16) // 16 bytes minsize
    {
      uint32_t pos = 0;
      if (receiveBuffer[0] == kRplNetscan)
      {
        auto telID = readUnalignBigEndian<uint32_t>(receiveBuffer.data() + 10u);
        if (telID != curtelegramID)
        {
          continue;
        }
        DeviceInfo dI = parseAutoIPBinary(receiveBuffer);
        if (dI.macAddress == "invalid")
          continue;
        auto status = deviceMACs.insert(dI.macAddress);
        if (status.second)
          deviceList.push_back(dI);
        continue;
      }
      if (receiveBuffer[0] == kRplScanColaB)
      {
        pos++;
        pos += 1; // unused byte
        uint16_t payLoadSize = readUnalignBigEndian<uint16_t>(receiveBuffer.data() + pos);
        pos += 2;
        pos += 6; // Skip mac address(part of xml)
        uint32_t recvTelegramID = readUnalignBigEndian<uint32_t>(receiveBuffer.data() + pos);
        pos += 4;
        // check if it is a response to our scan
        if (recvTelegramID != curtelegramID)
        {
          continue;
        }
        pos += 2; // unused
        // Get XML Payload
        if (receiveBuffer.size() >= pos + payLoadSize)
        {
          std::stringstream stringStream(std::string(reinterpret_cast<char*>(&receiveBuffer[pos]), payLoadSize));
          try
          {
            DeviceInfo dI     = parseAutoIPXml(stringStream);
            auto       status = deviceMACs.insert(dI.macAddress);
            if (status.second)
              deviceList.push_back(dI);
          }
          catch (...)
          {
            std::cerr << __FUNCTION__ << "parse xml failed" << std::endl;
          }
        }
        else
        {
          std::cerr << __FUNCTION__ << "Received invalid AutoIP Packet" << '\n';
        }
      }
    }
  }
  return deviceList;
}

DeviceInfo VisionaryAutoIP::parseAutoIPXml(std::stringstream& rStringStream)
{
  DeviceInfo deviceInfo;

  pt::ptree tree;
  pt::read_xml(rStringStream, tree);

  deviceInfo.colaVersion = COLA_1;
  deviceInfo.authVersion = SUL1;
  deviceInfo.macAddress  = tree.get_child("NetScanResult.<xmlattr>.MACAddr").get_value<std::string>();

  for (const auto& it : tree.get_child("NetScanResult"))
  {
    if (it.first != "<xmlattr>")
    {
      const std::string& key   = it.second.get<std::string>("<xmlattr>.key");
      const std::string& value = it.second.get<std::string>("<xmlattr>.value");
      if (key == "IPAddress")
      {
        deviceInfo.ipAddress = value;
      }

      if (key == "IPMask")
      {
        deviceInfo.networkMask = value;
      }

      if (key == "IPGateway")
      {
        deviceInfo.gateway = value;
      }

      if (key == "HostPortNo")
      {
        deviceInfo.colaPort = static_cast<uint16_t>(std::stoi(value));
      }

      if (key == "DeviceType")
      {
        deviceInfo.deviceIdent = value;
      }

      if (key == "SerialNumber")
      {
        deviceInfo.serialNumber = value;
      }

      if (key == "OrderNumber")
      {
        deviceInfo.orderNumber = value;
      }

      if (key == "DHCPClientEnabled")
      {
        deviceInfo.dhcpEnabled = value == "TRUE";
      }

      if (key == "IPConfigDuration")
      {
        deviceInfo.reconfigurationTimeMs = static_cast<uint32_t>(std::stoi(value));
      }
    }
  }

  return deviceInfo;
}

DeviceInfo VisionaryAutoIP::parseAutoIPBinary(const VisionaryAutoIP::ByteBuffer& buffer)
{
  DeviceInfo  deviceInfo;
  std::string macAddr;
  deviceInfo.macAddress  = "invalid";
  deviceInfo.colaVersion = COLA_2;
  deviceInfo.authVersion = SUL1; // default since not explicitly reported

  int    offset  = 16;
  size_t minSize = 18;
  if (buffer.size() < minSize)
    return deviceInfo;
  // auto deviceInfoVersion = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;

  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto cidNameLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += cidNameLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  deviceInfo.deviceIdent = std::string(reinterpret_cast<const char*>(buffer.data() + offset), cidNameLen);
  offset += cidNameLen;

  //  auto cidMajorVersion = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidMinorVersion = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidPatchVersion = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidBuildVersion = readUnalignBigEndian<uint32_t>(buffer.data() + offset);
  offset += 4;
  //  auto cidVersionClassifier = readUnalignBigEndian<uint8_t>(buffer.data() + offset);
  offset += 1;

  //  auto deviceState = readUnalignBigEndian<char>(buffer.data() + offset);
  offset += 1;
  //  auto reqUserAction = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;

  // Device name
  minSize += 16;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto deviceNameLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  // std::string deviceName(reinterpret_cast<const char*>(buffer.data() + offset), deviceNameLen);
  minSize += deviceNameLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  offset += deviceNameLen;

  // App name
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto appNameLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += appNameLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  // std::string appName(reinterpret_cast<const char*>(buffer.data() + offset), appNameLen);
  offset += appNameLen;

  // Project name
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto projNameLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += projNameLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  // std::string projName(reinterpret_cast<const char*>(buffer.data() + offset), projNameLen);
  offset += projNameLen;

  // Serial number
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto serialNumberLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += serialNumberLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  deviceInfo.serialNumber = std::string(reinterpret_cast<const char*>(buffer.data() + offset), serialNumberLen);
  offset += serialNumberLen;

  // Type code
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto typeCodeLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += typeCodeLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  // std::string typeCode(reinterpret_cast<const char*>(buffer.data() + offset), typeCodeLen);
  offset += typeCodeLen;

  // Firmware version
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto firmwareVersionLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += firmwareVersionLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  // std::string firmwareVersion(reinterpret_cast<const char*>(buffer.data() + offset), firmwareVersionLen);
  offset += firmwareVersionLen;

  // Order number
  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto orderNumberLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  minSize += orderNumberLen;
  if (buffer.size() < minSize)
    return deviceInfo;
  deviceInfo.orderNumber = std::string(reinterpret_cast<const char*>(buffer.data() + offset), orderNumberLen);
  offset += orderNumberLen;

  // # unused: flags, = struct.unpack('>B', rpl[offset:offset + 1])
  offset += 1;

  minSize += 3;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto auxArrayLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;

  for (int i = 0; i < auxArrayLen; ++i)
  {
    minSize += 4;
    if (buffer.size() < minSize)
      return deviceInfo;
    std::string key(reinterpret_cast<const char*>(buffer.data() + offset), 4);
    offset += 4;
    minSize += 2;
    if (buffer.size() < minSize)
      return deviceInfo;
    auto innerArrayLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
    offset += 2;
    minSize += innerArrayLen;
    if (buffer.size() < minSize)
      return deviceInfo;
    if (key == "AutV")
    {
      std::string autV(reinterpret_cast<const char*>(buffer.data() + offset), innerArrayLen);
      if (autV == "1.0.0.0R")
        deviceInfo.authVersion = SUL2;
    }
    offset += innerArrayLen;
  }

  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto scanIfLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  for (int i = 0; i < scanIfLen; ++i)
  {
    minSize += 2;
    if (buffer.size() < minSize)
      return deviceInfo;
    // auto ifaceNum = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
    offset += 2;
    minSize += 2;
    if (buffer.size() < minSize)
      return deviceInfo;
    auto ifaceNameLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
    offset += 2;
    minSize += ifaceNameLen;
    if (buffer.size() < minSize)
      return deviceInfo;
    // std::string ifaceName(reinterpret_cast<const char*>(buffer.data() + offset), ifaceNameLen);
    offset += ifaceNameLen;
  }

  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto comSettingsLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  for (int i = 0; i < comSettingsLen; ++i)
  {
    minSize += 4;
    if (buffer.size() < minSize)
      return deviceInfo;
    std::string key(reinterpret_cast<const char*>(buffer.data() + offset), 4);
    offset += 4;
    minSize += 2;
    if (buffer.size() < minSize)
      return deviceInfo;
    auto innerArrayLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
    offset += 2;
    minSize += innerArrayLen;
    if (buffer.size() < minSize)
      return deviceInfo;
    if (key == "EMAC")
    {
      MacAddress macAddress;
      std::memcpy(&macAddress.macAddress, buffer.data() + offset, sizeof(macAddress.macAddress));
      macAddr = convertMacToString(macAddress);
      offset += 6;
      continue;
    }
    if (key == "EIPa")
    {
      uint32_t value = readUnalignBigEndian<uint32_t>(buffer.data() + offset);
      offset += 4;
      deviceInfo.ipAddress = NetLink::addr2string(value);
      continue;
    }
    if (key == "ENMa")
    {
      uint32_t value = readUnalignBigEndian<uint32_t>(buffer.data() + offset);
      offset += 4;
      deviceInfo.networkMask = NetLink::addr2string(value);
      continue;
    }
    if (key == "EDGa")
    {
      uint32_t value = readUnalignBigEndian<uint32_t>(buffer.data() + offset);
      offset += 4;
      deviceInfo.gateway = NetLink::addr2string(value);
      continue;
    }
    if (key == "EDhc")
    {
      deviceInfo.dhcpEnabled = readUnalignBigEndian<uint8_t>(buffer.data() + offset) != 0;
      offset += 1;
      continue;
    }
    if (key == "ECDu")
    {
      auto configTime = readUnalignBigEndian<uint32_t>(buffer.data() + offset);
      offset += 4;
      deviceInfo.reconfigurationTimeMs = configTime * 1000;
      continue;
    }
    offset += innerArrayLen;
  }

  minSize += 2;
  if (buffer.size() < minSize)
    return deviceInfo;
  auto endPointsLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
  offset += 2;
  std::vector<uint16_t> ports;
  for (int i = 0; i < endPointsLen; ++i)
  {
    minSize += 1;
    if (buffer.size() < minSize)
      return deviceInfo;
    // auto colaVersion = readUnalignBigEndian<uint8_t>(buffer.data() + offset);
    offset += 1;
    minSize += 2;
    if (buffer.size() < minSize)
      return deviceInfo;
    auto innerArrayLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
    offset += 2;
    for (int j = 0; j < innerArrayLen; ++j)
    {
      minSize += 4;
      if (buffer.size() < minSize)
        return deviceInfo;
      std::string key(reinterpret_cast<const char*>(buffer.data() + offset), 4u);
      offset += 4;

      minSize += 2;
      if (buffer.size() < minSize)
        return deviceInfo;
      auto mostInnerArrayLen = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
      offset += 2;

      minSize += mostInnerArrayLen;
      if (buffer.size() < minSize)
        return deviceInfo;
      if (key == "DPNo") // PortNumber [UInt]
      {
        auto port = readUnalignBigEndian<uint16_t>(buffer.data() + offset);
        offset += 2;
        ports.push_back(port);
      }
      else
      {
        offset += mostInnerArrayLen;
      }
    }
  }
  deviceInfo.colaPort   = ports[0];
  deviceInfo.macAddress = macAddr;
  return deviceInfo;
}

bool VisionaryAutoIP::assign(const std::string& destinationMac,
                             ColaVersion        colaVer,
                             const std::string& ipAddr,
                             const std::string& ipMask,
                             const std::string& ipGateway,
                             bool               dhcpEnable,
                             uint32_t           reconfigurationTimeMs)
{
  uint8_t b[4];

  // Init Random generator
  std::random_device         rd;
  std::default_random_engine mt(rd());
  unsigned int               teleIdCounter = mt();

  ByteBuffer payload;
  if (colaVer == ColaVersion::COLA_1)
  {
    std::string dhcpString = "FALSE";
    if (dhcpEnable)
    {
      dhcpString = "TRUE";
    }
    std::string request = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<IPconfig MACAddr=\"" + destinationMac + "\">"
      "<Item key=\"IPAddress\" value=\"" + ipAddr + "\" />"
      "<Item key=\"IPMask\" value=\"" + ipMask + "\" />"
      "<Item key=\"IPGateway\" value=\"" + ipGateway + "\" />"
      "<Item key=\"DHCPClientEnabled\" value=\"" + dhcpString +"\" /></IPconfig>";

    payload.insert(payload.end(), request.c_str(), request.c_str() + request.size());
  }
  else // don't allow empty payload send COLA_2 on default
  {
    uint32_t ip = NetLink::string2addr(ipAddr);
    writeUnalignBigEndian<uint32_t>(b, sizeof(b), ip);
    payload.insert(payload.end(), b, b + 4u);

    uint32_t nm = NetLink::string2addr(ipMask);
    writeUnalignBigEndian<uint32_t>(b, sizeof(b), nm);
    payload.insert(payload.end(), b, b + 4u);

    uint32_t gw = NetLink::string2addr(ipGateway);
    writeUnalignBigEndian<uint32_t>(b, sizeof(b), gw);
    payload.insert(payload.end(), b, b + 4u);

    payload.push_back(static_cast<uint8_t>(dhcpEnable));
  }

  // AutoIP Discover Packet
  ByteBuffer autoIpPacket;
  autoIpPacket.push_back(0x11); // CMD ip config
  autoIpPacket.push_back(0x00); // not defined rfu

  // length of datablock
  uint8_t blockLenght[2];
  writeUnalignBigEndian(blockLenght, sizeof(blockLenght), static_cast<uint16_t>(payload.size()));
  autoIpPacket.insert(autoIpPacket.end(), blockLenght, blockLenght + 2u);

  MacAddress mac = convertMacToStruct(destinationMac);
  // Mac address
  autoIpPacket.insert(autoIpPacket.end(), mac.macAddress, mac.macAddress + 6u);

  // telegram id
  uint32_t curtelegramID = teleIdCounter++;
  writeUnalignBigEndian(b, sizeof(b), curtelegramID);
  autoIpPacket.insert(autoIpPacket.end(), b, b + 4u);

  // reserved
  autoIpPacket.push_back(0x01); // Indicates that telegram is a CoLa Scan telegram
  autoIpPacket.push_back(0x00);

  // payload
  autoIpPacket.insert(autoIpPacket.end(), payload.begin(), payload.end());

  m_link.write(autoIpPacket);

  const std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  while (true)
  {
    ByteBuffer                                  receiveBuffer(1500);
    const std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
    if ((now - startTime) > std::chrono::milliseconds(AUTOIP_TIMEOUT))
    {
      std::cerr << __FUNCTION__ << " Timeout" << '\n';
      break;
    }
    if (m_link.read(receiveBuffer) > 16) // 16 bytes minsize
    {
      if (receiveBuffer[0] == kRplIpconfig)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(reconfigurationTimeMs));
        return true;
      }
    }
  }
  return false;
}

VisionaryAutoIP::MacAddress VisionaryAutoIP::convertMacToStruct(const std::string& basicString)
{
  std::vector<uint8_t> ipInts;
  std::istringstream   ss(basicString);
  MacAddress           macAddress{};
  int                  i = 0;
  while (ss)
  {
    std::string token;
    size_t      pos;
    if (!getline(ss, token, ':'))
      break;
    macAddress.macAddress[i] = static_cast<uint8_t>(std::stoi(token, &pos, 16));
    i++;
  }
  return macAddress;
}

std::string VisionaryAutoIP::convertMacToString(const VisionaryAutoIP::MacAddress& macAddress)
{
  std::string macAddrString;
  for (int k = 0; k < 6; ++k)
  {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(macAddress.macAddress[k]);

    macAddrString += ss.str();
    if (k < 5)
    {
      macAddrString += ":";
    }
  }
  return macAddrString;
}

} // namespace visionary
