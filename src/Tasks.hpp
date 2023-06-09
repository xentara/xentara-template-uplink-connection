// Copyright (c) embedded ocean GmbH
#pragma once

#include <xentara/process/Task.hpp>

/// @brief Contains the Xentara tasks of the driver
namespace xentara::plugins::templateUplink::tasks
{

/// @brief A Xentara tasks used to send a transaction
extern const process::Task::Role kSend;

} // namespace xentara::plugins::templateUplink::tasks