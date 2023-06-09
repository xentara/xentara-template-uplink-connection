// Copyright (c) embedded ocean GmbH
#pragma once

#include <xentara/process/Event.hpp>

/// @brief Contains the Xentara events of the driver
namespace xentara::plugins::templateUplink::events
{

/// @brief A Xentara event that is fired when the records were successfully sent to the client
extern const process::Event::Role kSent;
/// @brief A Xentara event that is fired when a send error occurred
extern const process::Event::Role kSendError;

} // namespace xentara::plugins::templateUplink::events