//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#ifndef VISIONARY_VISIONARYCONTROL_H_INCLUDED
#define VISIONARY_VISIONARYCONTROL_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "CoLaCommand.h"
#include "ControlSession.h"
#include "IAuthentication.h"
#include "IProtocolHandler.h"
#include "TcpSocket.h"
#include "VisionaryType.h"

namespace visionary {

class FrameGrabberBase; // forward declaration
class VisionaryData;    // forward declaration

struct DeviceIdent
{
  std::string name;
  std::string version;
};

class VisionaryControl
{
public:
  /// Default session timeout
  static constexpr std::chrono::seconds kSessionTimeout = std::chrono::seconds(5);

  /// Constructor
  ///
  /// \param[in] type     product type of the Visionary sensor.
  ///
  // \throws std::runtime_error if the product type is unknown.
  //
  /// \note The product type is used to determine the protocol type and the blob port.
  VisionaryControl(VisionaryType visionaryType);

  /// Destructor
  ~VisionaryControl()
  {
    // make sure that the used socket is closed/freed under any circumstances!
    close();
  };

  /// Opens a connection to a Visionary sensor
  ///
  /// \param[in] type     protocol type the sensor understands (CoLa-A, CoLa-B or CoLa-2).
  ///                     This information is found in the sensor documentation.
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] sessionTimeout Timeout for Session (only used for Cola2)
  /// \param[in] autoReconnect Auto reconnect when connection was lost
  /// \param[in] connectTimeout Timeout for Connection
  ///
  /// \retval true The connection to the sensor successfully was established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or has a different IP address or name
  ///               - not available using for PCs network settings (different subnet)
  ///               - the protocol type or the port did not match. Please check your sensor documentation.
  bool open(const std::string&        hostname,
            std::chrono::seconds      sessionTimeout = kSessionTimeout,
            bool                      autoReconnect  = true,
            std::chrono::milliseconds connectTimeout = kSessionTimeout);

  /// Close a connection
  ///
  /// Closes the control connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void close();

  /// Login to the device.
  ///
  /// \param[in] userLevel The user level to login as.
  /// \param[in] password   Password for the selected user level.
  ///
  /// \return error code, 0 on success
  bool login(IAuthentication::UserLevel userLevel, const std::string& password);

  /// <summary>Logout from the device.</summary>
  /// <returns>True if logout was successful, false otherwise.</returns>
  bool logout();

  /// <summary>
  /// Get device information by reading the "DeviceIdent" variable from the device.
  /// </summary>
  /// <returns> Full DeviceIdent struct if successful, otherwise a struct with two empty strings.</returns>
  DeviceIdent getDeviceIdent();

  /// <summary>
  /// Get blob port address.
  /// </summary>
  /// <returns>Blob port, typically 2114.</returns>
  std::uint16_t getBlobPort();

  /// <summary>
  /// Start streaming the data by calling the "PLAYSTART" method on the device. Works only when acquisition is stopped.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool startAcquisition();

  /// <summary>
  /// Trigger a single image on the device. Works only when acquisition is stopped.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool stepAcquisition();

  /// <summary>
  /// Stops the data stream. Works always, also when acquisition is already stopped before.
  /// </summary>
  bool stopAcquisition();

  /// <summary>
  /// Tells the device that there is a streaming channel by invoking a method named GetBlobClientConfig.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool getDataStreamConfig();

  /// <summary>Send a <see cref="CoLaBCommand" /> to the device and waits for the result.</summary>
  /// <param name="command">Command to send</param>
  /// <returns>The response.</returns>
  CoLaCommand sendCommand(const CoLaCommand& command);

  /// Creates a VisionaryData object
  ///
  /// that is suitable for the specific visionary type, e.g.
  /// VisionaryDataTMini for VisionaryType::eVisionaryTMini
  ///
  /// \returns a new unique pointer to a VisionaryData object.
  ///
  /// \throws std::runtime_error if the visionary type is unknown.
  std::shared_ptr<VisionaryData> createDataHandler() const;

  /// Creates and returns a new frame grabber instance
  ///
  /// \returns a new unique pointer to a FrameGrabberBase object.
  ///
  /// \throws std::runtime_error if the visionary type is unknown.
  ///
  /// \note Contacts the device to get the configured blob port.
  std::unique_ptr<FrameGrabberBase> createFrameGrabber();

private:
  std::string receiveCoLaResponse();
  CoLaCommand receiveCoLaCommand();

  std::unique_ptr<TcpSocket>        m_pTransport;
  std::unique_ptr<IProtocolHandler> m_pProtocolHandler;
  std::unique_ptr<IAuthentication>  m_pAuthentication;
  std::unique_ptr<ControlSession>   m_pControlSession;

  VisionaryType             m_visionaryType;
  std::string               m_hostname;
  std::uint16_t             m_controlPort;
  std::chrono::seconds      m_sessionTimeout;
  std::chrono::milliseconds m_connectTimeout;
  bool                      m_autoReconnect;
};

} // namespace visionary

#endif // VISIONARY_VISIONARYCONTROL_H_INCLUDED
