// Copyright (c) embedded ocean GmbH
#pragma once

#include "CustomError.hpp"

#include <xentara/model/Attribute.hpp>

#include <cstdint>
#include <system_error>

/// @brief Contains the Xentara attributes of the uplink skill
namespace xentara::plugins::templateUplink::attributes
{

/// @brief A Xentara attribute containing the state of a client connection
extern const model::Attribute kConnectionState;
/// @brief A Xentara attribute containing the state of a transaction
extern const model::Attribute kTransactionState;

/// @brief A Xentara attribute containing the connection time for a client
extern const model::Attribute kConnectionTime;
/// @brief A Xentara attribute containing the send time for a transaction
extern const model::Attribute kSendTime;

/// @brief A Xentara attribute containing an error code for a client connection
extern const model::Attribute kError;

/// @namespace xentara::plugins::templateUplink::attributes
/// @todo add any additional attributes supplied by the individual elements

/// @brief The type used for error codes in kError and kWriteError
using ErrorCode = std::uint32_t;

/// @brief Generates an error code for a custom error
auto errorCode(CustomError error) noexcept -> ErrorCode;

/// @namespace xentara::plugins::templateUplink::attributes
/// @todo add more functions for other categories of errors, e.g. error codes from a library or protocol

/// @brief Generates an error code for an std::error_code
auto errorCode(std::error_code error) noexcept -> ErrorCode;

} // namespace xentara::plugins::templateUplink::attributes