#include "Globals.h" // NOTE: MSVC stupidness requires this to be the same across all modules
#include "EventLoop.h"
#include "OSSupport/Singleton.h"

#define _WEBSOCKETPP_CPP11_STL_
#define ASIO_STANDALONE 1

#include <asio.hpp>
#include <iostream>

namespace Keen
{
	namespace api
	{
		// Singleton pattern for managing the io_service instance
		class IOService : public Singleton<IOService>
		{
		public:
			asio::io_service& get_io_service()
			{
				static asio::io_service instance;
				return instance;
			}

			asio::strand<asio::io_context::executor_type>& get_strand()
			{
				static asio::strand<asio::io_context::executor_type> instance(get_io_service().get_executor());
				return instance;
			}
		};

		void* get_main_event_loop()
		{
			return &IOService::get_instance().get_io_service();
		}

		int run_main_event_loop()
		{
			auto& io_service = IOService::get_instance().get_io_service();

			try
			{
				io_service.run();
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception in run_main_event_loop: " << e.what() << std::endl;
				return 1;
			}

			return 0;
		}

		int exit_main_event_loop()
		{
			auto& io_service = IOService::get_instance().get_io_service();

			try
			{
				io_service.stop();
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception in exit_main_event_loop: " << e.what() << std::endl;
				return 1;
			}

			return 0;
		}

		template <class F, class... Args>
		auto enqueue(F&& f, Args &&...args)
			-> std::future<typename std::invoke_result<F, Args...>::type>
		{
			using return_type = typename std::invoke_result<F, Args...>::type;

			auto task = std::make_shared<std::packaged_task<return_type()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...));

			std::future<return_type> res = task->get_future();
			auto& strand = IOService::get_instance().get_strand();
			asio::post(strand, [task]()
				{ (*task)(); });

			return res;
		}

		void InvokeOnMainLoop(MessageData* pdata)
		{
			auto& strand = IOService::get_instance().get_strand();
			asio::post(strand, [pdata]() {
				if (pdata)
				{
					pdata->Run();
					delete pdata;
				} 
			});
		}

		void DelayOnMainLoop(int seconds, MessageData* pdata)
		{
			auto timer = std::make_shared<asio::steady_timer>(
				IOService::get_instance().get_io_service(),
				asio::chrono::seconds(seconds));

			timer->async_wait([pdata, timer](const asio::error_code& /*e*/) {
				if (pdata)
				{
					pdata->Run();
					delete pdata;
				}
			});
		}
	}
}
