// Copyright (c) embedded ocean GmbH
#include "TemplateTransaction.hpp"

#include "Attributes.hpp"
#include "Events.hpp"
#include "Tasks.hpp"

#include <xentara/config/FallbackHandler.hpp>
#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/model/ForEachAttributeFunction.hpp>
#include <xentara/model/ForEachEventFunction.hpp>
#include <xentara/model/ForEachTaskFunction.hpp>
#include <xentara/process/ExecutionContext.hpp>
#include <xentara/utils/json/decoder/Object.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>
#include <xentara/utils/eh/currentErrorCode.hpp>

#include <concepts>

namespace xentara::plugins::templateUplink
{
	
using namespace std::literals;

auto TemplateTransaction::load(utils::json::decoder::Object &jsonObject,
	config::Resolver &resolver,
	const config::FallbackHandler &fallbackHandler) -> void
{
	// Go through all the members of the JSON object that represents this object
	for (auto && [name, value] : jsonObject)
    {
		/// @todo rename member to something suitable for the specific client
		if (name == "records")
		{
			// Go through all the elements
			for (auto &&element : value.asArray())
			{
				// Add a record
				_records.emplace_front();
				// Load it
				_records.front().load(element, resolver);
			}
		}
		/// @todo load custom configuration parameters
		else if (name == "TODO"sv)
		{
			/// @todo parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			/// @todo check that the value is valid
			if (!"TODO")
			{
				/// @todo use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template transaction"));
			}

			/// @todo set the appropriate member variables
		}
		else
		{
			// Pass any unknown parameters on to the fallback handler, which will load the built-in parameters ("id", "uuid", and "children"),
			// and throw an exception if the key is unknown
            fallbackHandler(name, value);
		}
    }

	/// @todo perform consistency and completeness checks
	if (!"TODO")
	{
		/// @todo use an error message that tells the user exactly what is wrong
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("TODO is wrong with template transaction"));
	}
}

auto TemplateTransaction::collectData(std::chrono::system_clock::time_point timeStamp) -> utils::core::RawDataBlock
{
	// Go through all the records and collect the data
	utils::core::RawDataBlock data;
	for (auto &&record : _records)
	{
		record.collect(timeStamp,  data);
	}

	return data;
}

auto TemplateTransaction::performSendTask(const process::ExecutionContext &context) -> void
{
	// Only perform the read only if the client is connected
	if (!_client.get().connected())
	{
		return;
	}

	// Send the data
	send(context.scheduledTime());
}

auto TemplateTransaction::send(std::chrono::system_clock::time_point timeStamp) -> void
{
	// Collect the data
	auto data = collectData(timeStamp);
	
	try
	{
		/// @todo send the data

		/// @todo if the data function does not throw errors, but uses return types or internal handle state,
		// throw an std::system_error here on failure, or call handleSendError() directly.

		// The write was successful
		updateState(timeStamp);
	}
	catch (const std::exception &)
	{
		// Get the error from the current exception using this special utility function
		const auto error = utils::eh::currentErrorCode();
		// Update the state
		handleSendError(timeStamp, error);
	}
}

auto TemplateTransaction::handleSendError(std::chrono::system_clock::time_point timeStamp, std::error_code error)
	-> void
{
	// Update our own state
	updateState(timeStamp, error);
	// Notify the client
	_client.get().handleError(timeStamp, error, this);
}

auto TemplateTransaction::updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void
{
	// Make a write sentinel
	memory::WriteSentinel sentinel { _stateDataBlock };
	auto &state = *sentinel;

	// Update the state
	state._transactionState = !error;
	state._sendTime = timeStamp;
	state._error = error;

	// Determine the correct event
	const auto &event = error ? _sendErrorEvent : _sentEvent;
	// Commit the data and raise the event
	sentinel.commit(timeStamp, event);
}

auto TemplateTransaction::forEachAttribute(const model::ForEachAttributeFunction &function) const -> bool
{
	/// @todo handle any additional attributes this class supports, including attributes inherited from the client
	return
		function(attributes::kTransactionState) ||
		function(attributes::kSendTime) ||
		function(attributes::kError);
}

auto TemplateTransaction::forEachEvent(const model::ForEachEventFunction &function) -> bool
{
	// Handle all the events we support
	return
		function(events::kSent, sharedFromThis(&_sentEvent)) ||
		function(events::kSendError, sharedFromThis(&_sendErrorEvent));

	/// @todo handle any additional events this class supports
}

auto TemplateTransaction::forEachTask(const model::ForEachTaskFunction &function) -> bool
{
	// Handle all the tasks we support
	return
		function(tasks::kSend, sharedFromThis(&_sendTask));

	/// @todo handle any additional tasks this class supports
}

auto TemplateTransaction::makeReadHandle(const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle>
{
	// Try our attributes
	if (attribute == attributes::kTransactionState)
	{
		return _stateDataBlock.member(&State::_transactionState);
	}
	else if (attribute == attributes::kSendTime)
	{
		return _stateDataBlock.member(&State::_sendTime);
	}
	else if (attribute == attributes::kError)
	{
		return _stateDataBlock.member(&State::_error);
	}

	/// @todo add support for any additional attributes, including attributes inherited from the client

	return std::nullopt;
}

auto TemplateTransaction::realize() -> void
{
	// Create the data block
	_stateDataBlock.create(memory::memoryResources::data());
}

auto TemplateTransaction::prepare() -> void
{
	// Resolve all the handles for the records
	for (auto &&record : _records)
	{
		record.resolveHandles();
	}
}

auto TemplateTransaction::clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void
{
	// We cannot reset the error to Ok because we haven't actually sent a request yet. So we use the appropriate custom error code instead.
	auto effectiveError = error ? error : CustomError::Pending;

	// Simply update the state
	updateState(timeStamp, error);
}

auto TemplateTransaction::SendTask::preparePreOperational(const process::ExecutionContext &context) -> Status
{
	// Request a connection
	_target.get().requestConnect(context.scheduledTime());

	return Status::Completed;
}

auto TemplateTransaction::SendTask::operational(const process::ExecutionContext &context) -> void
{
	_target.get().performSendTask(context);
}

auto TemplateTransaction::SendTask::preparePostOperational(const process::ExecutionContext &context) -> Status
{
	// Execute the task one more time to send any remaining data
	_target.get().performSendTask(context);

	// Request a disconnect
	_target.get().requestDisconnect(context.scheduledTime());

	return Status::Completed;
}

} // namespace xentara::plugins::templateUplink