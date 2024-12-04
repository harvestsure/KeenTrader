#include "Globals.h"
#include "Sender.h"
#include "EventLoop.h"
#include "OSSupport/ThreadPool.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"


namespace Keen
{
	namespace api
	{
		httplib::Headers FromHeaders(const Headers& headers)
		{
			httplib::Headers h;
			h.insert(headers.begin(), headers.end());

			return h;
		}

		httplib::Params FromParams(const Params& params)
		{
			httplib::Params p;
			p.insert(params.begin(), params.end());

			return p;
		}

		Headers ToHeaders(const httplib::Headers& headers)
		{
			Headers h;
			h.insert(headers.begin(), headers.end());

			return h;
		}

		AString exception_detail(const std::exception& ex, const Request& request)
		{
			AString text = Printf("[%s]: Unhandled RestClient Error:%s\n",
				DateTimeToString(currentDateTime()).c_str(), typeid(ex).name());
			text += Printf("request:%s\n", request.__str__().c_str());
			text += "Exception trace: \n";
			text += ex.what();
			text += " \n\n";
			return text;
		}

		Response _http_request(const Request& request, const AString& url_base, const AString& proxy_host, uint16_t proxy_port)
		{
			httplib::Client cli(url_base);

			if (!proxy_host.empty() && proxy_port)
			{
				cli.set_proxy(proxy_host, proxy_port);
			}

			httplib::Request req;
			req.method = request.method;
			req.path = request.path;
			req.headers = FromHeaders(request.headers);
			req.params = FromParams(request.params);
			req.body = RequestDataToString(request.data);

			if (request.method == "GET" && req.params.size())
			{
				req.path = httplib::append_query_params(request.path, req.params);
			}

			httplib::Result res = cli.send(req);
			auto error = res.error();

			Response response;
			response.code = (int)error;

			if (res) {
				response.version = res->version;
				response.status = res->status;
				response.reason = res->reason;
				response.headers = ToHeaders(res->headers);
				response.body = res->body;
				response.location = res->location;
			}
			else {
				response.status = (int)error;
				response.reason = httplib::to_string(error);
			}

			return response;
		}

		void Sender::sendRequest(const AString& requestId, const Request& request, HTTPResponseHandler&& callbacks)
		{
			storeRequest(requestId, std::move(callbacks));

			auto do_http_request = [=, this]()
				{
					Response response = _http_request(request, this->_url_base, this->_proxy_host, this->_proxy_port);

					bool success = response.code == 0 && response.status / 100 == 2;

					InvokeToQueue([=, this]() {
						HTTPResponseHandler h;
						{
							std::lock_guard<std::mutex> lock(_queue_mutex);
							auto it = _parserMap.find(requestId);
							if (it != _parserMap.cend()) {
								h = it->second;
								_parserMap.erase(it);
							}
						}
						try
						{
							if (success) {
								if (h.onDone) {
									(*h.onDone)(requestId, response.body);
								}
							}
							else {
								LOGWARN("%s", request.__str__().c_str());

								if (h.onFail) {
									Error error(response.code, response.status, response.body);
									(*h.onFail)(requestId, error);
								}
							}
						}
						catch (const std::exception& e)
						{
							LOGERROR("%s", exception_detail(e, request).c_str());

							if (h.onError) {
								Error error(response.code, response.status, response.reason);
								(*h.onError)(requestId, e);
							}
						}

						unregisterRequest(requestId);
					});
				};

			ThreadPool::get_instance().enqueue(do_http_request);
		}

		Response Sender::sync_request(const Request& request) noexcept {
			Response response = _http_request(request, this->_url_base, this->_proxy_host, this->_proxy_port);

			return response;
		}

		void Sender::cancel(const AString& requestId)
		{
			unregisterRequest(requestId);
		}

		Sender::Sender() noexcept
			: _url_base(), _proxy_host(), _proxy_port(0) {

		}

		Sender::~Sender()
		{
			std::lock_guard<std::mutex> lock(_queue_mutex);
			_parserMap.clear();
		}

		void Sender::init(
			const AString& url_base,
			const AString& proxy_host,
			const uint16_t& proxy_port) 
		{
			this->_url_base = url_base;
			this->_proxy_host = proxy_host;
			this->_proxy_port = proxy_port;
		}

		void Sender::storeRequest(const AString& requestId, HTTPResponseHandler&& callbacks) {
			std::lock_guard<std::mutex> lock(_queue_mutex);
			_parserMap.emplace(requestId, std::move(callbacks));
		}

		void Sender::unregisterRequest(const AString& requestId) {
			std::lock_guard<std::mutex> lock(_queue_mutex);
			_parserMap.erase(requestId);
		}
	}
}
