#pragma once

#include "ContentLoader.h"

enum IfState;

class DialogLoader : public ContentLoader
{
public:
	void InitTokenizer() override;
	void DoLoading();
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;

private:
	void LoadDialog(const string& id);
	void LoadGlobals();

	vector<IfState> if_state;
};
