#pragma once

namespace Keen
{
	namespace api
	{		
		class KEEN_API_EXPORT WebsocketClient
		{
		public:

			WebsocketClient();
			virtual ~WebsocketClient();

			void init(
				AString host,
				AString proxy_host = "",
				uint16_t proxy_port = 0,
				int ping_interval = 60);

			void start();

			void stop();

			void reconnect();

			bool is_active() const;

			void send_packet(const Json& packet);

			virtual Json unpack_data(const AString& data);

			virtual void on_connected();

			virtual void on_disconnected();

			virtual void on_fail();

			virtual void on_packet(const Json& packet);

			virtual void on_error(const std::exception& e);

			AString exception_detail(const std::exception& ex);

		protected:
			void on_text(const AString& data);

		protected:
			class WebsocketClientImpl;

			std::shared_ptr<WebsocketClientImpl> _ws;

			AString m_uri;

			AString m_proxy_uri;

			int m_ping_interval;

			AString _last_sent_text;
			AString _last_received_text;
		};
	}
}
