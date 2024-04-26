//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#ifndef VISIONARY_FRAMEGRABBERBASE_H_INCLUDED
#define VISIONARY_FRAMEGRABBERBASE_H_INCLUDED

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "VisionaryDataStream.h"

namespace visionary {

class VisionaryControl; // forward declaration

class FrameGrabberBase
{
public:
  /// Constructor
  ///
  /// \param[in] visionaryControl Reference to the VisionaryControl object.
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] port port of the Visionary sensor.
  /// \param[in] timeout timeout for Connection
  ///
  /// \throws std::runtime_error if the visionary type in VisionaryControl is unknown.
  /// \throws std::runtime_error if the connection could not be established.
  FrameGrabberBase(VisionaryControl&         visionaryControl,
                   const std::string&        hostname,
                   std::uint16_t             port,
                   std::chrono::milliseconds timeout);

  /// Destructor
  ///
  /// Stops the grabber thread and waits for it to finish.
  ~FrameGrabberBase();

  /// Creates a new data handler.
  ///
  ///
  /// \returns a shared pointer to the newly created data handler.
  std::shared_ptr<VisionaryData> genCreateDataHandler() const;

  /// Returns the next frame.
  ///
  /// \param[out] pDataHandler Pointer to the data handler that will be filled with the next frame.
  /// \param[in] timeout Timeout for receiving the next frame.
  /// \param[in] onlyNewer If true, drops any frame already captured by the data stream and wait for a new one.
  bool genGetNextFrame(std::shared_ptr<VisionaryData>& pDataHandler,
                       bool                            onlyNewer = false,
                       std::chrono::milliseconds       timeout   = std::chrono::milliseconds(1000));

  /// Returns the current frame.
  ///
  /// Checks whether a new frame is available and returns it. This method does not wait for a new frame.
  ///
  /// \param[out] pDataHandler Pointer to the data handler that will be filled with the next frame.
  bool genGetCurrentFrame(std::shared_ptr<VisionaryData>& pDataHandler);

private:
  /// Thread function that runs the grabber loop.
  void run();

  const VisionaryControl& m_visionaryControl;
  std::atomic<bool>       m_isRunning;

  /// variables that are observed by the receive thread.
  const std::string               m_hostnameThreadRead;
  const std::uint16_t             m_portThreadRead;
  const std::chrono::milliseconds m_timeoutThreadRead;

  /// variables that must not be changed concurrent to the receive thread.
  bool                                 m_connectedThreadPrivate;
  std::unique_ptr<VisionaryDataStream> m_pDataStreamThreadPrivate;

  /// variables that are written by the receive thread and need to be synchronized by the included mutex.
  bool                           m_frameAvailableThreadShared;
  std::shared_ptr<VisionaryData> m_pDataHandlerThreadShared;
  std::mutex                     m_mutex;

  std::thread m_grabberThread;

  /// communicates when m_threadShared.frameAvailable is changed by the thread.
  std::condition_variable m_frameAvailableCv;
};

} // namespace visionary

#endif // VISIONARY_FRAMEGRABBERBASE_H_INCLUDED
