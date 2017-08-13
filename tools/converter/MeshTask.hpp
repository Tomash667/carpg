#pragma once

const int QMSH_TMP_VERSION_MIN = 12;
const int QMSH_TMP_VERSION_MAX = 19;
const int QMSH_VERSION = 20;
extern const char* CONVERTER_VERSION;

enum GROUP_OPTION
{
	GO_ONE,
	GO_FILE,
	GO_CREATE
};

struct ConversionData
{
	string input, output, group_file;
	GROUP_OPTION gopt;
	bool export_phy, force_output;
};

void Convert(ConversionData& data);
void Info(const char* path);
