#include <api/Globals.h>
#include <exchange/crypto_exchange.h>
#include <api/Logger.h>

using namespace Keen::api;

namespace Keen
{
	namespace exchange
	{
		CryptoExchange::CryptoExchange(EventEmitter* event_emitter, AString exchange_name)
			: BaseExchange(event_emitter, exchange_name)
		{
		}

		CryptoExchange::~CryptoExchange()
		{
		}

		bool CryptoExchange::set_position_mode(PositionMode mode)
		{
			position_mode = mode;
			
			AString mode_str = (mode == PositionMode::ONE_WAY) ? "ONE_WAY" : "HEDGE";
			write_log("设置持仓模式为: " + mode_str);
			
			return true;
		}

		PositionMode CryptoExchange::get_position_mode() const
		{
			return position_mode;
		}

		bool CryptoExchange::set_leverage(const AString& symbol, int leverage, const Json& params)
		{
			if (leverage <= 0)
			{
				write_log("杠杆倍数必须大于0，设置失败: " + symbol);
				return false;
			}

			leverage_config[symbol] = leverage;
			write_log("为 " + symbol + " 设置杠杆倍数: " + std::to_string(leverage));
			
			return true;
		}

		bool CryptoExchange::set_leverage_for_all_symbols(int leverage, const std::vector<AString>& symbols)
		{
			if (leverage <= 0)
			{
				write_log("杠杆倍数必须大于0，批量设置失败");
				return false;
			}

			bool all_success = true;
			
			for (const auto& symbol : symbols)
			{
				if (!set_leverage(symbol, leverage, Json::object()))
				{
					all_success = false;
				}
			}

			if (all_success)
			{
				write_log("成功为 " + std::to_string(symbols.size()) + " 个交易对设置杠杆倍数: " + std::to_string(leverage));
			}
			else
			{
				write_log("为部分交易对设置杠杆倍数失败，请检查");
			}

			return all_success;
		}

		int CryptoExchange::get_leverage(const AString& symbol) const
		{
			auto it = leverage_config.find(symbol);
			if (it != leverage_config.end())
			{
				return it->second;
			}
			return -1;  // 未设置
		}

		const std::map<AString, int>& CryptoExchange::get_all_leverages() const
		{
			return leverage_config;
		}
	}
}
