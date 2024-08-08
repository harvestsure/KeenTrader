#pragma once

namespace Keen
{
	namespace engine
	{
		extern KEEN_TRADER_EXPORT const AString EVENT_TIMER;
		extern KEEN_TRADER_EXPORT const AString EVENT_TICK;
		extern KEEN_TRADER_EXPORT const AString EVENT_TRADE;
		extern KEEN_TRADER_EXPORT const AString EVENT_ORDER;
		extern KEEN_TRADER_EXPORT const AString EVENT_POSITION;
		extern KEEN_TRADER_EXPORT const AString EVENT_ACCOUNT;
		extern KEEN_TRADER_EXPORT const AString EVENT_QUOTE;
		extern KEEN_TRADER_EXPORT const AString EVENT_CONTRACT;
		extern KEEN_TRADER_EXPORT const AString EVENT_LOG;

		class KEEN_TRADER_EXPORT Event
		{
		public:
			Event() = default;

			Event(AString type, std::any data = nullptr)
			{
				this->type = type;
				this->data = data;
			}

		public:
			AString type;
			std::any data;
		};

		using HandlerType = FnMut<void(const Event&)>;

		class KEEN_TRADER_EXPORT EventEmitter
		{
		public:

			EventEmitter(int interval = 1);

			virtual ~EventEmitter();

			void start();

			void stop();

			void put(const Event& event);

			void Register(AString type, HandlerType handler);

			void unRegister(AString type, HandlerType handler);

			void register_general(HandlerType handler);

			void unregister_general(HandlerType handler);

		protected:
			void _run();

			void _process(const Event& event);

			void _run_timer();

		private:
			int _interval;

			cCriticalSection	m_CS;
			std::deque<Event>	_queue;
			cEvent				m_QueueNonempty;

			bool _active = false;
			std::thread _thread;
			std::thread _timer;
			std::unordered_multimap<AString, HandlerType> _handlers;
			std::list<HandlerType> _general_handlers;
		};
	}
}
