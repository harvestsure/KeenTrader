#pragma once

namespace Keen
{
	namespace engine
	{
		


        enum class Direction {
            LONG,
            SHORT,
            NET
        };

        enum class Offset {
            NONE,
            OPEN,
            CLOSE,
            CLOSETODAY,
            CLOSEYESTERDAY
        };

        enum class Status {
            SUBMITTING,
            NOTTRADED,
            PARTTRADED,
            ALLTRADED,
            CANCELLED,
            REJECTED
        };

        enum class Product {
            EQUITY,
            FUTURES,
            OPTION,
            INDEX,
            FOREX,
            SPOT,
            ETF,
            BOND,
            WARRANT,
            SPREAD,
            FUND,
            CFD,
            SWAP
        };

        enum class OrderType {
            LIMIT,
            MARKET,
            STOP,
            FAK,
            FOK,
            RFQ
        };

        enum class OptionType {
            None,
            CALL,
            PUT
        };

        enum class Exchange {
            CFFEX,         // China Financial Futures Exchange
            SHFE,          // Shanghai Futures Exchange
            CZCE,          // Zhengzhou Commodity Exchange
            DCE,           // Dalian Commodity Exchange
            INE,           // Shanghai International Energy Exchange
            GFEX,          // Guangzhou Futures Exchange
            SSE,           // Shanghai Stock Exchange
            SZSE,          // Shenzhen Stock Exchange
            BSE,           // Beijing Stock Exchange
            SHHK,          // Shanghai-HK Stock Connect
            SZHK,          // Shenzhen-HK Stock Connect
            SGE,           // Shanghai Gold Exchange
            WXE,           // Wuxi Steel Exchange
            CFETS,         // CFETS Bond Market Maker Trading System
            XBOND,         // CFETS X-Bond Anonymous Trading System

            SMART,         // Smart Router for US stocks
            NYSE,          // New York Stock Exchnage
            NASDAQ,        // Nasdaq Exchange
            ARCA,          // ARCA Exchange
            EDGEA,         // Direct Edge Exchange
            ISLAND,        // Nasdaq Island ECN
            BATS,          // Bats Global Markets
            IEX,           // The Investors Exchange
            AMEX,          // American Stock Exchange
            TSE,           // Toronto Stock Exchange
            NYMEX,         // New York Mercantile Exchange
            COMEX,         // COMEX of CME
            GLOBEX,        // Globex of CME
            IDEALPRO,      // Forex ECN of Interactive Brokers
            CME,           // Chicago Mercantile Exchange
            ICE,           // Intercontinental Exchange
            SEHK,          // Stock Exchange of Hong Kong
            HKFE,          // Hong Kong Futures Exchange
            SGX,           // Singapore Global Exchange
            CBOT,          // Chicago Board of Trade
            CBOE,          // Chicago Board Options Exchange
            CFE,           // CBOE Futures Exchange
            DME,           // Dubai Mercantile Exchange
            EUREX,         // Eurex Exchange
            APEX,          // Asia Pacific Exchange
            LME,           // London Metal Exchange
            BMD,           // Bursa Malaysia Derivatives
            TOCOM,         // Tokyo Commodity Exchange
            EUNX,          // Euronext Exchange
            KRX,           // Korean Exchange
            OTC,           // OTC Product (Forex/CFD/Pink Sheet Equity)
            IBKRATS,       // Paper Trading Exchange of IB
            LOCAL,         // For local generated data

            // CryptoCurrency
            BITMEX,         // "BITMEX"
            OKX,            // "OKX"
            HUOBI,          // "HUOBI"
            BITFINEX,       // "BITFINEX"
            BINANCE,        // "BINANCE"
            BYBIT,          // "BYBIT"
            COINBASE,       // "COINBASE"
            BITSTAMP,       // "BITSTAMP"
        };

        enum class Currency {
            USD,
            HKD,
            CNY,
            CAD
        };

        enum class Interval {
            None,
            MINUTE,
            HOUR,
            DAILY,
            WEEKLY,
            TICK
        };

        extern KEEN_ENGINE_EXPORT AString str_direction(Direction direction);
		extern KEEN_ENGINE_EXPORT AString exchange_to_str(Exchange);
        extern KEEN_ENGINE_EXPORT AString status_to_str(Status);
        extern KEEN_ENGINE_EXPORT AString interval_to_str(Interval);

		extern KEEN_ENGINE_EXPORT Exchange str_to_exchange(AString);
		extern KEEN_ENGINE_EXPORT Interval str_to_interval(AString);

	}
}
