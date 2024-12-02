#include "Globals.h"
#include "RestClient.h"
#include "HttpClient/Sender.h"

namespace Keen
{
	namespace api
	{
		RestClient::RestClient()
		{
			_sender = new Sender();
		}

		RestClient::~RestClient()
		{
			if (_sender)
				delete _sender;
		}

		void RestClient::init(
			const AString& url_base,
			const AString& proxy_host,
			const uint16_t& proxy_port
		)
		{
			_sender->init(url_base, proxy_host, proxy_port);
		}

		void RestClient::start(int n/* = 3*/)
		{
		}

		void RestClient::stop()
		{
		}

		void RestClient::join()
		{
		}

		void RestClient::request(Request& request)
		{
			Response res;
			_sender->request<ResString>(this->sign(request))
				.done([=, this](const ResString& response)
				{
					try
					{
						request.callback(Json::parse(response.serialize()), request);
					}
					catch (const std::exception& e)
					{
						if (request.on_error)
							request.on_error(e, request);
						else
							this->on_error(e, request);
					}
				})
				.fail([=, this](const Error& error) {
					if (request.on_failed)
						request.on_failed(error.status(), request);
					else
						this->on_failed(error.status(), request);
				})
				.error([=, this](const std::exception& e)
				{
					if (request.on_error)
						request.on_error(e, request);
					else
						this->on_error(e, request);
				})
				.send();
		}

		Response RestClient::request(
			AString method,
			AString path,
			Params params,
			Json data,
			Headers headers
		)
		{
			std::promise<void> promise;
			Response res;

			Request request = Request{
				.method = method,
				.path = path,
				.params = params,
				.headers = headers,
				.data = data
			};

			_sender->request<ResString>(request)
				.done([&](const ResString& response) {
					//res = response;
					promise.set_value();
				})
				.fail([&](const Error& error) {
					res.code = error.code();
					res.status = error.status();
					res.reason = error.errorData();
					promise.set_value();
				})
				.error([&](const std::exception& e) {
					res.reason = e.what();
					promise.set_value();
				})
				.send();

			promise.get_future().get();

			return res;
		}

		Request& RestClient::sign(Request& request)
		{
			return request;
		}

		void RestClient::on_failed(int status_code, const Request& request)
		{
			LOGERROR("RestClient on failed ----------\n%s", request.__str__().c_str());
		}

		void RestClient::on_error(const std::exception& ex, const Request& request)
		{
			LOGERROR("RestClient on error ----------\n%s", this->exception_detail(ex, request).c_str());
		}

		AString RestClient::exception_detail(const std::exception& ex, const Request& request)
		{
			AString text = Printf("[%s]: Unhandled RestClient Error:%s\n",
				DateTimeToString(currentDateTime()).c_str(), typeid(ex).name());
			text += Printf("request:%s\n", request.__str__().c_str());
			text += "Exception trace: \n";
			text += ex.what();
			text += " \n\n";
			return text;
		}
	}
}
