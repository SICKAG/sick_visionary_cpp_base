//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#ifndef VISIONARY_VISONARYDATA_H_INCLUDED
#define VISIONARY_VISONARYDATA_H_INCLUDED

#include <cstddef> // for size_t
#include <cstdint>
#include <string>
#include <vector>

#include "PointXYZ.h"

namespace visionary {

/// Camera parameters.
///
/// This struct contains the intrinsic camera parameters, the lens distortion parameters and the transformation matrix
/// from the sensor focus to origin of the configured user coordinate system (from the mounting settings).
struct CameraParameters
{
  /// Height of the frame in pixels.
  int height;
  /// Width of the frame in pixels.
  int width;
  /// Camera to world transformation matrix
  ///
  /// from sensor focus into the coordinate system as configured in the mounting settings.
  /// The matrix is stored in row-major order.
  /// If the mounting settings are all zero, the user coordinate origin is the sensor reference point at the front of
  /// the housing.
  double cam2worldMatrix[4 * 4];
  /// Intrinsic camera Matrix.
  double fx, fy, cx, cy;
  /// Lens distortion parameters.
  double k1, k2, p1, p2, k3;
  /// FocalToRayCross - Correction Offset for depth info
  ///
  /// This value needs to be applied before the cam2world transformation.
  double f2rc;
};

struct DataSetsActive
{
  bool hasDataSetDepthMap;
  bool hasDataSetPolar2D;
  bool hasDataSetCartesian;
};

struct PointXYZC
{
  float x;
  float y;
  float z;
  float c;
};

// Base class for all Visionary data types
//
// This class is used to store the data received from the Visionary sensor.
// It contains the image data and the camera parameters.
//
// Additionally it provides functions to transform the image data to a point cloud.
// And finally it provides functions to parse the received data.
class VisionaryData
{
public:
  VisionaryData();
  virtual ~VisionaryData();

  //-----------------------------------------------
  // Getter Functions

  /// Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  ///
  /// \param[out] pointCloud  - Reference to pass back the point cloud. Will be resized and only contain new point
  /// cloud.
  virtual void generatePointCloud(std::vector<PointXYZ>& pointCloud) = 0;

  /// Transform the XYZ point cloud with the Cam2World matrix got from device
  ///
  /// \param[in,out] pointCloud  - Reference to the point cloud to be transformed. Contains the transformed point cloud
  /// afterwards.
  void transformPointCloud(std::vector<PointXYZ>& pointCloud) const;

  /// Returns the height of the image in pixels.
  int getHeight() const;

  /// Returns the width of the image in pixels.
  int getWidth() const;

  /// Returns the Byte length compared to data types
  std::uint32_t getFrameNum() const;

  /// Returns the timestamp in device format.
  ///
  /// Bits of the devices timestamp:
  ///   - 5 unused
  ///   - 12 Year
  ///   - 4 Month
  ///   - 5 Day
  ///   - 11 Timezone
  ///   - 5 Hour
  ///   - 6 Minute
  ///   - 6 Seconds
  ///   - 10 Milliseconds
  /// .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  ///
  /// \returns the timestamp in date/time format.
  ///
  /// \note To get timestamp in milliseconds call getTimestampMS()
  std::uint64_t getTimestamp() const;

  /// Returns the timestamp in milliseconds (UTC).
  ///
  /// \returns the timestamp in milliseconds (UTC).
  ///
  /// \note To get timestamp in device format call getTimestamp()
  std::uint64_t getTimestampMS() const;

  /// Returns a reference to the camera parameter struct
  ///
  /// \returns a reference to the camera parameter struct.
  const CameraParameters& getCameraParameters() const;

  /// Returns an empty vector. Override in VisionarySData.h
  ///
  /// \returns an empty vector
  virtual const std::vector<std::uint32_t>& getRGBAMap() const;

  /// Returns an empty vector. Override in VisionaryTMiniData.h
  ///
  /// \returns an empty vector
  virtual const std::vector<std::uint16_t>& getIntensityMap() const;

  //-----------------------------------------------
  // functions for parsing received blob

  /// Parse the XML Metadata part
  ///
  /// to get information about the sensor and the following image data.
  ///
  /// \param[in] xmlString      - XML Metadata part of the blob.
  /// \param[in] changeCounter  - Change counter of the XML Metadata part. This counter is incremented on each change of
  ///                             the XML Metadata part.
  ///
  /// \returns true when parsing was successful.
  virtual bool parseXML(const std::string& xmlString, std::uint32_t changeCounter) = 0;

  /// Parse the Binary data part to extract the image data.
  ///
  /// The image data is stored in the derived class.
  ///
  /// \param[in] inputBuffer  - Pointer to the binary data part of the blob.
  /// \param[in] length       - Length of the binary data part.
  ///
  /// \returns true when parsing was successful.
  virtual bool parseBinaryData(std::vector<uint8_t>::iterator inputBuffer, std::size_t length) = 0;

protected:
  // Device specific image types
  enum ImageType
  {
    UNKNOWN,
    PLANAR,
    RADIAL
  };

  /// Returns the size of a data type.
  ///
  /// \param[in] dataType  - String containing the data type.
  ///
  /// \returns the size of the data type in bytes.
  static std::size_t getItemLength(const std::string& dataType);

  /// Pre-calculate lookup table for faster point-cloud conversion.
  ///
  /// This function pre-calculates the lookup table for the lens distortion correction,
  /// which is needed for point cloud calculation.
  ///
  /// \param[in] imgType  - Type of the image (needed for correct transformation)
  ///
  /// \throws std::invalid_argument if the image type is unknown.
  /// \throws std::runtime_error if the image size is invalid.
  void preCalcCamInfo(const ImageType& type);

  /// Calculate and return the Point Cloud in the camera perspective.
  ///
  /// Units are in meters.
  ///
  /// \param[in] map         - Image to be transformed
  /// \param[in] imgType     - Type of the image (needed for correct transformation)
  /// \param[out] pointCloud  - Reference to pass back the point cloud. Will be resized and only contain new point
  void generatePointCloud(const std::vector<std::uint16_t>& map,
                          const ImageType&                  imgType,
                          std::vector<PointXYZ>&            pointCloud);

  //-----------------------------------------------
  /// Camera parameters to be read from XML Metadata part
  CameraParameters m_cameraParams{};

  /// Factor to convert unit of distance image to mm
  float m_scaleZ;

  /// Change counter to detect changes in XML
  std::uint_fast32_t m_changeCounter;

  /// Framenumber of the frame
  ///
  /// Dataset Version 1: incremented on each received image
  /// Dataset Version 2: framenumber received with dataset
  std::uint_fast32_t m_frameNum;

  /// Timestamp in blob format
  ///
  /// To get timestamp in milliseconds call getTimestampMS()
  std::uint64_t m_blobTimestamp;

  /// Image type used for the camera lens correction pre-calculations.
  ImageType m_preCalcCamInfoType;

  // The look-up-tables containing pre-calculations
  std::vector<PointXYZ> m_preCalcCamInfo;

private:
  // Bitmasks to calculate the timestamp in milliseconds
  // Bits of the devices timestamp: 5 unused - 12 Year - 4 Month - 5 Day - 11 Timezone - 5 Hour - 6 Minute - 6 Seconds -
  // 10 Milliseconds
  // .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  static constexpr std::uint64_t BITMASK_YEAR =
    0x7FF800000000000ull; // 0000011111111111100000000000000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_MONTH =
    0x780000000000ull; // 0000000000000000011110000000000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_DAY =
    0x7C000000000ull; // 0000000000000000000001111100000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_HOUR =
    0x7C00000ull; // 0000000000000000000000000000000000000111110000000000000000000000
  static constexpr std::uint64_t BITMASK_MINUTE =
    0x3F0000ull; // 0000000000000000000000000000000000000000001111110000000000000000
  static constexpr std::uint64_t BITMASK_SECOND =
    0xFC00ull; // 0000000000000000000000000000000000000000000000001111110000000000
  static constexpr std::uint64_t BITMASK_MILLISECOND =
    0x3FFull; // 0000000000000000000000000000000000000000000000000000001111111111
};

} // namespace visionary

#endif // VISIONARY_VISONARYDATA_H_INCLUDED
