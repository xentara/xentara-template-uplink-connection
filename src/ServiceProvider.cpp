// Copyright (c) embedded ocean GmbH
#include "ServiceProvider.hpp"

#include <xentara/plugin/SharedFactory.hpp>
#include <xentara/process/Microservice.hpp>

namespace xentara::plugins::templateUplink
{

class ServiceProvider::Environment : public process::ServiceProvider::Environment
{
public:
	/// @class xentara::plugins::templateUplink::ServiceProvider::Environment
	/// @todo add constructor to perform global initialization, if necessary

	/// @class xentara::plugins::templateUplink::ServiceProvider::Environment
	/// @todo add destructor to perform global cleanup, if necessary

	/// @name Virtual Overrides for process::ServiceProvider::Environment
	/// @{

	auto createMicroservice(const process::MicroserviceClass &componentClass, plugin::SharedFactory<process::Microservice> &factory)
		-> std::shared_ptr<process::Microservice> final;

	/// @}
};

auto ServiceProvider::Environment::createMicroservice(const process::MicroserviceClass &componentClass,
	plugin::SharedFactory<process::Microservice> &factory)
	-> std::shared_ptr<process::Microservice>
{
	if (&componentClass == &TemplateClient::Class::instance())
	{
		return factory.makeShared<TemplateClient>();
	}

	/// @todo handle any additional top-level microservice classes

	return nullptr;
}

auto ServiceProvider::createEnvironment() -> std::unique_ptr<process::ServiceProvider::Environment>
{
	return std::make_unique<Environment>();
}

} // namespace xentara::plugins::templateUplink