#include <CoLaError.h>

namespace visionary {

const char* decodeError(CoLaError::Enum errorCode)
{
  switch (errorCode)
  {
    case CoLaError::NETWORK_ERROR:
      return "network error.";
    case CoLaError::METHOD_IN_ACCESS_DENIED:
      return "access to method not allowed.";
    case CoLaError::METHOD_IN_UNKNOWN_INDEX:
      return "unknown method.";
    case CoLaError::VARIABLE_UNKNOWN_INDEX:
      return "unknown variable.";
    case CoLaError::LOCAL_CONDITION_FAILED:
      return "preconditions violated.";
    case CoLaError::INVALID_DATA:
      return "invalid data given for variable.";
    case CoLaError::UNKNOWN_ERROR:
      return "unknown error reason.";
    case CoLaError::BUFFER_OVERFLOW:
      return "data too long.";
    case CoLaError::BUFFER_UNDERFLOW:
      return "premature end of data.";
    case CoLaError::ERROR_UNKNOWN_TYPE:
      return "unknown type.";
    case CoLaError::VARIABLE_WRITE_ACCESS_DENIED:
      return "write-access denied.";
    case CoLaError::UNKNOWN_CMD_FOR_NAMESERVER:
      return "unknown command.";
    case CoLaError::UNKNOWN_COLA_COMMAND:
      return "The CoLa protocol specification does not define the given command, command is unknown.";
    case CoLaError::METHOD_IN_SERVER_BUSY:
      return "previous method call is still in progress.";
    case CoLaError::FLEX_OUT_OF_BOUNDS:
      return "array too long.";
    case CoLaError::EVENT_REG_UNKNOWN_INDEX:
      return "unknown event.";
    case CoLaError::COLA_VALUE_UNDERFLOW:
      return "value too large.";
    case CoLaError::COLA_A_INVALID_CHARACTER:
      return "invalid CoLa-A character.";
    case CoLaError::OSAI_NO_MESSAGE:
      return "OS out of message ressources.";
    case CoLaError::OSAI_NO_ANSWER_MESSAGE:
      return "OS out of answer message ressources on variable write.";
    case CoLaError::INTERNAL:
      return "Internal firmware error/method or variable declared but not defined.";
    case CoLaError::HUB_ADDRESS_CORRUPTED:
      return "Sopas Hubaddress length corrupt.";
    case CoLaError::HUB_ADDRESS_DECODING:
      return "Sopas Hubaddress syntax error.";
    case CoLaError::HUB_ADDRESS_ADDRESS_EXCEEDED:
      return "Too many hubs in the address.";
    case CoLaError::HUB_ADDRESS_BLANK_EXPECTED:
      return "malformed HUB address.";
    case CoLaError::ASYNC_METHODS_ARE_SUPPRESSED:
      return "internal error, no asynchronous methods supported.";
    case CoLaError::COMPLEX_ARRAYS_NOT_SUPPORTED:
      return "internal error, no complex array supported.";
    case CoLaError::SESSION_NO_RESOURCES:
      return "ressource error, out of sessions.";
    case CoLaError::SESSION_UNKNOWN_ID:
      return "invalid session id.";
    case CoLaError::CANNOT_CONNECT:
      return "failed to connect (probably to a Hub device).";
    case CoLaError::INVALID_PORT:
      return "CoLa2 routing error.";
    case CoLaError::SCAN_ALREADY_ACTIVE:
      return "UDP Scan is already running";
    case CoLaError::OUT_OF_TIMERS:
      return "ressource error, out of timers.";
    case CoLaError::WRITE_MODE_NOT_ENABLED:
      return "Writing not possible, device is in RUN mode";
    case CoLaError::SET_PORT_FAILED:
      return "internal scan error, cannot set port.";
    case CoLaError::IO_LINK_FUNC_TEMP_NOT_AVAILABLE:
      return "IoLink error: function temporarily not available";
    case CoLaError::UNKNOWN:
      return "Unknown Sopas Scan error";
    default:
      return "Unknown error";
  }
}

} // namespace visionary
