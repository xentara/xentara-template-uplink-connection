// Copyright (c) embedded ocean GmbH
#pragma once

#include "Attributes.hpp"
#include "CustomError.hpp"

#include <xentara/memory/Array.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/plugin/EnableSharedFromThis.hpp>
#include <xentara/process/Event.hpp>
#include <xentara/process/Microservice.hpp>
#include <xentara/process/MicroserviceClass.hpp>
#include <xentara/process/Task.hpp>
#include <xentara/utils/core/Uuid.hpp>
#include <xentara/utils/tools/Unique.hpp>

#include <string_view>
#include <functional>
#include <forward_list>

namespace xentara::plugins::templateUplink
{

using namespace std::literals;

/// @brief A class representing a client for specific type of service that data can be sent to.
/// @todo rename this class to something more descriptive
class TemplateClient final : public process::Microservice, public plugin::EnableSharedFromThis<TemplateClient>
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
			return u"TemplateClient"sv;
		}
	
		auto uuid() const -> utils::core::Uuid final
		{
			/// @todo assign a unique UUID
			return "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"_uuid;
		}

		/// @}

	private:
	    /// @brief The array handle for the class specific attributes within an element's configuration
		memory::Array::ObjectHandle<Config> _configHandle { config().appendObject<Config>() };

		/// @brief The global object that represents the class
		static Class _instance;
	};

	/// @brief A handle used to access the client
	/// @todo implement a proper handle
	class Handle final : private utils::tools::Unique
	{
	public:
		// determines of the client is connected
		explicit operator bool() const noexcept
		{
			/// @todo return the actual state
			return false;
		}
	};

	// Interface for objects that want to be notified of errors
	class ErrorSink
	{
	public:
		/// @brief Virtual destructor
		/// @note The destructor is pure virtual (= 0) to ensure that this class will remain abstract, even if we should remove all
		/// other pure virtual functions later. This is not necessary, of course, but prefents the abstract class from becoming
		/// instantiable by accident as a result of refactoring.
		virtual ~ErrorSink() = 0;

		/// @brief Called on error, or on success.
		///
		/// This function is called in three instances, with different values for the *error* parameter:
		///
		/// Call reason                               | Value of the *error* parameter
		/// :---------------------------------------- | :------------------------------------------
		/// A connection was successfully established | a default constructed std::error_code object
		/// A connection was gracefully closed        | CustomError::NotConnected
		/// The connection was lost unexpectedly      | an appropriate error code
		virtual auto clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void = 0;
	};

	/// @brief Adds an error sink
	auto addErrorSink(std::reference_wrapper<ErrorSink> sink)
	{
		_errorSinks.push_front(sink);
	}

	/// @brief Request that the client be connected.
	///
	/// Each call to this function must be balanced by a call to requestDisconnect().
	/// 
	/// If this is the first request, then the connection will be attempted, and the function will not return until
	/// the connection has been successfully established, or has failed. In either case, error sinks will be notified,
	/// so any error sinks calling this must be prepared to have clientStateChanged() called from within this function.
	auto requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void;

	/// @brief Request that the client be disconnected.
	///
	/// Each call to this function must balance a corresponding call to requestConnect().
	/// 
	/// If this is the last request, then the connection will be closed, and the function will not return until
	/// the connection has been terminated. All error sinks will be notified with error code CustomError::NotConnected,
	/// so any error sinks calling this must be prepared to have clientStateChanged() called from within this function.
	auto requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void;
	
	/// @brief Notifies the client that an error was detected from outside, e.g. when sending a request to the client.
	/// 
	/// If this error affects the client as a whole, error sinks will be notified. If the sender is an error sink itself,
	/// and does not which to be notified, but intends to handle the error itself instead, it can pass a pointer to itself as the sender parameter. 
	auto handleError(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *sender = nullptr) noexcept -> void;

	/// @brief Checks whether the client is up
	auto connected() const -> bool
	{
		return bool(_handle);
	}

	/// @brief Returns a handle to the client
	auto handle() const -> const Handle &
	{
		return _handle;
	}

	/// @name Virtual Overrides for process::Microservice
	/// @{

	auto createSubservice(const process::MicroserviceClass &microserviceClass, plugin::SharedFactory<process::Microservice> &factory) -> std::shared_ptr<process::Microservice> final;

	auto resolveAttribute(std::u16string_view name) -> const model::Attribute * final;
	
	auto resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task> final;

	auto resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event> final;

	auto readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle final;

	auto realize() -> void final;
	
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
	/// @brief This structure represents the current state of the client
	struct State
	{
		/// @brief The state of the connection
		bool _connectionState { false };
		/// @brief The last time the component was connected or disconnected
		std::chrono::system_clock::time_point _connectionTime { std::chrono::system_clock::time_point::min() };
		/// @brief The error code when connecting, or 0 for none.
		attributes::ErrorCode _error { attributes::errorCode(CustomError::NotConnected) };
	};

	/// @brief This class providing callbacks for the Xentara scheduler for the "reconnect" task
	class ReconnectTask final : public process::Task
	{
	public:
		/// @brief This constuctor attached the task to its target
		ReconnectTask(std::reference_wrapper<TemplateClient> target) : _target(target)
		{
		}

		/// @name Virtual Overrides for process::Task
		/// @{

		auto stages() const -> Stages final
		{
			return Stage::PreOperational | Stage::Operational | Stage::PostOperational;
		}

		auto preparePreOperational(const process::ExecutionContext &context) -> Status final;

		auto preOperational(const process::ExecutionContext &context) -> Status final;

		auto operational(const process::ExecutionContext &context) -> void final;

		auto preparePostOperational(const process::ExecutionContext &context) -> Status final;

		/// @}

	private:
		/// @brief A reference to the target element
		std::reference_wrapper<TemplateClient> _target;
	};
	
	/// @brief This function is called by the "reconnect" task.
	///
	/// This function attempts to reconnect any disconnected I/O components.
	auto performReconnectTask(const process::ExecutionContext &context) -> void;

	/// @brief Attempts to establish a connection to the client and updates the state accordingly.
	///
	/// This function will notify error sinks if anything changes.
	auto connect(std::chrono::system_clock::time_point timeStamp) -> void;

	/// @brief Terminates the connection to the client and updates the state accordingly.
	///
	/// This function will notify error sinks if anything changes.
	auto disconnect(std::chrono::system_clock::time_point timeStamp) -> void;

	/// @brief Updates the state and sends events
	auto updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *excludeErrorSink = nullptr) -> void;

	/// @brief Checks whether an error is the result of a lost connection
	static auto isConnectionError(std::error_code error) noexcept -> bool;

	/// @brief A Xentara event that is fired when the connection is established
	process::Event _connectedEvent;
	/// @brief A Xentara event that is fired when the connection is closed or lost
	process::Event _disconnectedEvent;

	/// @brief The "reconnect" task
	ReconnectTask _reconnectTask { *this };

	/// @brief A list of objects that want to be notified of errors
	std::forward_list<std::reference_wrapper<ErrorSink>> _errorSinks;

	/// @brief The number of people who would like this component to be connected
	std::atomic<std::size_t> _connectionRequestCount { 0 };

	/// @brief A handle to the client
	Handle _handle;
	/// @brief The last error we encountered.
	/// 
	/// May have the following values:
	/// - If the connection is open, this will be a default constructed std::error_code object
	/// - If the connection was closed gracefully, this will be CustomError::NotConnected;
	/// - Otherwise, this will contain an appropriate error code
	std::error_code _lastError { CustomError::NotConnected };

	/// @brief The data block that contains the state
	memory::ObjectBlock<memory::memoryResources::Data, State> _stateDataBlock;
};

inline TemplateClient::ErrorSink::~ErrorSink() = default;

} // namespace xentara::plugins::templateUplink