#include "VisionaryType.h"

namespace visionary {

constexpr char VisionaryType::kVisionaryS[];
constexpr char VisionaryType::kVisionaryTMini[];

VisionaryType VisionaryType::fromString(const std::string& typestring)
{
  if (typestring == kVisionaryS)
    return VisionaryType(eVisionaryS);

  else if (typestring == kVisionaryTMini)
    return VisionaryType(eVisionaryTMini);

  else
    throw std::invalid_argument("Unknown Visionary type");
}

std::string VisionaryType::toString() const noexcept
{
  switch (m_type)
  {
    case eVisionaryS:
      return kVisionaryS;

    case eVisionaryTMini:
      return kVisionaryTMini;

    default:
      return "";
  }
}

std::vector<std::string> VisionaryType::getNames() noexcept
{
  return {kVisionaryS, kVisionaryTMini};
}

} // namespace visionary
