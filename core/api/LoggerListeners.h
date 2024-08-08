
#include "Logger.h"

KEEN_API_EXPORT std::unique_ptr<cLogger::cListener> MakeConsoleListener();
KEEN_API_EXPORT std::unique_ptr<cLogger::cListener> MakeFileListener();




