//
// Created by Jonathan Davis on 6/21/20.
//

#include "basictypes.h"
#include "editor.h"
#include "memory.h"
#include "keystore.h"
#include "game.h"
#include "imgui.h"
#include "util.h"

const size_t kEditorAdditionalMem = MB(32);
const size_t kMaxEditorAtlases = 16;

struct Editor
{
	SpriteAtlas atlases[kMaxEditorAtlases];
	u32 numAtlases;
	BuddyAllocator* alloc;

	SpriteAtlas* curAtlas;
	u32 curAtlasIdx;
	Sprite* curSprite;
	u32 curSpriteIdx;
	SpriteFrame* curFrame;
	u32 curFrameIdx;

	bool editorWindowOpen;
};
static Editor *ged = nullptr;

static void SetCurAtlasItems()
{
	Assert(ged->curAtlasIdx < ged->numAtlases);
	ged->curAtlas = &ged->atlases[ged->curAtlasIdx];

	Assert(ged->curSpriteIdx < ged->curAtlas->numSprites);
	ged->curSprite = ged->curAtlas->sprites[ged->curSpriteIdx];

	Assert(ged->curFrameIdx < ged->curSprite->numFrames);
	ged->curFrame = &ged->curSprite->frames[ged->curFrameIdx];
}

static void CopyAtlasesFromGame()
{
	SpriteAtlas* gameAtlases = Game_GetAtlases(&ged->numAtlases);
	for (u32 i = 0; i < ged->numAtlases; i++)
	{
		ged->atlases[i] = gameAtlases[i];
	}

	ged->curFrameIdx = ged->curSpriteIdx = ged->curAtlasIdx = 0;
	SetCurAtlasItems();
}

void ShowSpritesTab()
{
	if (ImGui::BeginCombo("Atlas", VS("%s (%s)", KS_SymbolString(ged->curAtlas->name), ged->curAtlas->imageFile), 0))
	{
		for (u32 i = 0; i < ged->numAtlases; i++)
		{
			if (ImGui::Selectable(VS("%s (%s)", KS_SymbolString(ged->atlases[i].name), ged->atlases[i].imageFile), i == ged->curAtlasIdx))
			{
				ged->curAtlasIdx = i;
				SetCurAtlasItems();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::BeginChild("sprites", ImVec2(150, 0), true);
	for (u32 i = 0; i < ged->curAtlas->numSprites; i++)
	{
		Sprite* spr = ged->curAtlas->sprites[i];
		if (ImGui::Selectable(KS_SymbolString(spr->name), i == ged->curSpriteIdx))
		{
			ged->curSpriteIdx = i;
			SetCurAtlasItems();
		}
		ImGui::SameLine();
		if (ImGui::InputInt2("size", spr->size.v))
		{
			printf("Recalc sprite %d %d\n", spr->size.x, spr->size.y);
		}
		ImGui::SameLine();
		Color tint((u32)spr->tint);
		if (ImGui::ColorEdit4("tint", tint.f, 0))
		{
			spr->tint = ColorU(tint);
		}
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("frames");
	Color tint((u32)ged->curSprite->tint);
	ImVec4 iTint(tint.r, tint.g, tint.b, tint.a);
	for (u32 i = 0; i < ged->curSprite->numFrames; i++)
	{
		SpriteFrame* frame = &ged->curSprite->frames[i];
		ImVec2 tluv(frame->topLeftUV.x, frame->topLeftUV.y);
		ImVec2 bruv(frame->bottomRightUV.x, frame->bottomRightUV.y);
		ImGui::Image((ImTextureID)ged->curAtlas->bitmap, ImVec2(60, 60), tluv, bruv, iTint, ImVec4(1.0, 1.0, 0.0, 1.0));
	}
	ImGui::EndChild();
}

void ShowOtherTab()
{
	ImGui::Text("This is the other tab.");
}

bool Editor_UpdateAndRender()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.40, io.DisplaySize.y));
	if (!ImGui::Begin("Editor", &ged->editorWindowOpen, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImGui::End();
		return false;
	}
	if (ImGui::BeginTabBar("##MainTabBar"))
	{
		if (ImGui::BeginTabItem("Sprites"))
		{
			ShowSpritesTab();
			ImGui::EndTabItem();
		}
		if( (ImGui::BeginTabItem("Other")))
		{
			ShowOtherTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	return false;
}

static void Editor_Init(const SubSystem* sys, bool isReInit)
{
	ged = (Editor *)sys->globalPtr;

	if (!isReInit)
	{
		CopyAtlasesFromGame();
		ged->editorWindowOpen = true;
		ged->alloc = BA_InitBuffer((u8 *)(ged + 1), kEditorAdditionalMem, 32);
	}
}

SubSystem EditorSubSystem = { "Editor", Editor_Init, sizeof(Editor) + kEditorAdditionalMem, nullptr };

