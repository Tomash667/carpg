// error handling, crash reporting, writing stream log
#include "Pch.h"
#include "GameCore.h"
#include "ErrorHandler.h"
#include "Engine.h"
#include "Version.h"
#pragma warning(push)
#pragma warning(disable : 4091)
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
	TextLogger* tlog = dynamic_cast<TextLogger*>(Logger::global);
	if(tlog)
		return tlog;
	MultiLogger* mlog = dynamic_cast<MultiLogger*>(Logger::global);
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
ErrorHandler::ErrorHandler() : crash_mode(CrashMode::Normal), stream_log_mode(StreamLogMode::Errors), stream_log_file("log.stream"), current_packet(nullptr),
registered(false)
{
}

//=================================================================================================
long ErrorHandler::HandleCrash(EXCEPTION_POINTERS* exc)
{
	Error("Handling crash. Code: 0x%x\nText: %s\nFlags: %d\nAddress: 0x%p\nMode: %s.", exc->ExceptionRecord->ExceptionCode,
		CodeToString(exc->ExceptionRecord->ExceptionCode), exc->ExceptionRecord->ExceptionFlags, exc->ExceptionRecord->ExceptionAddress,
		ToString(crash_mode));

	// if disabled simply return
	if(crash_mode == CrashMode::None)
	{
		Error("Crash mode set to none.");
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// create directory for minidumps/logs
	CreateDirectory("crashes", nullptr);

	// prepare string with datetime
	time_t t = time(0);
	tm ct;
	localtime_s(&ct, &t);
	cstring str_time = Format("%04d%02d%02d%02d%02d%02d", ct.tm_year + 1900, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec);

	if(!IsDebuggerPresent())
	{
		// create file for minidump
		HANDLE hDumpFile = CreateFile(Format("crashes/crash%s.dmp", str_time), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0,
			CREATE_ALWAYS, 0, 0);
		if(hDumpFile == INVALID_HANDLE_VALUE)
			hDumpFile = CreateFile(Format("crash%s.dmp", str_time), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

		// save minidump
		if(hDumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION ExpParam;
			ExpParam.ThreadId = GetCurrentThreadId();
			ExpParam.ExceptionPointers = exc;
			ExpParam.ClientPointers = TRUE;

			// set type
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

			// write dump
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, minidump_type, &ExpParam, nullptr, nullptr);
			CloseHandle(hDumpFile);
		}
		else
			Error("Failed to save minidump (%d).", GetLastError());
	}
	else
		Warn("Debugger is present, not creating minidump.");

	// copy log
	TextLogger* tlog = GetTextLogger();
	if(tlog)
	{
		tlog->Flush();
		CopyFile(tlog->GetPath().c_str(), Format("crashes/crash%s.txt", str_time), FALSE);
	}

	// copy stream
	if(stream_log.IsOpen())
	{
		if(current_packet)
			StreamEnd(false);
		stream_log.Flush();
		if(stream_log.GetSize() > 0)
			CopyFile(stream_log_file.c_str(), Format("crashes/crash%s.stream", str_time), FALSE);
	}

	// show error message
	cstring msg = Format("Engine: Unhandled exception caught!\nCode: 0x%x\nText: %s\nFlags: %d\nAddress: 0x%p\n\nPlease report this error.",
		exc->ExceptionRecord->ExceptionCode, CodeToString(exc->ExceptionRecord->ExceptionCode), exc->ExceptionRecord->ExceptionFlags,
		exc->ExceptionRecord->ExceptionAddress);
	Engine::Get().ShowError(msg);

	return EXCEPTION_EXECUTE_HANDLER;
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

	// stream log mode
	Config::GetResult result = cfg.TryGetEnum<StreamLogMode>("stream_log_mode", stream_log_mode, {
		{ "none", StreamLogMode::None },
		{ "errors", StreamLogMode::Errors },
		{ "full", StreamLogMode::Full }
	});
	if(result != Config::GET_OK)
	{
		stream_log_mode = StreamLogMode::Errors;
		if(result == Config::GET_INVALID)
			Error("Settings: Invalid stream log mode '%s'.", cfg.GetString("stream_log_mode").c_str());
	}
	Info("Settings: stream_log_mode = %s", ToString(stream_log_mode));

	// stream log file
	stream_log_file = cfg.GetString("stream_log_file", "log.stream");
	if(!stream_log.Open(stream_log_file.c_str()))
	{
		DWORD error = GetLastError();
		if(stream_log_file == "log.stream")
			Error("Failed to open 'log.stream', error %u.", error);
		else
		{
			Error("Invalid stream log filename '%s'. Using default.", stream_log_file.c_str());
			stream_log_file = "log.stream";
			if(!stream_log.Open(stream_log_file.c_str()))
			{
				DWORD error = GetLastError();
				Error("Failed to open 'log.stream', error %u.", error);
			}
		}
	}

	// update settings
	cfg.Add("crash_mode", ToString(crash_mode));
	cfg.Add("stream_log_mode", ToString(stream_log_mode));
	cfg.Add("stream_log_file", stream_log_file.c_str());

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

		int r = crInstall(&info);
		assert(r == 0);

		r = crSetCrashCallback(OnCrash, nullptr);
		assert(r == 0);

		if(!log_path.empty())
		{
			r = crAddFile2(log_path.c_str(), nullptr, "Log file", CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK | CR_AF_ALLOW_DELETE);
			assert(r == 0);
		}
		if(stream_log_mode != StreamLogMode::None)
		{
			r = crAddFile2(stream_log_file.c_str(), nullptr, "Multiplayer log", CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK | CR_AF_ALLOW_DELETE);
			assert(r == 0);
		}
		r = crAddScreenshot2(CR_AS_MAIN_WINDOW | CR_AS_PROCESS_WINDOWS | CR_AS_USE_JPEG_FORMAT | CR_AS_ALLOW_DELETE, 50);
		assert(r == 0);

		registered = true;
	}
}

//=================================================================================================
void ErrorHandler::UnregisterHandler()
{
	if(registered)
		crUninstall();
}

//=================================================================================================
void ErrorHandler::StreamStart(Packet* packet, int type)
{
	assert(packet);
	assert(!current_packet);

	current_packet = packet;
	current_stream_type = type;
}

//=================================================================================================
void ErrorHandler::StreamEnd(bool ok)
{
	if(!stream_log.IsOpen() && (stream_log_mode == StreamLogMode::None || (stream_log_mode == StreamLogMode::Errors && ok)))
	{
		current_packet = nullptr;
		return;
	}

	assert(current_packet);

	stream_log.Write<byte>(0xFF);
	stream_log.Write<byte>(ok ? 0 : 1);
	stream_log.Write<byte>(current_stream_type);
	stream_log.Write(current_packet->systemAddress.address);
	stream_log.Write(current_packet->length);
	stream_log.Write(current_packet->data, current_packet->length);

	if(!write_packets.empty())
	{
		for(WritePacket* packet : write_packets)
		{
			stream_log.Write<byte>(0xFF);
			stream_log.Write<byte>(2);
			stream_log.Write<byte>(packet->type);
			stream_log.Write(packet->adr.address);
			stream_log.Write(packet->size);
			stream_log.Write(packet->data.data(), packet->size);
			packet->Free();
		}
		write_packets.clear();
	}

	current_packet = nullptr;
}

//=================================================================================================
void ErrorHandler::StreamWrite(const void* data, uint size, int type, const SystemAddress& adr)
{
	assert(data && size > 0);

	if (!stream_log.IsOpen() || stream_log_mode != StreamLogMode::Full)
		return;

	if(current_packet)
	{
		WritePacket* packet = WritePacket::Get();
		packet->data.resize(size);
		memcpy(packet->data.data(), data, size);
		packet->size = size;
		packet->type = type;
		packet->adr = adr;
		write_packets.push_back(packet);
	}
	else
	{
		stream_log.Write<byte>(0xFF);
		stream_log.Write<byte>(2);
		stream_log.Write<byte>(type);
		stream_log.Write(adr.address);
		stream_log.Write(size);
		stream_log.Write(data, size);
	}
}
