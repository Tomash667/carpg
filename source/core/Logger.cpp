#include "Pch.h"
#include "Base.h"

static cstring log_level_name[4] = {
	"INFO ",
	"WARN ",
	"ERROR",
	"FATAL"
};

void Logger::GetTime(tm& out)
{
	time_t t = time(0);
	localtime_s(&out, &t);
}

ConsoleLogger::~ConsoleLogger()
{
	Log("*** End of log.", L_INFO);
}

void ConsoleLogger::Log(cstring text, LOG_LEVEL level)
{
	assert(text);

	tm time;
	GetTime(time);

	printf("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, log_level_name[level], text);
	fflush(stdout);
}

void ConsoleLogger::Log(cstring text, LOG_LEVEL level, const tm& time)
{
	assert(text);

	printf("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, log_level_name[level], text);
	fflush(stdout);
}

TextLogger::TextLogger(cstring filename) : path(filename)
{
	assert(filename);
	out.open(filename);
}

TextLogger::~TextLogger()
{
	Log("*** End of log.", L_INFO);
}

void TextLogger::Log(cstring text, LOG_LEVEL level)
{
	assert(text);

	tm time;
	GetTime(time);

	out << Format("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, log_level_name[level], text);
}

void TextLogger::Log(cstring text, LOG_LEVEL level, const tm& time)
{
	assert(text);

	out << Format("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, log_level_name[level], text);
}

void TextLogger::Flush()
{
	out.flush();
}

MultiLogger::~MultiLogger()
{
	DeleteElements(loggers);
}

void MultiLogger::Log(cstring text, LOG_LEVEL level)
{
	assert(text);

	for(Logger* logger : loggers)
		logger->Log(text, level);
}

void MultiLogger::Log(cstring text, LOG_LEVEL level, const tm& time)
{
	assert(text);

	for(Logger* logger : loggers)
		logger->Log(text, level, time);
}

void MultiLogger::Flush()
{
	for(Logger* logger : loggers)
		logger->Flush();
}

void PreLogger::Apply(Logger* logger)
{
	assert(logger);

	for(Prelog* log : prelogs)
		logger->Log(log->str.c_str(), log->level, log->time);

	if(flush)
		logger->Flush();
	DeleteElements(prelogs);
}

void PreLogger::Clear()
{
	DeleteElements(prelogs);
}

void PreLogger::Log(cstring text, LOG_LEVEL level)
{
	assert(text);

	Prelog* p = new Prelog;
	p->str = text;
	p->level = level;
	GetTime(p->time);

	prelogs.push_back(p);
}

void PreLogger::Log(cstring text, LOG_LEVEL level, const tm& time)
{
	assert(text);

	Prelog* p = new Prelog;
	p->str = text;
	p->level = level;
	p->time = time;

	prelogs.push_back(p);
}

void PreLogger::Flush()
{
	flush = true;
}
