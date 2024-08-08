#pragma once

namespace Keen
{
    namespace api
    {
        class HTTPAbstractDoneHandler
        {
        public:
            virtual void operator()(const AString &requestId, const AString& data) = 0;
        };
        using HTTPDoneHandlerPtr = std::shared_ptr<HTTPAbstractDoneHandler>;

        class HTTPAbstractFailHandler
        {
        public:
            virtual void operator()(const AString &requestId, const Error &e) = 0;
        };

        using HTTPFailHandlerPtr = std::shared_ptr<HTTPAbstractFailHandler>;

        class HTTPAbstractErrorHandler
        {
        public:
            virtual void operator()(const AString& requestId, const std::exception& e) = 0;
        };

        using HTTPErrorHandlerPtr = std::shared_ptr<HTTPAbstractErrorHandler>;

        class HTTPAbstractProcessHandler
        {
        public:
            virtual void operator()(const AString &requestId, const Process &process) = 0;
        };
        using HTTPProcessHandlerPtr = std::shared_ptr<HTTPAbstractProcessHandler>;

        class HTTPResponseHandler
        {
        public:
            HTTPResponseHandler() = default;
            HTTPResponseHandler(HTTPDoneHandlerPtr &&done, HTTPFailHandlerPtr &&fail, HTTPErrorHandlerPtr&& error, HTTPProcessHandlerPtr &&process)
                : onDone(std::move(done)), onFail(std::move(fail)), onError(std::move(error)),  onProcess(std::move(process))
            {
            }
            HTTPDoneHandlerPtr onDone;
            HTTPFailHandlerPtr onFail;
            HTTPErrorHandlerPtr onError;
            HTTPProcessHandlerPtr onProcess;
        };
    }
}
