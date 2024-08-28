#include <api/Globals.h>
#include "utility.h"

#include<date/date.h>


namespace Keen
{
	namespace engine
	{
		std::tuple<AString, Exchange> extract_kt_symbol(AString kt_symbol)
		{
			AStringVector ret = StringSplit(kt_symbol, ".");
			AString symbol = ret[0];
			AString exchange_str = ret[1];

			return std::make_tuple(symbol, str_to_exchange(exchange_str));
		}

		fs::path pathHome() {
			const char* homeDir = nullptr;

#ifdef _WIN32
			homeDir = std::getenv("USERPROFILE");
#else
			homeDir = std::getenv("HOME");
#endif

			if (homeDir) {
				return std::filesystem::path(homeDir);
			}
			else {
				throw std::runtime_error("Cannot determine the user's home directory.");
			}
		}

		fs::path _get_trader_dir(AString temp_name)
		{
			auto temp_path = fs::current_path();
			temp_path.append(temp_name);

			if (fs::exists(temp_path))
			{
				return temp_path.string();
			}

			temp_path = pathHome();
			temp_path.append(temp_name);

			if (!fs::exists(temp_path))
			{
				fs::create_directory(temp_path);
			}

			return temp_path.string();
		}

		AString get_file_path(AString filename)
		{
			fs::path TEMP_DIR = _get_trader_dir(".keen_trader");
			return TEMP_DIR.append(filename).string();
		}

		AString get_folder_path(AString folder_name)
		{
			fs::path TEMP_DIR = _get_trader_dir(".keen_trader");
			auto folder_path = TEMP_DIR.append(folder_name);
			if (!fs::exists(folder_path))
			{
				fs::create_directory(folder_path);
			}

			return folder_path.string();
		}

		float round_to(float value, float target)
		{
			float rounded = float(int(round(value / target)) * target);
			return rounded;
		}

		float floor_to(float value, float target)
		{
			float result = float(int(floor(value / target)) * target);
			return result;
		}

		float ceil_to(float value, float target)
		{
			float result = float(int(ceil(value / target)) * target);
			return result;
		}

		int get_digits(double value) {
			std::stringstream ss;
			ss << std::fixed << value; // 使用定点表示法将浮点数转换为字符串
			std::string valueStr = ss.str();

			if (valueStr.find("e-") != std::string::npos) {
				// 处理科学计数法
				size_t pos = valueStr.find("e-");
				return std::stoi(valueStr.substr(pos + 2));
			}
			else if (valueStr.find('.') != std::string::npos) {
				// 处理普通小数
				size_t pos = valueStr.find('.');
				return valueStr.length() - pos - 1;
			}
			else {
				// 无小数点
				return 0;
			}
		}

		float JsonToFloat(const Json& data)
		{
			if (data.is_string())
			{
				return std::atof(AString(data).c_str());
			}
			else if (data.is_number())
			{
				return data.get<float>();
			}
			else
			{
				return 0;
			}
		}

		int JsonToInt(const Json& data)
		{
			if (data.is_string())
			{
				return std::atoi(AString(data).c_str());
			}
			else if (data.is_number())
			{
				return data.get<int>();
			}
			else
			{
				return 0;
			}
		}


		Json load_json(AString filename)
		{
			fs::path filepath = get_file_path(filename);

			if (fs::exists(filepath))
			{
				AString file_str = cFile::ReadWholeFile(filepath.string());
				if (file_str.empty())
				{
					return Json();
				}
				Json data = Json::parse(file_str);
				return data;
			}
			else
			{
				save_json(filename, Json::object());
				return Json::object();
			}
		}

		bool save_json(AString filename, const Json& data)
		{
			AString filepath = get_file_path(filename);
			cFile file(filepath, cFile::fmWrite);
			if (file.IsOpen())
			{
				AString buffer = data.dump(4, ' ', false, Json::error_handler_t::replace);
				file.Write(buffer.data(), buffer.size());

				file.Close();

				return true;
			}

			return false;
		}



		time_point<system_clock, minutes> getMinutes(DateTime dateTime)
		{
			return time_point_cast<minutes>(dateTime);
		}

		time_point<system_clock, hours> getHours(DateTime dateTime)
		{
			return time_point_cast<hours>(dateTime);
		}

		DateTime replaceMinutes(DateTime dateTime)
		{
			return time_point_cast<milliseconds>(time_point_cast<minutes>(dateTime));
		}

		DateTime replaceHours(DateTime dateTime)
		{
			return time_point_cast<milliseconds>(time_point_cast<hours>(dateTime));
		}


		BarGenerator::BarGenerator(FnMut<void(BarData)> on_bar,
			int window, FnMut<void(BarData)> on_window_bar, Interval interval)
			: on_bar(on_bar)
			, window(window)
			, on_window_bar(on_window_bar)
			, interval(interval)
			, interval_count(0)
			, bar()
			, window_bar()
			, last_tick()
			, last_bar()
		{
		}

		void BarGenerator::update_tick(const TickData& tick)
		{
			bool new_minute = false;

			// Filter tick data with 0 last price
			if (!tick.last_price)
				return;

			if (!this->bar)
			{
				new_minute = true;
			}
			else if (getMinutes(this->bar->datetime) != getMinutes(tick.datetime))
			{
				this->bar->datetime = replaceMinutes(this->bar->datetime);
				this->on_bar(this->bar.value());

				new_minute = true;
			}

			if (new_minute)
			{
				BarData newbar;
				newbar.symbol = tick.symbol;
				newbar.exchange = tick.exchange;
				newbar.interval = Interval::MINUTE;
				newbar.datetime = tick.datetime;
				newbar.exchange_name = tick.exchange_name;
				newbar.open_price = tick.last_price;
				newbar.high_price = tick.last_price;
				newbar.low_price = tick.last_price;
				newbar.close_price = tick.last_price;
				newbar.open_interest = tick.open_interest;
				newbar.__post_init__();

				this->bar = newbar;
			}
			else
			{
				this->bar->high_price = std::max(this->bar->high_price, tick.last_price);
				this->bar->low_price = std::min(this->bar->low_price, tick.last_price);
				this->bar->close_price = tick.last_price;
				this->bar->open_interest = tick.open_interest;
				this->bar->datetime = tick.datetime;
			}

			if (this->last_tick)
			{
				float volume_change = tick.volume - this->last_tick->volume;
				this->bar->volume += std::max(volume_change, 0.0f);
			}

			this->last_tick = tick;
		}

		void BarGenerator::update_bar(const BarData& bar)
		{
			// If not inited, creaate window bar object
			if (!this->window_bar)
			{
				DateTime dt;
				// Generate timestamp for bar data
				if (this->interval == Interval::MINUTE)
					dt = replaceMinutes(bar.datetime);
				else
					dt = replaceHours(bar.datetime);

				BarData newbar;

				newbar.symbol = bar.symbol;
				newbar.exchange = bar.exchange;
				newbar.datetime = dt;
				newbar.exchange_name = bar.exchange_name;
				newbar.open_price = bar.open_price;
				newbar.high_price = bar.high_price;
				newbar.low_price = bar.low_price;
				newbar.__post_init__();

				this->window_bar = newbar;
			}
			// Otherwise, update high / low price into window bar
			else
			{
				this->window_bar->high_price = std::max(
					this->window_bar->high_price, bar.high_price);
				this->window_bar->low_price = std::min(
					this->window_bar->low_price, bar.low_price);
			}

			// Update close price / volume into window bar
			this->window_bar->close_price = bar.close_price;
			this->window_bar->volume += int(bar.volume);
			this->window_bar->open_interest = bar.open_interest;

			// Check if window bar completed
			bool finished = false;

			if (this->interval == Interval::MINUTE)
			{
				auto dp = date::floor<date::days>(bar.datetime);
				auto time = date::make_time(duration_cast<milliseconds>(bar.datetime - dp));

				// x - minute bar
				if ((time.minutes().count() + 1) % this->window == 0)
					finished = true;
			}

			else if (this->interval == Interval::HOUR)
			{
				if (this->last_bar && getHours(bar.datetime) != getHours(this->last_bar->datetime))
				{
					// 1 - hour bar
					if (this->window == 1)
						finished = true;
					// x - hour bar
					else
					{
						this->interval_count += 1;

						if (this->interval_count % this->window)
						{
							finished = true;
							this->interval_count = 0;
						}
					}
				}
			}

			if (finished)
			{
				this->on_window_bar(this->window_bar.value());
			}
			this->window_bar = std::nullopt;

			// Cache last bar object
			this->last_bar = bar;
		}

		void BarGenerator::generate()
		{
			this->bar->datetime = getMinutes(this->bar->datetime);
			this->on_bar(this->bar.value());
			this->bar = std::nullopt;
		}
	}
}
