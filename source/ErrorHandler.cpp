// error handling, crash reporting, writing stream log
#include "Pch.h"
#include "GameCore.h"
#include "ErrorHandler.h"
#include "Version.h"
#include "Config.h"
#define IN
#define OUT
#include <CrashRpt.h>

//-----------------------------------------------------------------------------
ErrorHandler ErrorHandler::handler;

//=================================================================================================
void DoCrash()
{
	int* z = nullptr;
#pragma warning(push)
#pragma warning(suppress: 6011)
	*z = 13;
#pragma warning(pop)
}

//=================================================================================================
cstring ExceptionTypeToString(int exctype)
{
	switch(exctype)
	{
	case CR_SEH_EXCEPTION:
		return "SEH exception";
	case CR_CPP_TERMINATE_CALL:
		return "C++ terminate() call";
	case CR_CPP_UNEXPECTED_CALL:
		return "C++ unexpected() call";
	case CR_CPP_PURE_CALL:
		return "C++ pure virtual function call";
	case CR_CPP_NEW_OPERATOR_ERROR:
		return "C++ new operator fault";
	case CR_CPP_SECURITY_ERROR:
		return "Buffer overrun error";
	case CR_CPP_INVALID_PARAMETER:
		return "Invalid parameter exception";
	case CR_CPP_SIGABRT:
		return "C++ SIGABRT signal";
	case CR_CPP_SIGFPE:
		return "C++ SIGFPE signal";
	case CR_CPP_SIGILL:
		return "C++ SIGILL signal";
	case CR_CPP_SIGINT:
		return "C++ SIGINT signal";
	case CR_CPP_SIGSEGV:
		return "C++ SIGSEGV signal";
	case CR_CPP_SIGTERM:
		return "C++ SIGTERM signal";
	default:
		return Format("unknown(%d)", exctype);
	}
}

//=================================================================================================
cstring CodeToString(DWORD err)
{
	switch(err)
	{
	case EXCEPTION_ACCESS_VIOLATION:         return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:               return "Breakpoint was encountered";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "Float: Denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "Float: Divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "Float: Inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "Float: Invalid operation";
	case EXCEPTION_FLT_OVERFLOW:             return "Float: Overflow";
	case EXCEPTION_FLT_STACK_CHECK:          return "Float: Stack check";
	case EXCEPTION_FLT_UNDERFLOW:            return "Float: Underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:            return "Page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "Integer: Divide by zero";
	case EXCEPTION_INT_OVERFLOW:             return "Integer: Overflow";
	case EXCEPTION_INVALID_DISPOSITION:      return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:         return "Private Instruction";
	case EXCEPTION_SINGLE_STEP:              return "Single step";
	case EXCEPTION_STACK_OVERFLOW:           return "Stack overflow";
	default:								 return "Unknown exception code";
	}
}

//=================================================================================================
TextLogger* GetTextLogger()
{
	Logger* log = Logger::GetInstance();
	TextLogger* tlog = dynamic_cast<TextLogger*>(log);
	if(tlog)
		return tlog;
	MultiLogger* mlog = dynamic_cast<MultiLogger*>(log);
	if(mlog)
	{
		for(vector<Logger*>::iterator it = mlog->loggers.begin(), end = mlog->loggers.end(); it != end; ++it)
		{
			tlog = dynamic_cast<TextLogger*>(*it);
			if(tlog)
				return tlog;
		}
	}
	return nullptr;
}

//=================================================================================================
int WINAPI OnCrash(CR_CRASH_CALLBACK_INFO* crash_info)
{
	cstring type = ExceptionTypeToString(crash_info->pExceptionInfo->exctype);
	if(crash_info->pExceptionInfo->pexcptrs && crash_info->pExceptionInfo->pexcptrs->ExceptionRecord)
	{
		PEXCEPTION_RECORD ptr = crash_info->pExceptionInfo->pexcptrs->ExceptionRecord;
		Error("Engine: Unhandled exception caught!\nType: %s\nCode: 0x%x (%s)\nFlags: %d\nAddress: 0x%p\n\nPlease report this error.",
			type, ptr->ExceptionCode, CodeToString(ptr->ExceptionCode), ptr->ExceptionFlags, ptr->ExceptionAddress);
	}
	else
		Error("Engine: Unhandled exception caught!\nType: %s", type);
	TextLogger* text_logger = GetTextLogger();
	if(text_logger)
		text_logger->Flush();
	return CR_CB_DODEFAULT;
}

//=================================================================================================
ErrorHandler::ErrorHandler() : crash_mode(CrashMode::Normal)
{
}

//=================================================================================================
void ErrorHandler::RegisterHandler(Config& cfg, const string& log_path)
{
	int version = cfg.GetVersion();

	// crash mode
	if(version == 0)
	{
		int mode;
		Config::GetResult result = cfg.TryGetInt("crash_mode", mode);
		if(result == Config::GET_MISSING)
			mode = 0;
		else if(result == Config::GET_INVALID || mode < 0 || mode > 2)
		{
			mode = 0;
			Error("Settings: Invalid crash mode '%s'.", cfg.GetString("crash_mode").c_str());
		}
		switch(mode)
		{
		default:
		case 0:
			crash_mode = CrashMode::Normal;
			break;
		case 1:
			crash_mode = CrashMode::DataSeg;
			break;
		case 2:
			crash_mode = CrashMode::Full;
			break;
		}
	}
	else
	{
		Config::GetResult result = cfg.TryGetEnum<CrashMode>("crash_mode", crash_mode, {
			{ "none", CrashMode::None },
			{ "normal", CrashMode::Normal },
			{ "dataseg", CrashMode::DataSeg },
			{ "full", CrashMode::Full }
		});
		if(result != Config::GET_OK)
		{
			crash_mode = CrashMode::Normal;
			if(result == Config::GET_INVALID)
				Error("Settings: Invalid crash mode '%s'.", cfg.GetString("crash_mode").c_str());
		}
	}
	Info("Settings: crash_mode = %s", ToString(crash_mode));
	cfg.Add("crash_mode", ToString(crash_mode));

	// crash handler
	if(!IsDebuggerPresent())
	{
		MINIDUMP_TYPE minidump_type;
		switch(crash_mode)
		{
		default:
		case CrashMode::Normal:
			minidump_type = MiniDumpNormal;
			break;
		case CrashMode::DataSeg:
			minidump_type = MiniDumpWithDataSegs;
			break;
		case CrashMode::Full:
			minidump_type = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithFullMemory);
			break;
		}

		CR_INSTALL_INFO info = { 0 };
		info.cb = sizeof(CR_INSTALL_INFO);
		info.pszAppName = "CaRpg";
		info.pszAppVersion = VERSION_STR;
		info.pszUrl = "http://carpg.pl/crashrpt.php";
		info.uPriorities[CR_HTTP] = 1;
		info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
		info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
		info.uMiniDumpType = minidump_type;
		info.dwFlags = CR_INST_ALL_POSSIBLE_HANDLERS | CR_INST_SHOW_ADDITIONAL_INFO_FIELDS | CR_INST_NO_EMAIL_VALIDATION;

		int r = crInstall(&info);
		assert(r == 0);

		r = crSetCrashCallback(OnCrash, nullptr);
		assert(r == 0);

		if(!log_path.empty())
		{
			r = crAddFile2(log_path.c_str(), nullptr, "Log file", CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK | CR_AF_ALLOW_DELETE);
			assert(r == 0);
		}
		r = crAddScreenshot2(CR_AS_MAIN_WINDOW | CR_AS_PROCESS_WINDOWS | CR_AS_USE_JPEG_FORMAT | CR_AS_ALLOW_DELETE, 50);
		assert(r == 0);
	}
}
