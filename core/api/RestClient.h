#pragma once

#include "HttpClient/Sender.h"

namespace Keen
{
	namespace api
	{
		class KEEN_API_EXPORT RestClient
		{
		public:
			RestClient();
			virtual ~RestClient();

			void init(
				const AString& url_base,
				const AString& proxy_host = AString(),
				const uint16_t& proxy_port = 0);

			void start(int n = 3);

			void stop();

			void join();

			void request(Request& request);

			Response request(
				AString method,
				AString path,
				Params params = Params(),
				Json data = Json(),
				Headers headers = Headers());

			virtual Request &sign(Request &request);

			virtual void on_failed(int status_code, const Request &request);

			virtual void on_error(const std::exception &ex, const Request &request);

			AString exception_detail(const std::exception &ex, const Request &request);

		protected:
			Sender *_sender;
		};
	}
}
