//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "VisionaryControl.h"

#include <cassert>
#include <limits> // for int max
#include <stdexcept>

#include "AuthenticationLegacy.h"
#include "AuthenticationSecure.h"
#include "CoLa2ProtocolHandler.h"
#include "CoLaBProtocolHandler.h"
#include "CoLaParameterReader.h"
#include "CoLaParameterWriter.h"
#include "ControlSession.h"
#include "FrameGrabber.h"
#include "TcpSocket.h"
#include "VisionaryEndian.h"
#include "VisionarySData.h"
#include "VisionaryTMiniData.h"

namespace visionary {

constexpr std::chrono::seconds VisionaryControl::kSessionTimeout;

VisionaryControl::VisionaryControl(VisionaryType visionaryType)
  : m_visionaryType(visionaryType), m_controlPort(0), m_autoReconnect(false)
{
}

std::shared_ptr<VisionaryData> VisionaryControl::createDataHandler() const
{
  switch (m_visionaryType)
  {
    case VisionaryType::eVisionaryS:
      return std::shared_ptr<VisionaryData>(new VisionarySData());

    case VisionaryType::eVisionaryTMini:
      return std::shared_ptr<VisionaryData>(new VisionaryTMiniData());
  }
  throw std::runtime_error("Unknown Visionary type");
}

std::unique_ptr<FrameGrabberBase> VisionaryControl::createFrameGrabber()
{
  switch (m_visionaryType)
  {
    case VisionaryType::eVisionaryS:
      return std::unique_ptr<FrameGrabberBase>(
        new FrameGrabberBase(*this, m_hostname, getBlobPort(), m_sessionTimeout));

    case VisionaryType::eVisionaryTMini:
      return std::unique_ptr<FrameGrabberBase>(
        new FrameGrabberBase(*this, m_hostname, getBlobPort(), m_sessionTimeout));
  }
  throw std::runtime_error("Unknown Visionary type");
}

bool VisionaryControl::open(const std::string&        hostname,
                            std::chrono::seconds      sessionTimeout,
                            bool                      autoReconnect,
                            std::chrono::milliseconds connectTimeout)
{
  m_hostname         = hostname;
  m_sessionTimeout   = sessionTimeout;
  m_connectTimeout   = connectTimeout;
  m_autoReconnect    = autoReconnect;
  m_pProtocolHandler = nullptr;
  m_pTransport       = nullptr;

  std::unique_ptr<IProtocolHandler> pProtocolHandler;

  std::unique_ptr<TcpSocket> pTransport(new TcpSocket());

  switch (m_visionaryType)
  {
    case VisionaryType::eVisionaryS:
      pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLaBProtocolHandler(*pTransport));
      m_controlPort    = 2112;
      break;

    case VisionaryType::eVisionaryTMini:
      pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLa2ProtocolHandler(*pTransport));
      m_controlPort    = 2122;
      break;

    default:
      throw std::runtime_error("Unknown Visionary type");
  }

  if (pTransport->connect(hostname, m_controlPort, connectTimeout) != 0)
  {
    return false;
  }

  // at the end we only have one byte for seconds
  if (sessionTimeout > std::chrono::seconds(std::numeric_limits<std::uint8_t>::max()))
  {
    return false;
  }

  if (!pProtocolHandler->openSession(
        static_cast<std::uint8_t>(std::chrono::duration_cast<std::chrono::seconds>(sessionTimeout).count())))
  {
    pTransport->shutdown();
    return false;
  }

  std::unique_ptr<ControlSession> pControlSession;
  pControlSession = std::unique_ptr<ControlSession>(new ControlSession(*pProtocolHandler));

  std::unique_ptr<IAuthentication> pAuthentication;
  switch (m_visionaryType)
  {
    case VisionaryType::eVisionaryS:
    case VisionaryType::eVisionaryTMini:
      pAuthentication = std::unique_ptr<IAuthentication>(new AuthenticationSecure(*this));
      break;
    default:
      throw std::runtime_error("Unknown Visionary type");
  }

  // commit: a set pTransport means an open connection
  m_pTransport       = std::move(pTransport);
  m_pProtocolHandler = std::move(pProtocolHandler);
  m_pControlSession  = std::move(pControlSession);
  m_pAuthentication  = std::move(pAuthentication);

  return true;
}

void VisionaryControl::close()
{
  if (m_pAuthentication)
  {
    (void)m_pAuthentication->logout();
    m_pAuthentication = nullptr;
  }
  if (m_pProtocolHandler)
  {
    m_pProtocolHandler->closeSession();
    m_pProtocolHandler = nullptr;
  }
  if (m_pTransport)
  {
    m_pTransport->shutdown();
    m_pTransport = nullptr;
  }
  if (m_pControlSession)
  {
    m_pControlSession = nullptr;
  }
}

bool VisionaryControl::login(IAuthentication::UserLevel userLevel, const std::string& password)
{
  return m_pAuthentication->login(userLevel, password);
}

bool VisionaryControl::logout()
{
  return m_pAuthentication->logout();
}

DeviceIdent VisionaryControl::getDeviceIdent()
{
    const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "DeviceIdent").build();
    CoLaCommand response = sendCommand(command);
    if (response.getError() == CoLaError::OK)
    {
      CoLaParameterReader reader(response);
      DeviceIdent         deviceIdent;
      deviceIdent.name    = reader.readFlexString();
      deviceIdent.version = reader.readFlexString();
      return deviceIdent;
    }
    else
    {
        return DeviceIdent{"", ""};
    }
}

bool VisionaryControl::startAcquisition()
{
  const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTART").build();

  CoLaCommand response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stepAcquisition()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYNEXT").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stopAcquisition()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTOP").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::getDataStreamConfig()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "GetBlobClientConfig").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

CoLaCommand VisionaryControl::sendCommand(const CoLaCommand& command)
{
  CoLaCommand response =
    (m_pControlSession != nullptr) ? m_pControlSession->send(command) : CoLaCommand(std::vector<uint8_t>());

  if (m_autoReconnect
      && (response.getError() == CoLaError::SESSION_UNKNOWN_ID || response.getError() == CoLaError::NETWORK_ERROR))
  {
    if (m_pTransport)
    {
      m_pTransport->shutdown();
    }
    const bool success = open(m_hostname, m_sessionTimeout, m_autoReconnect, m_connectTimeout);
    if (success)
    {
      response = m_pControlSession->send(command);
    }
  }

  return response;
}

std::uint16_t VisionaryControl::getBlobPort()
{
  const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "BlobTcpPortAPI").build();

  CoLaCommand response = sendCommand(command);
  if (response.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(response).readUInt();
  }
  else
  {
    return 2114u; // default blob port
  }
}

} // namespace visionary
