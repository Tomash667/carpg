#pragma once

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
