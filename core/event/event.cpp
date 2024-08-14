#include <api/Globals.h>
#include "event.h"
#define ASIO_STANDALONE 1

#include <asio.hpp>

namespace Keen
{
	namespace engine
	{
		const char* EVENT_TIMER = "eTimer.";
		const char* EVENT_TICK = "eTick.";
		const char* EVENT_TRADE = "eTrade.";
		const char* EVENT_ORDER = "eOrder.";
		const char* EVENT_POSITION = "ePosition.";
		const char* EVENT_ACCOUNT = "eAccount.";
		const char* EVENT_QUOTE = "eQuote.";
		const char* EVENT_CONTRACT = "eContract.";
		const char* EVENT_LOG = "eLog.";


		class EventEmitterImpl : public EventEmitter
		{
		public:
			EventEmitterImpl(asio::io_service* io_service)
				: _interval(1)
				, _active(false)
				, _timer(*io_service)
			{

			}

			void start(int interval = 1)
			{
				this->_active = true;
				this->_interval = interval;

				this->_thread = std::thread(&EventEmitterImpl::_run, this);

				_run_timer();
			}

			void stop()
			{
				this->_active = false;
				m_QueueNonempty.Set();
				this->_thread.join();
			}

			void put(const Event& event)
			{
				cCSLock Lock(m_CS);
				this->_queue.push_back(event);
				m_QueueNonempty.Set();
			}

			void Register(const AString& type, HandlerType handler)
			{
				cCSLock Lock(m_CS);

				bool bExisted = false;
				for (auto pos = this->_handlers.equal_range(type); pos.first != pos.second; pos.first++)
				{
					if (pos.first->second.target_type().name() == handler.target_type().name())
					{
						bExisted = true;
						break;
					}
				}

				if (!bExisted)
				{
					this->_handlers.insert(std::make_pair(type, handler));
				}
			}

			void unRegister(const AString& type, HandlerType handler)
			{
				cCSLock Lock(m_CS);

				for (auto pos = this->_handlers.equal_range(type); pos.first != pos.second; pos.first++)
				{
					if (pos.first->second.target_type().name() == handler.target_type().name())
					{
						this->_handlers.erase(pos.first);
						break;
					}
				}
			}

			void register_general(HandlerType handler)
			{
				cCSLock Lock(m_CS);

				bool bExisted = false;
				for (auto it : this->_general_handlers)
				{
					if (it.target_type().name() == handler.target_type().name())
					{
						bExisted = true;
						break;
					}
				}

				if (!bExisted)
				{
					this->_general_handlers.push_back(handler);
				}
			}

			void unregister_general(HandlerType handler)
			{
				for (auto it = this->_general_handlers.begin();it!= this->_general_handlers.end();++it)
				{
					HandlerType curHandler = *it;
					if (curHandler.target_type().name() == handler.target_type().name())
					{
						this->_general_handlers.erase(it);
						break;
					}
				}
			}

		protected:
			void _run()
			{
				for (;;)
				{
					cCSLock Lock(m_CS);
					while (this->_active && (this->_queue.size() == 0))
					{
						cCSUnlock Unlock(Lock);
						m_QueueNonempty.Wait();
					}

					if (!this->_active)
					{
						return;
					}

					ASSERT(!this->_queue.empty());

					Event event = this->_queue.front();

					this->_queue.pop_front();
					Lock.Unlock();

					api::InvokeToQueue([this, event]() {
						this->_process(event);
					});
				}
			}

			void _process(const Event& event)
			{
				if (_handlers.count(event.type))
				{
					auto pr = _handlers.equal_range(event.type);
					while (pr.first != pr.second)
					{
						HandlerType handler = pr.first->second;
						handler(event);
						++pr.first;
					}
				}

				for (auto handler : this->_general_handlers)
				{
					handler(event);
				}
			}

			void _run_timer()
			{
				if (!this->_active)
					return;

				_timer.expires_after(std::chrono::seconds(this->_interval));
				_timer.async_wait([this](const asio::error_code& ec)
					{
						if (!ec)
						{
							Event event(EVENT_TIMER);
							this->put(event);

							_run_timer();
						}
					});
			}

		private:
			int _interval;

			cCriticalSection	m_CS;
			std::deque<Event>	_queue;
			cEvent				m_QueueNonempty;

			bool _active = false;
			std::thread _thread;
			asio::steady_timer _timer;
			std::unordered_multimap<AString, HandlerType> _handlers;
			std::list<HandlerType> _general_handlers;

		};

		EventEmitter* MakeEventEmitter()
		{
			static std::unique_ptr<EventEmitterImpl> instance;
			if (!instance)
			{
				auto event_loop = api::get_main_event_loop();
				instance = std::make_unique<EventEmitterImpl>((asio::io_service*)event_loop);
			}
			return instance.get();
		}
	}
}
