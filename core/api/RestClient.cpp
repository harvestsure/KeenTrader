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
						request.on_failed(error, request);
					else
						this->on_failed(error, request);
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
			Request request = Request{
				.method = method,
				.path = path,
				.params = params,
				.headers = headers,
				.data = data
			};

			Response response = _sender->sync_request(request);

			return response;
		}

		Request& RestClient::sign(Request& request)
		{
			return request;
		}

		void RestClient::on_failed(const Error& error, const Request& request)
		{
			LOGERROR("RestClient on failed ----------\n\tcode:[%d] errorData:[%s] \n \trequest%s", 
				error.status(), error.errorData().c_str(), request.__str__().c_str());
		}

		void RestClient::on_error(const std::exception& ex, const Request& request)
		{
			LOGERROR("RestClient on error ----------\n\t%s", this->exception_detail(ex, request).c_str());
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
