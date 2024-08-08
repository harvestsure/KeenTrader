#include <api/Globals.h>
#include "event.h"

namespace Keen
{
	namespace engine
	{
		const AString EVENT_TIMER = "eTimer";
		const AString EVENT_TICK = "eTick.";
		const AString EVENT_TRADE = "eTrade.";
		const AString EVENT_ORDER = "eOrder.";
		const AString EVENT_POSITION = "ePosition.";
		const AString EVENT_ACCOUNT = "eAccount.";
		const AString EVENT_QUOTE = "eQuote.";
		const AString EVENT_CONTRACT = "eContract.";
		const AString EVENT_LOG = "eLog";
		
		EventEmitter::EventEmitter(int interval/* = 1*/)
		{
			this->_interval = interval;
			this->_active = false;
		}

		EventEmitter::~EventEmitter()
		{

		}

		void EventEmitter::_run()
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

				Event &event = this->_queue.front();
				this->_process(event);
				this->_queue.pop_front();
				Lock.Unlock();
			}
		}

		void EventEmitter::_process(const Event& event)
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

		void EventEmitter::_run_timer()
		{
			while (this->_active)
			{
				std::this_thread::sleep_for(std::chrono::seconds(this->_interval));
				Event event = Event(EVENT_TIMER);
				this->put(event);
			}
		}

		void EventEmitter::start()
		{
			this->_active = true;
			this->_thread = std::thread(&EventEmitter::_run, this);
			this->_timer = std::thread(&EventEmitter::_run_timer, this);
		}

		void EventEmitter::stop()
		{
			this->_active = false;
			m_QueueNonempty.Set();
			this->_timer.join();
			this->_thread.join();
		}

		void EventEmitter::put(const Event& event)
		{
			cCSLock Lock(m_CS);
			this->_queue.push_back(event);
			m_QueueNonempty.Set();
		}

		void EventEmitter::Register(AString type, HandlerType handler)
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

		void EventEmitter::unRegister(AString type, HandlerType handler)
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

		void EventEmitter::register_general(HandlerType handler)
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

		void EventEmitter::unregister_general(HandlerType handler)
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
	}
}
