#pragma once

//-----------------------------------------------------------------------------
// Loggery
//-----------------------------------------------------------------------------
// interfejs logera
struct Logger
{
	enum LOG_LEVEL
	{
		L_INFO,
		L_WARN,
		L_ERROR,
		L_FATAL
	};

	virtual ~Logger() {}
	void GetTime(tm& time);

	virtual void Log(cstring text, LOG_LEVEL level) = 0;
	virtual void Log(cstring text, LOG_LEVEL level, const tm& time) = 0;
	virtual void Flush() = 0;

	void Info(cstring text) { Log(text, L_INFO); }
	void Warn(cstring text) { Log(text, L_WARN); }
	void Error(cstring text) { Log(text, L_ERROR); }
	void Fatal(cstring text) { Log(text, L_FATAL); }
};

// pusty loger, nic nie robi
struct NullLogger : public Logger
{
	NullLogger() {}
	void Log(cstring text, LOG_LEVEL level) {}
	void Log(cstring text, LOG_LEVEL, const tm& time) {}
	void Flush() {}
};

// loger do konsoli
struct ConsoleLogger : public Logger
{
	~ConsoleLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush() {}
};

// loger do pliku txt
struct TextLogger : public Logger
{
	std::ofstream out;
	string path;

	explicit TextLogger(cstring filename);
	~TextLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

// loger do kilku innych logerów
struct MultiLogger : public Logger
{
	vector<Logger*> loggers;

	~MultiLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

// loger który przechowuje informacje przed wybraniem okreœlonego logera
struct PreLogger : public Logger
{
private:
	struct Prelog
	{
		string str;
		LOG_LEVEL level;
		tm time;
	};

	vector<Prelog*> prelogs;
	bool flush;

public:
	PreLogger() : flush(false) {}
	void Apply(Logger* logger);
	void Clear();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

extern Logger* logger;

#define LOG(msg) logger->Info(msg)
#define INFO(msg) logger->Info(msg)
#define WARN(msg) logger->Warn(msg)
#define ERROR(msg) logger->Error(msg)
#define FATAL(msg) logger->Fatal(msg)
