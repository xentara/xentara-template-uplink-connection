// Copyright (c) embedded ocean GmbH
#pragma once

#include "TemplateClient.hpp"
#include "TemplateTransaction.hpp"

#include <xentara/process/ServiceProvider.hpp>
#include <xentara/utils/core/Uuid.hpp>

#include <string_view>

namespace xentara::plugins::templateUplink
{

using namespace std::literals;

/// @brief The service provider class
///
/// This class registers all the elements the service provider provides, and creates the service provider runtime environment.
class ServiceProvider final : public process::ServiceProvider
{
public:
	/// @name Virtual Overrides for process::ServiceProvider
	/// @{

	auto name() const -> std::string_view final
	{
		/// @todo change class name
		return "TemplateUplink"sv;
	}

	auto uuid() const -> utils::core::Uuid final
	{
		/// @todo assign a unique UUID
		return "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"_uuid;
	}

	auto registerObjects(Registry &registry) -> void final
	{
		/// @todo register any additional element classes
		registry <<
			TemplateClient::Class::instance() <<
			TemplateTransaction::Class::instance();
	}

	auto createEnvironment() -> std::unique_ptr<process::ServiceProvider::Environment> final;
		
	/// @}

private:
	/// @class xentara::plugins::templateUplink::ServiceProvider::Environment
	/// @brief The service provider runtime environment
	class Environment;
};

} // namespace xentara::plugins::templateUplink