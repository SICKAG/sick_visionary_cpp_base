//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "FrameGrabberBase.h"

#include <chrono>
#include <iostream>

#include "VisionaryControl.h"

namespace visionary {
FrameGrabberBase::FrameGrabberBase(VisionaryControl&         visionaryControl,
                                   const std::string&        hostname,
                                   std::uint16_t             port,
                                   std::chrono::milliseconds timeout)
  : m_visionaryControl(visionaryControl)
  , m_isRunning(false)
  , m_hostnameThreadRead(hostname)
  , m_portThreadRead(port)
  , m_timeoutThreadRead(timeout)
  , m_connectedThreadPrivate(false)
  , m_pDataStreamThreadPrivate(nullptr)
  , m_frameAvailableThreadShared(false)
  , m_pDataHandlerThreadShared(nullptr)
{
  m_pDataHandlerThreadShared = genCreateDataHandler();

  m_pDataStreamThreadPrivate = std::unique_ptr<VisionaryDataStream>(new VisionaryDataStream(genCreateDataHandler()));

  m_connectedThreadPrivate =
    m_pDataStreamThreadPrivate->open(m_hostnameThreadRead, m_portThreadRead, m_timeoutThreadRead);

  if (!m_connectedThreadPrivate)
  {
    throw std::runtime_error("Failed to connect");
  }

  m_isRunning     = true;
  m_grabberThread = std::thread(&FrameGrabberBase::run, this);
}

std::shared_ptr<VisionaryData> FrameGrabberBase::genCreateDataHandler() const
{
  return m_visionaryControl.createDataHandler();
}

FrameGrabberBase::~FrameGrabberBase()
{
  m_isRunning = false;
  m_grabberThread.join();
}

void FrameGrabberBase::run()
{
  while (m_isRunning)
  {
    if (!m_connectedThreadPrivate)
    {
      std::cerr << "Connection lost, reconnecting" << '\n';

      m_pDataStreamThreadPrivate->close();

      m_connectedThreadPrivate =
        m_pDataStreamThreadPrivate->open(m_hostnameThreadRead, m_portThreadRead, m_timeoutThreadRead);

      if (!m_connectedThreadPrivate)
      {
        std::cerr << "Failed to connect to " << m_hostnameThreadRead << ':' << m_portThreadRead << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    if (m_connectedThreadPrivate)
    {
      if (m_pDataStreamThreadPrivate->getNextFrame())
      {
        std::unique_lock<std::mutex> guard(m_mutex);

        m_frameAvailableThreadShared = true;
        auto pOldDataHandler         = std::move(m_pDataHandlerThreadShared);
        m_pDataHandlerThreadShared   = std::move(m_pDataStreamThreadPrivate->getDataHandler());
        m_pDataStreamThreadPrivate->setDataHandler(pOldDataHandler);

        m_frameAvailableCv.notify_one();
      }
    }
  }
}

bool FrameGrabberBase::genGetNextFrame(std::shared_ptr<VisionaryData>& pDataHandler,
                                       bool                            onlyNewer,
                                       std::chrono::milliseconds       timeout)
{
  if (!pDataHandler)
  {
    pDataHandler = genCreateDataHandler();
  }

  std::unique_lock<std::mutex> guard(m_mutex);

  if (onlyNewer)
  {
    // we ignore any already available frames
    m_frameAvailableThreadShared = false;
  }

  m_frameAvailableCv.wait_for(guard, timeout, [this] { return m_frameAvailableThreadShared; });

  if (m_frameAvailableThreadShared)
  {
    m_frameAvailableThreadShared = false;

    // exchange handlers
    const auto tmp             = std::move(pDataHandler);
    pDataHandler               = std::move(m_pDataHandlerThreadShared);
    m_pDataHandlerThreadShared = tmp;

    return true;
  }
  return false;
}

bool FrameGrabberBase::genGetCurrentFrame(std::shared_ptr<VisionaryData>& pDataHandler)
{
  if (!pDataHandler)
  {
    pDataHandler = genCreateDataHandler();
  }

  std::unique_lock<std::mutex> guard(m_mutex);

  if (m_frameAvailableThreadShared)
  {
    m_frameAvailableThreadShared = false;

    // exchange handlers
    const auto tmp             = std::move(pDataHandler);
    pDataHandler               = std::move(m_pDataHandlerThreadShared);
    m_pDataHandlerThreadShared = tmp;

    return true;
  }
  return false;
}

} // namespace visionary
