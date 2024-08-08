#pragma once

#include <engine/object.h>


namespace Keen
{
	namespace engine
	{
		template<class T> T copy(const T &t)
		{
			return t;
		}

		template<class K, class V>
		std::optional<V> GetWithNone(const std::map<K, V>& maps, K key)
		{
			auto it = maps.find(key);
			if (it != maps.end())
			{
				return it->second;
			}
			else
			{
				return std::nullopt;
			}
		}

		template<class K, class V>
		V GetWithNull(const std::map<K, V>& maps, K key)
		{
			auto it = maps.find(key);
			if (it != maps.end())
			{
				return it->second;
			}
			else
			{
				return nullptr;
			}
		}

		template<class K, class V>
		V GetWithDefault(const std::map<K, V>& maps, K key, V d)
		{
			auto it = maps.find(key);
			if (it != maps.end())
			{
				return it->second;
			}
			else
			{
				return d;
			}
		}

		template<class K, class V>
		std::list<K> MapKeys(const std::map<K, V>& maps)
		{
			std::list<K> result;
			for (auto&[key, value] : maps)
			{
				result.push_back(key);
			}

			return result;
		}

		template<class K, class V>
		std::list<V> MapValues(const std::map<K, V>& maps)
		{
			std::list<V> result;
			for (auto&[key, value] : maps)
			{
				result.push_back(value);
			}

			return result;
		}

		extern KEEN_TRADER_EXPORT std::tuple<AString, Exchange> extract_kt_symbol(AString kt_symbol);

		extern KEEN_TRADER_EXPORT AString _get_trader_dir(AString temp_name);

		extern KEEN_TRADER_EXPORT AString get_file_path(AString filename);

		extern KEEN_TRADER_EXPORT AString get_folder_path(AString folder_name);

		extern KEEN_TRADER_EXPORT float round_to(float value, float target);

		extern KEEN_TRADER_EXPORT float floor_to(float value, float target);

		extern KEEN_TRADER_EXPORT float ceil_to(float value, float target);

		extern KEEN_TRADER_EXPORT float JsonToFloat(const Json& data);

		extern KEEN_TRADER_EXPORT int JsonToInt(const Json& data);

		extern KEEN_TRADER_EXPORT Json load_json(AString filename);

		extern KEEN_TRADER_EXPORT bool save_json(AString filename, const Json& data);


		class KEEN_TRADER_EXPORT BarGenerator
		{
		public:
			BarGenerator(
				FnMut<void(BarData)> on_bar,
				int window = 0,
				FnMut<void(BarData)> on_window_bar = nullptr,
				Interval interval = Interval::MINUTE
			);

			void update_tick(const TickData& tick);

			void update_bar(const BarData& bar);

			void generate();

		protected:
			std::optional<BarData> bar;
			std::optional<BarData> window_bar;
			FnMut<void(BarData)> on_bar;
			FnMut<void(BarData)> on_window_bar;

			int window;
			Interval interval;
			int interval_count;

			std::optional<TickData> last_tick;
			std::optional<BarData> last_bar;
		};
	}
}
