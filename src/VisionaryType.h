#ifndef VISIONARY_VISIONARYTYPE_H_INCLUDED
#define VISIONARY_VISIONARYTYPE_H_INCLUDED

#include <stdexcept>
#include <string>
#include <vector>

namespace visionary {

/// Visionary product type
///
/// Defines constants that represent the different Visionary products.
class VisionaryType
{
public:
  /// \defgroup Visionary-type-names Visionary product type constants.
  ///
  /// The names only contain the product type, not the version.
  /// Only non-whitespace ASCII characters are used. Spaces in the official product names are replaced by underscores.
  ///
  // @{
  /// Name of the Visionary-S product type.
  constexpr static const char kVisionaryS[] = "Visionary-S";
  /// Name of the Visionary-T Mini product type.
  constexpr static const char kVisionaryTMini[] = "Visionary-T_Mini";
  /// @}

  /// Enumeration of the different Visionary product types.
  enum Enum
  {
    eVisionaryS,    ///< Visionary-S product type.
    eVisionaryTMini ///< Visionary-T Mini product type.
  };

  /// Constructor
  /// \param[in] type     product type of the Visionary sensor.
  ///
  /// \throws std::invalid_argument if the product type is unknown.
  VisionaryType(Enum type) : m_type(type)
  {
    switch (m_type)
    {
      case eVisionaryS:
      case eVisionaryTMini:
        break;

      default:
        throw std::invalid_argument("Unknown Visionary type");
    }
  }

  /// Returns the type enum.
  ///
  /// \returns the type enum.
  Enum asEnum() const noexcept
  {
    return m_type;
  }

  /// Implicit conversion to enum value.
  ///
  /// \returns the type enum.
  operator Enum() const noexcept
  {
    return asEnum();
  }

  /// Create a VisionaryType from the product type string.
  ///
  /// Possible values are found in \ref Visionary-type-names.
  ///
  /// \param[in] typestring string representation of the VisionaryType.
  ///
  /// \throws std::invalid_argument if the product type is unknown.
  static VisionaryType fromString(const std::string& typestring);

  /// Get the string representation of the VisionaryType.
  ///
  /// \returns the string representation of the VisionaryType.
  std::string toString() const noexcept;

  /// Return all possible VisionaryType names.
  ///
  /// \returns all possible VisionaryType names as a vector of strings.
  static std::vector<std::string> getNames() noexcept;

private:
  Enum m_type;
};

} // namespace visionary

#endif // VISIONARY_VISIONARYTYPE_H_INCLUDED
