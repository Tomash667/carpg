#pragma once

class ContentLoader
{
public:
	bool Load(Tokenizer& t, cstring filename, int top_group, delegate<void(int, const string& id)> action)
	{
		LocalString path = Format("%s/%s", content::system_dir.c_str(), filename);

		if(!t.FromFile(path))
		{
			Error("Failed to open file '%s'.", path.c_str());
			++content::errors;
			return false;
		}

		try
		{
			content::errors += t.ParseTop<int>(top_group, [&](int top)
			{
				if(!t.IsString())
				{
					Error(t.FormatUnexpected(tokenizer::T_STRING));
					return false;
				}
				local_id = t.MustGetString();
				t.Next();

				try
				{
					action(top, local_id);
					return true;
				}
				catch(Tokenizer::Exception& e)
				{
					Error("Failed to parse %s '%s': %s", t.GetKeyword(top_group, top)->name, local_id.c_str(), e.ToString());
					return false;
				}
			});
		}
		catch(Tokenizer::Exception& e)
		{
			Error("Failed to parse file '%s': %s", path.c_str(), e.ToString());
			++content::errors;
		}

		return true;
	}

private:
	string local_id;
};
