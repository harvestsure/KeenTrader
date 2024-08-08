

#include <api/Globals.h>
#include <engine/engine.h>
#include <engine/engine.h>
#include <exchange/okx_exchange.h>
#include <exchange/binance_exchange.h>

#include <app/cta_strategy/cta_strategy.h>
#include <app/cta_strategy/engine.h>


using namespace Keen::engine;
using namespace Keen::exchange;
using namespace Keen::app;


#ifdef _MSC_VER
#include <tchar.h>
#include <time.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp")

std::wstring get_sys_time()
{
	time_t t;
	tm* local;
	tm* gmt;
	wchar_t buf[128] = { 0 };

	t = time(NULL);
	local = localtime(&t);
	wcsftime(buf, 64, L"%Y_%m_%d_%H_%M_%S", local);

	return buf;
}

static LONG WINAPI crashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	MINIDUMP_EXCEPTION_INFORMATION M;
	HANDLE hDump_File;
	wchar_t Dump_Path[512];

	M.ThreadId = GetCurrentThreadId();
	M.ExceptionPointers = ExceptionInfo; // got by GetExceptionInformation()
	M.ClientPointers = 0;

	GetModuleFileNameW(nullptr, Dump_Path, sizeof(Dump_Path));
	lstrcatW(Dump_Path, get_sys_time().c_str());
	lstrcatW(Dump_Path, L".dmp");

	hDump_File = CreateFileW(Dump_Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDump_File, (MINIDUMP_TYPE)(MiniDumpWithFullMemory),
		ExceptionInfo ? &M : nullptr, nullptr, nullptr);

	CloseHandle(hDump_File);

	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

int main()
{

#ifdef _MSC_VER
	SetUnhandledExceptionFilter(crashHandler);
#endif
	ConsoleSignalHandler::Register();

	EventEmitter* event_emitter = new EventEmitter;
	TradeEngine* trade_engine = new TradeEngine(event_emitter);

	auto okex = trade_engine->add_exchange<okx::OkxExchange>();
	trade_engine->add_exchange<binance::BinanceExchange>();

	CtaEngine* cta_engine = dynamic_cast<CtaEngine*>(trade_engine->add_app<CtaStrategyApp>());
	trade_engine->write_log("Main engine was created successfully");

	LogEngine* log_engine = dynamic_cast<LogEngine*>(trade_engine->get_engine("log"));
	event_emitter->Register(EVENT_CTA_LOG, std::bind(&LogEngine::process_log_event, log_engine, std::placeholders::_1));
	trade_engine->write_log("Register log event listeners");

#ifdef _DEBUG
	Json SETTINGS =
	{
		{"API Key", ""},
		{"Secret Key", ""},
		{"Passphrase", ""},
		{"Proxy Host", ""},
		{"Proxy Port", 0},
		{"Server", "TEST"}
	};
#else
	Json SETTINGS =
	{
		{"API Key", ""},
		{"Secret Key", ""},
		{"Passphrase", ""},
		{"Proxy Host", ""},
		{"Proxy Port", 0},
		{"Server", "REAL"}
	};
#endif // DEBUG

	trade_engine->connect(SETTINGS, "OKX");
	trade_engine->write_log("Connecting to OKX interface");

	SubscribeRequest req;
	req.exchange = Exchange::OKX;
	req.symbol = "LDO-USDT-SWAP";
	// okex->subscribe(req);

	SETTINGS =
	{
		{"key", ""},
		{"secret", ""},
		{"session_number", 3},
		{"proxy_host", ""},
		{"proxy_port", 0} };

	// trade_engine->connect(SETTINGS, "BINANCE");
	// trade_engine->write_log("Connecting to BINANCE interface");

	std::thread t([=] {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		OrderRequest order_req{
			.symbol = "ETH-USDT-SWAP",
			.exchange = Exchange::OKX,
			.direction = Direction::LONG,
			.type = OrderType::LIMIT,
			.volume = 1,
			.price = 120,
			.offset = Offset::OPEN,
			.exchange_name = "OKX" };

		AString orderid = okex->send_order(order_req);

		std::this_thread::sleep_for(std::chrono::seconds(1));

		CancelRequest cancel_req{
			.orderid = orderid,
			.symbol = "ETH-USDT-SWAP",
			.exchange = Exchange::OKX };

		okex->cancel_order(cancel_req);
		});

	cta_engine->init_engine();
	trade_engine->write_log("CTA strategy initialization completed");

	cta_engine->init_all_strategies();
	std::this_thread::sleep_for(std::chrono::seconds(2)); // Leave enough time to complete strategy initialization;
	trade_engine->write_log("CTA strategies are fully initialized");

	cta_engine->start_all_strategies();
	trade_engine->write_log("All CTA strategies are activated");

	run_main_event_loop();


	SAFE_RELEASE(trade_engine);
	SAFE_RELEASE(event_emitter);
}