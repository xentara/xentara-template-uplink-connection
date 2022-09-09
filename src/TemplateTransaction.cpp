// Copyright (c) embedded ocean GmbH
#include "TemplateTransaction.hpp"

#include "Attributes.hpp"

#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/process/ExecutionContext.hpp>
#include <xentara/utils/json/decoder/Object.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>
#include <xentara/utils/eh/currentErrorCode.hpp>

#include <concepts>

namespace xentara::plugins::templateUplink
{
	
using namespace std::literals;

TemplateTransaction::Class TemplateTransaction::Class::_instance;

auto TemplateTransaction::loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void
{
	// Get a reference that allows us to modify our own config attributes
    auto &&configAttributes = initializer[Class::instance().configHandle()];

	// Go through all the members of the JSON object that represents this object
	for (auto && [name, value] : jsonObject)
    {
		/// @todo rename member to something suitable for the specific client
		if (name == u8"records")
		{
			// Go through all the elements
			for (auto &&element : value.asArray())
			{
				// Add a record
				_records.emplace_front();
				// Load it
				_records.front().loadConfig(element, resolver);
			}
		}
		/// @todo load custom configuration parameters
		else if (name == u8"TODO"sv)
		{
			/// @todo parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			/// @todo check that the value is valid
			if (!"TODO")
			{
				/// @todo use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template transaction"));
			}

			/// @todo set the appropriate member variables, and update configAttributes accordingly (if necessary) 
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

		// Fire the event
		_sentEvent.fire();
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
		
	// Send the event
	_sendErrorEvent.fire();
}

auto TemplateTransaction::updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void
{
	// Make a write sentinel
	memory::WriteSentinel sentinel { _stateDataBlock };
	auto &state = *sentinel;

	// Update the state
	state._transactionState = !error;
	state._sendTime = timeStamp;
	state._error = attributes::errorCode(error);

	// Commit the data
	sentinel.commit();
}

auto TemplateTransaction::resolveAttribute(std::u16string_view name) -> const model::Attribute *
{
	/// @todo add any additional attributes this class supports, including attributes inherited from the client
	return model::Attribute::resolve(name,
		attributes::kTransactionState,
		attributes::kSendTime,
		attributes::kError);
}

auto TemplateTransaction::resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task>
{
	if (name == u"send"sv)
	{
		return std::shared_ptr<process::Task>(sharedFromThis(), &_sendTask);
	}

	/// @todo resolve any additional tasks

	return nullptr;
}

auto TemplateTransaction::resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event>
{
	// Check all the events we support
	if (name == u"sent"sv)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_sentEvent);
	}
	else if (name == u"sendError"sv)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_sendErrorEvent);
	}

	/// @todo resolve any additional events

	return nullptr;
}

auto TemplateTransaction::readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle
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

	return data::ReadHandle::Error::Unknown;
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