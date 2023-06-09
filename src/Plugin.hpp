// Copyright (c) embedded ocean GmbH
#pragma once

#include "Skill.hpp"

#include <xentara/plugin/Plugin.hpp>

/// @namespace xentara
/// @brief base namespace for Xentara

/// @namespace xentara::plugins
/// @brief base namespace for Xentara plugins

/// @brief base namespace for the uplink skill
/// @todo rename namespace
namespace xentara::plugins::templateUplink
{

/// @brief The class that registers the library as a Xentara plugin.
class Plugin final : plugin::Plugin
{
public:
	/// @name Virtual Overrides for plugin::Plugin
	/// @{

	auto registerSkills(Registry & registry) -> void final
	{
		// Register the skill class.
		registry << _skillClass;
	}
	
	/// @}

private:
	/// @brief The skill class object
	Skill::Class _skillClass;

	/// @brief The global plugin object
	static Plugin _instance;
};

} // namespace xentara::plugins::templateUplink
