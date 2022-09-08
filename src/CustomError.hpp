// Copyright (c) embedded ocean GmbH
#pragma once

#include <system_error>
#include <type_traits>

namespace xentara::plugins::templateUplink
{

/// @brief Custom error codes used by the driver
/// @todo Add error codes for other custom errors
enum class CustomError
{
	/// @brief The base value for custom errors, so they don't overlap with system errors
	NoError = 0,

	/// @brief The device connection is not open.
	/// @todo change this to NotOpen for devices that are not remotely connected
	NotConnected,

	/// @brief A transaction has not been sent yet.
	Pending,

	/// @brief An unknown error occurred
	UnknownError = 999
};

/// @brief The error category for custom errors
auto customErrorCategory() noexcept -> const std::error_category &;

/// @brief Support automatic conversion from @ref CustomError to std::error_code
inline auto make_error_code(CustomError error) noexcept -> std::error_code
{
	return { int(error), customErrorCategory() };
}

} // namespace xentara::plugins::templateUplink

/// @brief Template specialization for use with the standard C++ library
namespace std
{

/// @brief This specialization enables automatic conversion from xentara::plugins::templateUplink::CustomError to std::error_code.
template<>
struct is_error_code_enum<xentara::plugins::templateUplink::CustomError> : public std::true_type
{
};

}