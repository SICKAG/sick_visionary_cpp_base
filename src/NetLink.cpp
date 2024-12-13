//
// Copyright (c) 2024 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "NetLink.h"

#include <iostream>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <ifaddrs.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
typedef int SOCKET;
#  define INVALID_SOCKET (SOCKET(-1))
#  define SOCKET_ERROR (-1)
#endif

namespace visionary {

struct NetLink::SocketData
{
#ifdef _WIN32
  ::WSADATA wsaData = {};
#else
  ::SOCKET rxsock = INVALID_SOCKET;
#endif
  ::SOCKET      socket     = INVALID_SOCKET;
  ::sockaddr_in localAddr  = {};
  ::sockaddr_in remoteAddr = {};
};

NetLink::NetLink(std::string const& localIp, uint8_t prefix, uint16_t port, std::string const& remoteIp)
  : m_pData(std::make_shared<SocketData>())
{
  // despite doing DGRAM always bind to a local address endpoint
  // to ensure traffic goes through the desired physical interface
  m_pData->localAddr.sin_family                              = AF_INET;
  *reinterpret_cast<uint32_t*>(&m_pData->localAddr.sin_addr) = htonl(string2addr(localIp));
  m_pData->localAddr.sin_port                                = ::htons(port);

  m_pData->remoteAddr.sin_family                              = AF_INET;
  *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = htonl(string2addr(remoteIp));
  m_pData->remoteAddr.sin_port                                = htons(port);

  if (prefix > 32)
  {
    std::cerr << "ERR: invalid prefix" << '\n';
    return;
  }
  int shift = 32 - prefix;
  m_netmask <<= shift / 2; // split shift because operand is modulo 32
  m_netmask <<= shift - (shift / 2);
  m_netmask = htonl(m_netmask);

  // simplify our life - specifying INADDR_BROADCAST as remote will actually
  // broadcast twice limited and direct subnet broadcast to catch all cases
  uint32_t subnetBroadcastAddr = *reinterpret_cast<uint32_t*>(&m_pData->localAddr.sin_addr) | ~m_netmask;
  if ((*reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) == INADDR_BROADCAST)
      || (*reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) == subnetBroadcastAddr))
  {
    *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = subnetBroadcastAddr;
    m_broadcast                                                 = true;
  }

  if ((*reinterpret_cast<uint32_t*>(&m_pData->localAddr.sin_addr) & m_netmask)
      != (*reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) & m_netmask))
  {
    std::cerr << "WARN: remote in different network" << '\n';
  }

#ifdef _WIN32
  m_init = ::WSAStartup(MAKEWORD(2, 2), &m_pData->wsaData) == NO_ERROR;
  if (!m_init)
  {
    std::cerr << "ERR: WSAStartup failed" << '\n';
  }
#else
  if (m_broadcast)
  {
    m_pData->rxsock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_pData->rxsock == INVALID_SOCKET)
    {
      std::cerr << "ERR: open rxsock failed" << '\n';
      return;
    }
  }
#endif
  m_pData->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (m_pData->socket == INVALID_SOCKET)
  {
    std::cerr << "ERR: open socket failed" << '\n';
    return;
  }

  int option{};
#ifdef _WIN32
  option = SOCKET_TIMEOUT_MS;
  if (setsockopt(m_pData->socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&option), sizeof(option)) < 0)
    std::cerr << "WARN: set SO_RCVTIMEO failed" << '\n';

  if (!m_broadcast)
  {
    option = 4 * 1024 * 1024; // 4MB buffer internal to network stack
    if (setsockopt(m_pData->socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&option), sizeof(option)) < 0)
      std::cerr << "WARN: set SO_RCVBUF failed" << '\n';
  }

#else
  timeval timeout{};
  timeout.tv_sec  = SOCKET_TIMEOUT_MS / 1000;
  timeout.tv_usec = (SOCKET_TIMEOUT_MS % 1000) * 1000;
  if (setsockopt(m_pData->socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0)
    std::cerr << "WARN: set SO_RCVTIMEO failed" << '\n';
  if (m_broadcast)
  {
    if (setsockopt(m_pData->rxsock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0)
      std::cerr << "WARN: set SO_RCVTIMEO failed" << '\n';
  }

  if (!m_broadcast)
  {
    option = 4 * 1024 * 1024; // 4MB buffer internal to network stack
    if (setsockopt(m_pData->socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&option), sizeof(option)) < 0)
      std::cerr << "WARN: set SO_RCVBUF failed" << '\n';
  }

#endif
  if (m_broadcast)
  {
    option = 1;
    if (setsockopt(m_pData->socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&option), sizeof(option)) < 0)
      std::cerr << "WARN: set SO_BROADCAST failed" << '\n';
#ifndef _WIN32
    if (setsockopt(m_pData->rxsock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&option), sizeof(option)) < 0)
      std::cerr << "WARN: set SO_BROADCAST failed" << '\n';
#endif
  }

  if (bind(m_pData->socket, reinterpret_cast<sockaddr*>(&m_pData->localAddr), sizeof(m_pData->localAddr)) < 0)
    std::cerr << "WARN: set bind failed" << '\n';
#ifndef _WIN32
  if (m_broadcast)
  {
    uint32_t backup = *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr);
    *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = INADDR_BROADCAST;
    if (bind(m_pData->rxsock, reinterpret_cast<sockaddr*>(&m_pData->remoteAddr), sizeof(m_pData->remoteAddr)) < 0)
      std::cerr << "WARN: set bind failed" << '\n';
    *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = backup;
  }
#endif
}

NetLink::~NetLink(void)
{
#ifdef _WIN32
  if (m_pData->socket != INVALID_SOCKET)
    ::closesocket(m_pData->socket);
  if (m_init)
    ::WSACleanup();
#else
  if (m_pData->socket != INVALID_SOCKET)
    ::close(m_pData->socket);
  if (m_pData->rxsock != INVALID_SOCKET)
    ::close(m_pData->rxsock);
#endif
}

uint32_t NetLink::string2addr(std::string const& ip)
{
  uint32_t addr{};
  if (::inet_pton(AF_INET, ip.c_str(), &addr) <= 0)
  {
    std::cerr << "ERR: invalid remote ip" << '\n';
    return 0;
  }
  return ntohl(addr);
}

std::string NetLink::addr2string(uint32_t addr)
{
  addr          = htonl(addr);
  char buffer[] = "255.255.255.255";
  if (!::inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)))
  {
    std::cerr << "ERR: addr2string failed";
    return "";
  }
  return std::string(buffer);
}

uint32_t NetLink::localAddr(void)
{
  return ntohl(*reinterpret_cast<uint32_t*>(&m_pData->localAddr.sin_addr));
}

uint32_t NetLink::networkMask(void)
{
  return ntohl(m_netmask);
}

uint32_t NetLink::remoteAddr(void)
{
  return ntohl(*reinterpret_cast<uint32_t*>(&m_pData->localAddr.sin_addr));
}

int NetLink::write(std::vector<uint8_t> const& buffer)
{
  if (m_pData->socket == INVALID_SOCKET)
    return false;
  int len = 0;
  if (m_broadcast)
  {
    uint32_t backup = *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr);
    *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = INADDR_BROADCAST;
    len                                                         = sendto(m_pData->socket,
                 reinterpret_cast<char const*>(buffer.data()),
                 static_cast<unsigned short>(
                   buffer.size()), // mtu = 9000 for jumbo frames should perfectly fit into unsigned short
                 0,
                 reinterpret_cast<sockaddr*>(&m_pData->remoteAddr),
                 sizeof(m_pData->remoteAddr));
    *reinterpret_cast<uint32_t*>(&m_pData->remoteAddr.sin_addr) = backup;
  }
  len = sendto(
    m_pData->socket,
    reinterpret_cast<char const*>(buffer.data()),
    static_cast<unsigned short>(buffer.size()), // mtu = 9000 for jumbo frames should perfectly fit into unsigned short
    0,
    reinterpret_cast<sockaddr*>(&m_pData->remoteAddr),
    sizeof(m_pData->remoteAddr));
  return len;
}

int NetLink::read(std::vector<uint8_t>& buffer)
{
  if (m_pData->socket == INVALID_SOCKET)
    return -1;
  int len = recv(m_pData->socket,
                 reinterpret_cast<char*>(buffer.data()),
                 static_cast<unsigned short>(buffer.capacity()),
                 0); // mtu = 9000 for jumbo frames should perfectly fit into unsigned short
#ifndef _WIN32
  if (len <= 0)
  {
    if (m_pData->rxsock == INVALID_SOCKET)
      return -1;
    len = recv(m_pData->rxsock,
               reinterpret_cast<char*>(buffer.data()),
               static_cast<unsigned short>(buffer.capacity()),
               0); // mtu = 9000 for jumbo frames should perfectly fit into unsigned short
  }
#endif
  if (len > 0)
    buffer.resize(static_cast<size_t>(len));
  return len;
}

} // namespace visionary
