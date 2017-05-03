#pragma once

#include "GuiDialog.h"

class Button;
class ListBox;
class TextBox;

namespace gui
{
	class PickFileDialogItem;
	struct PickFileDialogOptions;

	class PickFileDialog : public GuiDialog
	{
	public:
		typedef delegate<void(PickFileDialog*)> Handler;

		static void Show(const PickFileDialogOptions& options);
		static void Destroy();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		bool GetResult() const { return result; }
		const string& GetPath() const { assert(result); return result_path; }
		const string& GetFilename() const { assert(result); return result_filename; }

	private:
		enum EventId
		{
			SelectItem = GuiEvent_Custom,
			Cancel
		};

		struct Filter
		{
			string text;
			vector<string> exts;
		};

		PickFileDialog();
		~PickFileDialog();

		void Setup(const PickFileDialogOptions& options);
		void LoadDir(bool keep_selection);
		void ParseFilters(const string& str);
		bool HandleChangeExtension(int action, int index);
		bool HandleListBoxEvent(int action, int index);
		void PickItem();
		void PickDir(PickFileDialogItem* item);
		void CancelPick();

		static PickFileDialog* self;
		ListBox* list_box, *list_extensions;
		TextBox* tb_path, *tb_filename;
		Button* bt_select, *bt_cancel;
		string root_dir, active_dir, result_filename, result_path;
		vector<Filter> filters;
		Filter* active_filter;
		Handler handler;
		TEX tex_dir;
		bool result;
	};

	struct PickFileDialogOptions
	{
		PickFileDialog::Handler handler;
		string title, filters, root_dir, start_path;
		bool preview;

		void Show() { PickFileDialog::Show(*this); }
	};
}
