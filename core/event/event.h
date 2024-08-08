#pragma once

namespace Keen
{
	namespace engine
	{
		extern KEEN_EVENT_EXPORT const char* EVENT_TIMER;
		extern KEEN_EVENT_EXPORT const char* EVENT_TICK;
		extern KEEN_EVENT_EXPORT const char* EVENT_TRADE;
		extern KEEN_EVENT_EXPORT const char* EVENT_ORDER;
		extern KEEN_EVENT_EXPORT const char* EVENT_POSITION;
		extern KEEN_EVENT_EXPORT const char* EVENT_ACCOUNT;
		extern KEEN_EVENT_EXPORT const char* EVENT_QUOTE;
		extern KEEN_EVENT_EXPORT const char* EVENT_CONTRACT;
		extern KEEN_EVENT_EXPORT const char* EVENT_LOG;

		class Event
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

		class EventEmitter
		{
		public:
			using HandlerType = FnMut<void(const Event&)>;

			virtual ~EventEmitter() {
			}

			virtual void start(int interval = 1) = 0;

			virtual void stop() = 0;

			virtual void put(const Event& event) = 0;

			virtual void Register(const AString& type, HandlerType handler) = 0;

			virtual void unRegister(const AString& type, HandlerType handler) = 0;

			virtual void register_general(HandlerType handler) = 0;

			virtual void unregister_general(HandlerType handler) = 0;
		};

		extern "C" KEEN_EVENT_EXPORT EventEmitter* MakeEventEmitter();
	}
}
