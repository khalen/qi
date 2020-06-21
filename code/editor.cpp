//
// Created by Jonathan Davis on 6/21/20.
//

#include "basictypes.h"
#include "editor.h"
#include "game.h"

const size_t kEditorAdditionalMem = MB(16);

struct Editor
{

};

bool Editor_UpdateAndRender() { return false; }

static Editor* ged = nullptr;

static void Editor_Init(const SubSystem* sys, bool isReInit)
{
	ged = (Editor *)sys->globalPtr;

	if (!isReInit)
	{

	}
}

SubSystem EditorSubSystem = { "Editor", Editor_Init, sizeof(Editor) + kEditorAdditionalMem, nullptr };

