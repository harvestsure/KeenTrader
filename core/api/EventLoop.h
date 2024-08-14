
#pragma once

namespace Keen
{
    namespace api
    {
        extern KEEN_API_EXPORT void *get_main_event_loop();

        extern KEEN_API_EXPORT int run_main_event_loop();

        extern KEEN_API_EXPORT int exit_main_event_loop();

        class MessageData
        {
        public:
            virtual ~MessageData() {};
            virtual void Run() = 0;
        };

        template <class FunctorT>
        class MessageWithFunctor final : public MessageData
        {
        public:
            explicit MessageWithFunctor(FunctorT &&functor)
                : functor_(std::forward<FunctorT>(functor)) {}

            void Run() override { functor_(); }

        private:
            virtual ~MessageWithFunctor() override {}

            typename std::remove_reference<FunctorT>::type functor_;

            DISALLOW_COPY_AND_ASSIGN(MessageWithFunctor);
        };

        template <class FunctorT>
        void InvokeToQueue(FunctorT &&functor)
        {
            InvokeOnMainLoop(new MessageWithFunctor<FunctorT>(
                std::forward<FunctorT>(functor)));
        }

        KEEN_API_EXPORT void InvokeOnMainLoop(MessageData *pdata);
    }
}
