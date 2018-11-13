#pragma once

#include "ConversionData.h"

void Convert(ConversionData& data);
void Info(const char* path);
void Compare(const char* path1, const char* path2);
int Upgrade(const char* path, bool force);
void UpgradeDir(const char* path, bool force, bool subdir);
