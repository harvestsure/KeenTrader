#pragma once

#include <engine/exchange.h>
#include <engine/constant.h>
#include <map>
#include <string>
#include <vector>

using namespace Keen::engine;

namespace Keen
{
	namespace exchange
	{
		/**
		 * @brief 数字货币交易所基础类
		 * 专门处理数字货币交易所的全局设置，如持仓模式和杠杆倍数
		 * 作为 BinanceLinearExchange 和 OkxExchange 的中间层
		 */
		class KEEN_EXCHANGE_EXPORT CryptoExchange : public BaseExchange
		{
		public:
			CryptoExchange(EventEmitter* event_emitter, AString exchange_name);
			virtual ~CryptoExchange();

			/**
			 * @brief 设置持仓模式（账户级全局设置）
			 * @param mode 持仓模式：ONE_WAY（单向）或 HEDGE（双向/对冲）
			 * @return true 设置成功，false 设置失败
			 */
			virtual bool set_position_mode(PositionMode mode);

			/**
			 * @brief 获取当前持仓模式
			 * @return 当前的持仓模式
			 */
			virtual PositionMode get_position_mode() const;

			/**
			 * @brief 为指定币种设置杠杆倍数
			 * @param symbol 交易对符号 (e.g., "BTCUSDT")
			 * @param leverage 杠杆倍数
			 * @param params 额外参数（可选）
			 * @return true 设置成功，false 设置失败
			 */
			virtual bool set_leverage(const AString& symbol, int leverage, const Json& params = Json::object());

			/**
			 * @brief 为所有交易对设置杠杆倍数
			 * @param leverage 杠杆倍数
			 * @param symbols 交易对列表
			 * @return true 全部设置成功，false 至少有一个设置失败
			 */
			virtual bool set_leverage_for_all_symbols(int leverage, const std::vector<AString>& symbols);

			/**
			 * @brief 获取指定币种的当前杠杆倍数
			 * @param symbol 交易对符号
			 * @return 杠杆倍数，-1 表示获取失败或未设置
			 */
			virtual int get_leverage(const AString& symbol) const;

			/**
			 * @brief 获取所有币种的杠杆倍数
			 * @return 币种与杠杆倍数的映射
			 */
			virtual const std::map<AString, int>& get_all_leverages() const;

		protected:
			// 持仓模式状态
			PositionMode position_mode = PositionMode::ONE_WAY;

			// 存储各交易对的杠杆倍数配置
			std::map<AString, int> leverage_config;
		};
	}
}
