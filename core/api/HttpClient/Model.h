#pragma once

#include <sstream>

namespace Keen
{
	namespace api
	{
		class Request;
		using Headers = std::map<std::string, std::string>;
		using Params = std::map<std::string, std::string>;
		using RequestData = std::variant<Json, AString>;

		inline AString BuildHeaders(const Headers& headers)
		{
			AString str;
			bool first = true;
			for (auto i = headers.begin(); i != headers.end(); i++)
			{
				if (first)
				{
					first = false;
				}
				else
				{
					str += "&";
				}
				str += i->first;
				str += "=";
				str += i->second;
			}
			return str;
		}

		inline AString BuildParams(const Params& m)
		{
			AString str;
			bool first = true;
			for (auto i = m.begin(); i != m.end(); i++)
			{
				if (first)
				{
					/// str += "?";
					first = false;
				}
				else
				{
					str += "&";
				}
				str += i->first;
				str += "=";
				str += i->second;
			}
			return str;
		}

		inline AString RequestDataToString(const RequestData& data)
		{
			AString request_data;

			if (std::holds_alternative<Json>(data))
			{
				request_data = std::get<Json>(data).dump();
			}
			else if (std::holds_alternative<AString>(data))
			{
				request_data = std::get<AString>(data);
			}
			/*else if (std::holds_alternative<Byte*>(request.data))
			{
				request_data.append((char*)std::get<Byte*>(request.data));
			}*/

			return request_data;
		}

		class Response
		{
		public:
			std::string version;
			int code = 0;
			int status = -1;
			std::string reason;
			Headers headers;
			std::string body;
			std::string location; // Redirect location
		};

		class Request
		{
		public:
			AString __str__() const
			{
				int status_code;
				if (!this->response)
					status_code = 0;
				else
					status_code = this->response->status;

				std::stringstream ss;
				ss << "request: " << this->method << " " << this->path << " because " << status_code << ": \n"
					<< "headers: " << BuildHeaders(this->headers) << "\n"
					<< "params: " << BuildParams(this->params) << "\n"
					<< "data: " << RequestDataToString(this->data) << "\n"
					<< "response: " << (this->response ? this->response->body : "") << "\n";
				return ss.str();
			}

		public:
			AString method;
			AString path;
			Params params;
			Headers headers;
			RequestData data;

			std::any extra;

			Response* response = nullptr;
		};

		class Process
		{
		public:
			Process(UInt64 a, UInt64 b) : _a(a), _b(b)
			{
			}
			Process() : _a(0), _b(0)
			{
			}
			AString serialize()
			{
				std::ostringstream oss;
				oss << _a << ' ' << _b;
				return oss.str();
			}
			void deserialize(const AString& bytes)
			{
				std::istringstream iss(bytes);
				iss >> _a >> _b;
			}
			const UInt64& a() const
			{
				return _a;
			}
			const UInt64& b() const
			{
				return _b;
			}

		private:
			UInt64 _a;
			UInt64 _b;
		};

		class ResponseData
		{
		public:
			ResponseData(const AString& data) : _data(data)
			{
			}
			virtual void read() = 0;

		public:
			AString _data;
		};

		class ResString : public ResponseData 
		{
		public:
			ResString(const AString& data) 
				: ResponseData(data) { }

			virtual void read() override {

			}

			const AString& serialize() const {
				return _data;
			}
		};

		class ResJson : public ResponseData
		{
		public:
			ResJson(const AString& data)
				: ResponseData(data) { }

			virtual void read() override {

			}

			const Json serialize() const {
				return Json::parse(_data);
			}
		};

		class Error
		{
		public:
			Error(int code, int status, const AString& errorData) : _code(code), _status(status), _errorData(errorData)
			{
			}

			Error(const AString& bytes)
			{
				std::istringstream iss(bytes);
				iss >> _code >> _status;
				std::getline(iss >> std::ws, _errorData);
			}

			Error()
			{
			}

			AString serialize()
			{
				std::ostringstream oss;
				oss << _code << ' ' << _status << ' ' << _errorData;
				return oss.str();
			}

			static Error deserialize(const AString& bytes)
			{
				return Error(bytes);
			}

			const int& code() const
			{
				return _code;
			}

			const int& status() const
			{
				return _status;
			}

			const AString& errorData() const
			{
				return _errorData;
			}

		private:
			int _code;
			int _status;
			AString _errorData;
		};
	}
}
