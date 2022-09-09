// Copyright (c) embedded ocean GmbH
#pragma once

#include "TemplateClient.hpp"
#include "TemplateRecord.hpp"
#include "CustomError.hpp"
#include "Attributes.hpp"

#include <xentara/memory/Array.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/plugin/EnableSharedFromThis.hpp>
#include <xentara/process/Event.hpp>
#include <xentara/process/Microservice.hpp>
#include <xentara/process/MicroserviceClass.hpp>
#include <xentara/process/Task.hpp>
#include <xentara/utils/core/RawDataBlock.hpp>
#include <xentara/utils/core/Uuid.hpp>

#include <functional>
#include <string_view>
#include <forward_list>

namespace xentara::plugins::templateUplink
{

using namespace std::literals;

/// @brief A class representing a data transaction for writing data to a client.
/// @todo rename this class to something more descriptive
class TemplateTransaction final : public process::Microservice, public TemplateClient::ErrorSink, public plugin::EnableSharedFromThis<TemplateTransaction>
{
private:
	/// @brief A structure used to store the class specific attributes within an element's configuration
	struct Config final
	{
		/// @todo Add custom config attributes
	};
	
public:
	/// @brief The class object containing meta-information about this element type
	class Class final : public process::MicroserviceClass
	{
	public:
		/// @brief Gets the global object
		static auto instance() -> Class&
		{
			return _instance;
		}

	    /// @brief Returns the array handle for the class specific attributes within an element's configuration
	    auto configHandle() const -> const auto &
        {
            return _configHandle;
        }

		/// @name Virtual Overrides for process::MicroserviceClass
		/// @{

		auto name() const -> std::u16string_view final
		{
			/// @todo change class name
			return u"TemplateTransaction"sv;
		}
	
		auto uuid() const -> utils::core::Uuid final
		{
			/// @todo assign a unique UUID
			return "cccccccc-cccc-cccc-cccc-cccccccccccc"_uuid;
		}

		/// @}

	private:
	    /// @brief The array handle for the class specific attributes within an element's configuration
		memory::Array::ObjectHandle<Config> _configHandle { config().appendObject<Config>() };

		/// @brief The global object that represents the class
		static Class _instance;
	};

	/// @brief This constructor attaches the output to its client
	TemplateTransaction(std::reference_wrapper<TemplateClient> client) :
		_client(client)
	{
		client.get().addErrorSink(*this);
	}

	/// @name Virtual Overrides for process::Microservice
	/// @{

	auto resolveAttribute(std::u16string_view name) -> const model::Attribute * final;
	
	auto resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task> final;

	auto resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event> final;

	auto readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle final;

	auto realize() -> void final;

	auto prepare() -> void final;

	/// @}

	/// @name Virtual Overrides for TemplateClient::ErrorSink
	/// @{

	auto clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void final;
	
	/// @}

protected:
	/// @name Virtual Overrides for process::Microservice
	/// @{

	auto loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void final;
	
	/// @}

private:
	/// @brief This structure represents the current state of the transaction
	struct State final
	{
		/// @brief Whether the transaction is being transmitted correctly
		bool _transactionState { false };
		/// @brief The last time the records were sent (successfully or not)
		std::chrono::system_clock::time_point _sendTime { std::chrono::system_clock::time_point::min() };
		/// @brief The error code when sending the records, or 0 for none.
		attributes::ErrorCode _error { attributes::errorCode(CustomError::NotConnected) };
	};

	/// @brief This class providing callbacks for the Xentara scheduler for the "send" task
	class SendTask final : public process::Task
	{
	public:
		/// @brief This constuctor attached the task to its target
		SendTask(std::reference_wrapper<TemplateTransaction> target) : _target(target)
		{
		}

		/// @name Virtual Overrides for process::Task
		/// @{

		auto stages() const -> Stages final
		{
			return Stage::PreOperational | Stage::Operational | Stage::PostOperational;
		}

		auto preparePreOperational(const process::ExecutionContext &context) -> Status final;

		auto operational(const process::ExecutionContext &context) -> void final;

		auto preparePostOperational(const process::ExecutionContext &context) -> Status final;

		/// @}

	private:
		/// @brief A reference to the target element
		std::reference_wrapper<TemplateTransaction> _target;
	};

	/// @brief This function is forwarded to the client.
	auto requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
	{
		_client.get().requestConnect(timeStamp);
	}

	/// @brief This function is forwarded to the client.
	auto requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
	{
		_client.get().requestDisconnect(timeStamp);
	}

	/// @brief This function is called by the "send" task.
	///
	/// This function attempts to send the collected records if the client is up.
	auto performSendTask(const process::ExecutionContext &context) -> void;
	/// @brief Attempts to write send the collected records to the client and updates the state accordingly.
	auto send(std::chrono::system_clock::time_point timeStamp) -> void;	
	/// @brief Handles a send error
	auto handleSendError(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void;

	/// @brief Collects the data for all the records
	/// @todo use a more appropriate type for holding the data
	auto collectData(std::chrono::system_clock::time_point timeStamp) -> utils::core::RawDataBlock;

	/// @brief Updates the state
	auto updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error = std::error_code()) -> void;

	/// @brief The client this transaction belongs to
	std::reference_wrapper<TemplateClient> _client;
	/// @brief The records to be collected
	std::forward_list<TemplateRecord> _records;

	/// @brief A Xentara event that is fired when the records were successfully sent to the client
	process::Event _sentEvent;
	/// @brief A Xentara event that is fired when a send error occurred
	process::Event _sendErrorEvent;

	/// @brief The "send" task
	SendTask _sendTask { *this };

	/// @brief The data block that contains the state
	memory::ObjectBlock<memory::memoryResources::Data, State> _stateDataBlock;
};

} // namespace xentara::plugins::templateUplink