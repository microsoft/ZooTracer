#pragma once
#include <string>
#include <iostream>

namespace zt{

	/// simple message logger.
	///<remarks>See "How to: Marshal Callbacks and Delegates By Using C++ Interop" http://msdn.microsoft.com/en-us/library/367eeye0.aspx </remarks>
	class Log{
	public:
		using Logger = void(__stdcall *)(std::string);
		static void write(std::string line){
			Logger logger = instance()._logger;
			if (logger == nullptr) std::cerr << line << std::endl;
			else logger(line);
		}
		static void set(Logger logger){ instance()._logger = logger; }
	private:
		static Log& instance(){
			static Log single;
			return single;
		}
		Log() :_logger(nullptr){}
		Logger _logger;
	};
}