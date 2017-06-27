#include "Stdafx.h"
#include <ztLog.h>
#include <msclr/marshal_cppstd.h>

namespace zt{

	using System::Runtime::InteropServices::Marshal;
	// see "How to: Marshal Callbacks and Delegates By Using C++ Interop" http://msdn.microsoft.com/en-us/library/367eeye0.aspx
	delegate void LoggerDelegate(std::string);
	public ref class WpfLog{
	public:
		WpfLog(System::Action<System::String^>^ logger){
			_managed_logger = logger;
			// prevents garbage collection of the delegate
			_unmanaged_logger = gcnew LoggerDelegate(this, &WpfLog::_logger);
			auto logger_ptr = Marshal::GetFunctionPointerForDelegate(_unmanaged_logger);
			Log::set(static_cast<Log::Logger>(logger_ptr.ToPointer()));
		}
		~WpfLog(){ Log::set(nullptr); }
	private:
		LoggerDelegate^ _unmanaged_logger;
		System::Action<System::String^>^ _managed_logger;
		void _logger(std::string line){ _managed_logger(msclr::interop::marshal_as<System::String^>(line)); }
	};
}