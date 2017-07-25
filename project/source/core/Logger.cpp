#include "Pch.h"
#include "Core.h"

Logger* Logger::global;
const cstring Logger::level_names[4] = {
	"INFO ",
	"WARN ",
	"ERROR",
	"FATAL"
};

void Logger::Log(Level level, cstring msg)
{
	assert(msg);
	time_t t = time(0);
	tm tm;
	localtime_s(&tm, &t);
	Log(level, msg, tm);
}

ConsoleLogger::~ConsoleLogger()
{
	Info("*** End of log.");
}

void ConsoleLogger::Log(Level level, cstring text, const tm& time)
{
	printf("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, level_names[level], text);
	fflush(stdout);
}

TextLogger::TextLogger(cstring filename) : path(filename)
{
	assert(filename);
	out.open(filename);
}

TextLogger::~TextLogger()
{
	Info("*** End of log.");
}

void TextLogger::Log(Level level, cstring text, const tm& time)
{
	out << Format("%02d:%02d:%02d %s - %s\n", time.tm_hour, time.tm_min, time.tm_sec, level_names[level], text);
}

void TextLogger::Flush()
{
	out.flush();
}

MultiLogger::~MultiLogger()
{
	DeleteElements(loggers);
}

void MultiLogger::Log(Level level, cstring text, const tm& time)
{
	for(Logger* logger : loggers)
		logger->Log(level, text, time);
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
		logger->Log(log->level, log->str.c_str(), log->time);

	if(flush)
		logger->Flush();
	DeleteElements(prelogs);
}

void PreLogger::Clear()
{
	DeleteElements(prelogs);
}

void PreLogger::Log(Level level, cstring text, const tm& time)
{
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
