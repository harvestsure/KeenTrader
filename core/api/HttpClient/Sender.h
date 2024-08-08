#pragma once

#include "Model.h"
#include "Callback.h"

namespace Keen
{
	namespace api
	{
		class HTTPAbstractDoneHandler;
		class KEEN_API_EXPORT Sender
		{
		public:
			class RequestBuilder
			{
			public:
				RequestBuilder(const RequestBuilder& other) = delete;
				RequestBuilder& operator=(const RequestBuilder& other) = delete;
				RequestBuilder& operator=(RequestBuilder&& other) = delete;

			protected:
				template <typename Response>
				class DoneHandler : public HTTPAbstractDoneHandler
				{
					using Callback = FnMut<void(const Response& result)>;

				public:
					DoneHandler(Callback handler) : _handler(std::move(handler))
					{
					}
					void operator()(const AString& requestId, const AString& data) override
					{
						auto handler = std::move(_handler);
						auto result = Response(data);
						result.read();
						handler(result);
					}
					virtual ~DoneHandler()
					{
					}

				private:
					Callback _handler;
				};

				class FailHandler : public HTTPAbstractFailHandler
				{
					using Callback = FnMut<void(const Error& error)>;

				public:
					FailHandler(Callback handler)
						: _handler(std::move(handler))
					{
					}
					void operator()(const AString& requestId, const Error& error) override
					{
						if (_handler)
						{
							_handler(error);
						}
					}

				private:
					Callback _handler;
				};

				class ErrorHandler : public HTTPAbstractErrorHandler
				{
					using Callback = FnMut<void(const std::exception& e)>;

				public:
					ErrorHandler(Callback handler)
						: _handler(std::move(handler))
					{
					}
					void operator()(const AString& requestId, const std::exception& e) override
					{
						if (_handler)
						{
							_handler(e);
						}
					}

				private:
					Callback _handler;
				};

				class ProcessHandler : public HTTPAbstractProcessHandler
				{
					using Callback = FnMut<void(const Process& process)>;

				public:
					ProcessHandler(Callback handler)
						: _handler(std::move(handler))
					{
					}
					void operator()(const AString& requestId, const Process& error) override
					{
						if (_handler)
						{
							_handler(error);
						}
					}

				private:
					Callback _handler;
				};

				explicit RequestBuilder(Sender* sender) noexcept : _sender(sender)
				{
				}
				RequestBuilder(RequestBuilder&& other) = default;

				virtual ~RequestBuilder()
				{
				}

				void setDoneHandler(HTTPDoneHandlerPtr&& handler) noexcept
				{
					_done = std::move(handler);
				}
				void setFailHandler(HTTPFailHandlerPtr&& handler) noexcept
				{
					_fail = std::move(handler);
				}
				void setErrorHandler(HTTPErrorHandlerPtr&& handler) noexcept
				{
					_error = std::move(handler);
				}
				void setProcessHandler(HTTPProcessHandlerPtr&& handler) noexcept
				{
					_process = std::move(handler);
				}

				Sender* sender() const noexcept
				{
					return _sender;
				}

				HTTPDoneHandlerPtr takeOnDone() noexcept
				{
					return std::move(_done);
				}
				HTTPFailHandlerPtr takeOnFail()
				{
					return std::move(_fail);
				}
				HTTPErrorHandlerPtr takeOnError()
				{
					return std::move(_error);
				}
				HTTPProcessHandlerPtr takeOnProcess()
				{
					return std::move(_process);
				}

			private:
				HTTPDoneHandlerPtr _done;
				HTTPFailHandlerPtr _fail;
				HTTPErrorHandlerPtr _error;
				HTTPProcessHandlerPtr _process;
				Sender* _sender;
			};

			template <typename Response>
			class AsyncRequestBuilder : public RequestBuilder
			{
			private:
				friend class Sender;
				AsyncRequestBuilder(Sender* sender, const Request& request) noexcept : RequestBuilder(sender), _request(request)
				{
				}
				AsyncRequestBuilder(AsyncRequestBuilder& other) = default;

			public:
				[[nodiscard]] AsyncRequestBuilder& done(FnMut<void(const Response& result)> callback)
				{
					setDoneHandler(std::make_shared<DoneHandler<Response>>(std::move(callback)));
					return *this;
				}

				[[nodiscard]] AsyncRequestBuilder& fail(FnMut<void(const Error& error)> callback) noexcept
				{
					setFailHandler(std::make_shared<FailHandler>(std::move(callback)));
					return *this;
				}

				[[nodiscard]] AsyncRequestBuilder& error(FnMut<void(const std::exception& e)> callback) noexcept
				{
					setErrorHandler(std::make_shared<ErrorHandler>(std::move(callback)));
					return *this;
				}

				[[nodiscard]] AsyncRequestBuilder& process(FnMut<void(const Process& error)> callback) noexcept
				{
					setProcessHandler(std::make_shared<ProcessHandler>(std::move(callback)));
					return *this;
				}

				AString send()
				{
					return sender()->send(
						_request,
						takeOnDone(),
						takeOnProcess(),
						takeOnFail(),
						takeOnError());
				}

			private:
				const Request& _request;
			};

		public:
			AString send(
				const Request& request,
				HTTPDoneHandlerPtr&& onDone,
				HTTPProcessHandlerPtr&& onProcess = nullptr,
				HTTPFailHandlerPtr&& onFail = nullptr,
				HTTPErrorHandlerPtr&& OnError = nullptr)
			{
				AString requestStr = generateUuid();
				sendRequest(requestStr, request, HTTPResponseHandler(std::move(onDone), std::move(onFail), std::move(OnError), std::move(onProcess)));
				return requestStr;
			}

			void cancel(const AString& requestId);

		private:
			void sendRequest(const AString& requestId, const Request& request, HTTPResponseHandler&& callbacks);

		public:
			virtual ~Sender();

			explicit Sender() noexcept;

			void init(
				const AString& url_base,
				const AString& proxy_host = AString(),
				const uint16_t& proxy_port = 0);

			template <typename Response>
			[[nodiscard]] AsyncRequestBuilder<Response> request(const Request& request) noexcept;

		private:
			void storeRequest(const AString& requestId, HTTPResponseHandler&& callbacks);
			void unregisterRequest(const AString& requestId);

		private:
			AString _url_base;
			AString _proxy_host;
			uint16_t _proxy_port;

			std::mutex _queue_mutex;
			std::map<AString, HTTPResponseHandler> _parserMap;
		};

		template <typename Response>
		Sender::AsyncRequestBuilder<Response> Sender::request(const Request& request) noexcept
		{
			return AsyncRequestBuilder<Response>(this, request);
		}
	}
}
