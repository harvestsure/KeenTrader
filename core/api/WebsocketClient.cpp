#include "Globals.h"
#include "WebsocketClient.h"
#include "Logger.h"
#include "LoggerListeners.h"

#define _WEBSOCKETPP_CPP11_STL_
#define ASIO_STANDALONE 1

#include <asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;


#define BUFLEN 65536

namespace Keen
{
	namespace api
	{
		class WebsocketClient::WebsocketClientImpl
		{
		public:
			typedef WebsocketClientImpl type;

			WebsocketClientImpl(asio::io_service* io_service, const std::string& uri, int m_ping_interval, const std::string& proxy_uri = "", WebsocketClient* client = nullptr)
				: m_uri(uri), m_proxy_uri(proxy_uri), m_ping_interval(m_ping_interval), m_io_service(io_service), m_reconnect_timer(*io_service), m_stopped(false), m_callback(client)
			{
				m_client.set_access_channels(websocketpp::log::alevel::none);
				m_client.set_error_channels(websocketpp::log::elevel::none);

				// Initialize ASIO
				m_client.init_asio(m_io_service);
				m_client.start_perpetual();

				// Register our handlers
				m_client.set_socket_init_handler(bind(&type::on_socket_init, this, ::_1));
				m_client.set_tls_init_handler(bind(&type::on_tls_init, this, ::_1));
				m_client.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
				m_client.set_open_handler(bind(&type::on_open, this, ::_1));
				m_client.set_pong_handler(bind(&type::on_pong, this, ::_1, ::_2));
				m_client.set_close_handler(bind(&type::on_close, this, ::_1));
				m_client.set_fail_handler(bind(&type::on_fail, this, ::_1));
			}

			~WebsocketClientImpl()
			{
				stop();
			}

		private:
			void on_socket_init(websocketpp::connection_hdl hdl)
			{
				// m_handle = hdl;
			}

			context_ptr on_tls_init(websocketpp::connection_hdl)
			{
				context_ptr ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

				try
				{
					ctx->set_options(asio::ssl::context::default_workarounds |
						asio::ssl::context::no_sslv2 |
						asio::ssl::context::no_sslv3 |
						asio::ssl::context::single_dh_use);
				}
				catch (std::exception& e)
				{
					LOGERROR("Error in context pointer: %s", e.what());
				}
				return ctx;
			}

			void on_fail(websocketpp::connection_hdl hdl)
			{
				if (m_callback)
				{
					m_callback->on_fail();
				}
			}

			void on_open(websocketpp::connection_hdl hdl)
			{
				m_hdl = hdl;
				start_ping();

				if (m_callback)
				{
					m_callback->on_connected();
				}
			}

			void on_message(websocketpp::connection_hdl, message_ptr msg)
			{
				std::string message = msg->get_payload();

				if (m_callback)
				{
					m_callback->on_text(message);
				}
			}

			void on_pong(websocketpp::connection_hdl, std::string msg)
			{
			}

			void on_close(websocketpp::connection_hdl)
			{
				if (m_callback)
				{
					m_callback->on_disconnected();
				}
			}

		public:
			void start()
			{
				connect();
			}

			void send(const std::string& message)
			{
				if (m_hdl.lock())
				{
					m_client.send(m_hdl, message, websocketpp::frame::opcode::text);
				}
				else
				{
					LOGERROR("Connection not open, cannot send message.");
				}
			}

			void stop()
			{
				m_stopped = true;
				if (m_hdl.lock())
				{
					websocketpp::lib::error_code ec;
					m_client.close(m_hdl, websocketpp::close::status::going_away, "", ec);
					if (ec)
					{
						LOGERROR("Error closing connection: %s", ec.message().c_str());
					}
				}

				m_client.stop_perpetual();
			}

			bool is_stopped() const
			{
				return m_stopped;
			}

		private:
			void connect()
			{
				if (m_stopped)
					return;

				websocketpp::lib::error_code ec;
				client::connection_ptr con = m_client.get_connection(m_uri, ec);

				if (!m_proxy_uri.empty())
				{
					con->set_proxy(m_proxy_uri);
				}

				if (ec)
				{
					std::cerr << "Connect initialization error: " << ec.message() << std::endl;
					return;
				}

				m_client.connect(con);
			}

			void start_ping()
			{
				if (m_stopped)
					return;

				m_ping_timer = std::make_shared<asio::steady_timer>(*m_io_service, m_ping_interval);
				m_ping_timer->async_wait([this](const asio::error_code& ec)
					{
						if (!ec) {
							if (m_hdl.lock())
							{
								websocketpp::lib::error_code ec;
								m_client.ping(m_hdl, "ping", ec);
								if (ec) {
									LOGERROR("Ping failed with exception: %d -- %s", ec.value(), ec.message().c_str());
								}
							}

							start_ping();
						} }
				);
			}

		public:
			void handle_reconnect()
			{
				if (m_stopped)
					return;

				LOGWARN("Connection closed, attempting to reconnect after 5 seconds...");

				m_reconnect_timer.expires_after(5s);
				m_reconnect_timer.async_wait([this](const asio::error_code& ec)
				{
					if (!ec) {
						connect();
					} 
				});
			}

		private:
			std::string m_uri;
			std::string m_proxy_uri;
			std::chrono::seconds m_ping_interval;
			client m_client;
			asio::io_service* m_io_service;
			asio::steady_timer m_reconnect_timer;
			websocketpp::connection_hdl m_hdl;
			std::shared_ptr<asio::steady_timer> m_ping_timer;
			std::atomic<bool> m_stopped;
			WebsocketClient* m_callback;
		};

		WebsocketClient::WebsocketClient()
			: m_ping_interval(10)
		{
			this->m_uri = "";
			this->_ws = nullptr;
		}

		WebsocketClient::~WebsocketClient()
		{
		}

		void WebsocketClient::init(
			AString host,
			AString proxy_host /* = ""*/,
			uint16_t proxy_port /* = 0*/,
			int ping_interval /* = 60*/)
		{
			this->m_uri = host;
			this->m_ping_interval = ping_interval; // seconds

			if (!proxy_host.empty() && proxy_port)
			{
				m_proxy_uri = Printf("http://%s:%d", proxy_host.c_str(), proxy_port);
			}
		}

		void WebsocketClient::start()
		{
			auto event_loop = get_main_event_loop();
			this->_ws.reset(new WebsocketClientImpl((asio::io_service*)event_loop, m_uri, m_ping_interval, m_proxy_uri, this));
			this->_ws->start();
		}

		void WebsocketClient::stop()
		{
			if (this->_ws)
			{
				this->_ws->stop();
			}
		}

		void WebsocketClient::reconnect()
		{
			if (this->_ws)
			{
				this->_ws->handle_reconnect();
			}
		}

		bool WebsocketClient::is_active() const
		{
			return this->_ws && !this->_ws->is_stopped();
		}

		void WebsocketClient::send_packet(const Json& packet)
		{
			AString text = packet.dump();
			this->_last_sent_text = text;

			if (this->_ws)
			{
				this->_ws->send(text);
			}
		}

		Json WebsocketClient::unpack_data(const AString& data)
		{
			return Json::parse(data);
		}

		void WebsocketClient::on_connected()
		{
			return;
		}

		void WebsocketClient::on_disconnected()
		{
			return;
		}

		void WebsocketClient::on_packet(const Json& packet)
		{
			return;
		}

		void WebsocketClient::on_error(const std::exception& e)
		{
			LOGERROR("WebsocketClient on error ----------\n%s", this->exception_detail(e).c_str());
		}

		void WebsocketClient::on_fail()
		{
			reconnect();
		}

		AString WebsocketClient::exception_detail(const std::exception& ex)
		{
			AString text = Printf("[%s]: Unhandled WebSocket Error:%s\n",
				DateTimeToString(currentDateTime()).c_str(), typeid(ex).name());
			text += Printf("LastSentText:\n%s\n", this->_last_sent_text.c_str());
			text += Printf("LastReceivedText:\n%s\n", this->_last_received_text.c_str());
			text += "Exception trace: \n";
			text += ex.what();
			text += " \n\n";
			return text;
		}

		void WebsocketClient::on_text(const AString& text)
		{
			this->_last_received_text = text;

			try
			{
				Json data = this->unpack_data(text);
				if (data.is_null())
				{
					return;
				}

				this->on_packet(data);
			}
			catch (const std::exception& e)
			{
				this->on_error(e);
			}
		}
	}
}
