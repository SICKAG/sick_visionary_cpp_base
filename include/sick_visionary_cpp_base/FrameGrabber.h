//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#ifndef VISIONARY_FRAMEGRABBER_H_INCLUDED
#define VISIONARY_FRAMEGRABBER_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

#include "FrameGrabberBase.h"
#include "VisionaryDataStream.h"

namespace visionary {

class VisionaryControl; // forward declaration

/**
 * \brief Class which receives frames from the device in background thread and provides the latest one via an interface.

 * This helps avoiding getting old frames because of buffering of data within the network infrastructure.
 * It also handles automatically reconnects in case of connection issues.
 */
template <class DataType>
class FrameGrabber : public FrameGrabberBase
{
public:
  FrameGrabber(VisionaryControl&         visionaryControl,
               const std::string&        hostname,
               std::uint16_t             port,
               std::chrono::milliseconds timeout)
    : FrameGrabberBase(visionaryControl, hostname, port, timeout)
  {
  }

  virtual ~FrameGrabber() = default;

  /// Creates a new data handler.
  /// \returns a shared pointer to the newly created data handler.
  std::shared_ptr<DataType> createDataHandler() const
  {
    return std::dynamic_pointer_cast<DataType>(genCreateDataHandler());
  }

  /// Gets the next blob from the connected device
  /// \param[in, out] pDataHandler an (empty) pointer where the blob will be stored in
  /// \param[in] timeout controls the timeout how long to wait for a new blob, default 1000ms
  ///
  /// \retval true New blob has been received and stored in pDataHandler Pointer
  /// \retval false No new blob has been received
  bool getNextFrame(std::shared_ptr<DataType>& pDataHandler,
                    std::chrono::milliseconds  timeout = std::chrono::milliseconds(1000))
  {
    if (pDataHandler == nullptr)
      pDataHandler = createDataHandler();

    auto pTypedDataHandler = std::move(std::static_pointer_cast<VisionaryData>(pDataHandler));

    const auto retVal = genGetNextFrame(pTypedDataHandler, timeout);

    pDataHandler = std::move(std::static_pointer_cast<DataType>(pTypedDataHandler));

    return retVal;
  }

  /// Gets the current blob from the connected device
  /// \param[in, out] pDataHandler an (empty) pointer where the blob will be stored in
  ///
  /// \retval true a blob was available and has been stored in pDataHandler Pointer
  /// \retval false No blob was available
  bool getCurrentFrame(std::shared_ptr<DataType>& pDataHandler)
  {
    if (pDataHandler == nullptr)
      pDataHandler = createDataHandler();

    auto pTypedDataHandler = std::move(std::dynamic_pointer_cast<VisionaryData>(pDataHandler));

    const auto retVal = genGetCurrentFrame(pTypedDataHandler);

    pDataHandler = std::move(std::dynamic_pointer_cast<DataType>(pTypedDataHandler));

    return retVal;
  }
}; // class FrameGrabber

} // namespace visionary

#endif // VISIONARY_FRAMEGRABBER_H_INCLUDED
