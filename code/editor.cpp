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
	char spriteNameTextEditBuf[64];
	bool isEditingCurSpriteName;

	SpriteFrame* curFrame;
	u32 curFrameIdx;
	i32 hoveredFrameIdx;

	r32 atlasImgScale;

	bool editorWindowOpen;
};
static Editor *ged = nullptr;

static void EndEditingCurrentSpriteName()
{
	if (ged->isEditingCurSpriteName)
	{
		ged->isEditingCurSpriteName = false;
		if (strlen(ged->spriteNameTextEditBuf) > 0)
		{
			ged->curSprite->name          = ST_Intern(KS_GetStringTable(), ged->spriteNameTextEditBuf);
			ged->spriteNameTextEditBuf[0] = 0;
		}
	}
}

static void BeginEditingCurrentSpriteName()
{
	if (ged->isEditingCurSpriteName)
		EndEditingCurrentSpriteName();
	ged->isEditingCurSpriteName = true;
	strncpy(ged->spriteNameTextEditBuf, KS_SymbolString(ged->curSprite->name), sizeof(ged->spriteNameTextEditBuf));
}

static void SetCurAtlasItems()
{
	Assert(ged->curAtlasIdx < ged->numAtlases);
	if (ged->curAtlas != &ged->atlases[ged->curAtlasIdx])
		ged->atlasImgScale = 1.0f;
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
	// Atlas combo
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
	ImGui::Separator();

	r32 childHeight = ImGui::GetContentRegionAvail().y * 0.33f;

	// Sprite list
	ImGui::BeginChild("sprites", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, childHeight), true);
	ImGui::Columns(2);
	for (u32 i = 0; i < ged->curAtlas->numSprites; i++)
	{
		Sprite* spr = ged->curAtlas->sprites[i];
		bool isSelected = (i == ged->curSpriteIdx);
		bool gotSelected = false;
		if (isSelected && ged->isEditingCurSpriteName)
		{
			ImGui::PushID(KS_SymbolString(spr->name));
			if (ImGui::InputText("",
								 ged->spriteNameTextEditBuf,
								 sizeof(ged->spriteNameTextEditBuf),
								 ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
				EndEditingCurrentSpriteName();
			ImGui::PopID();
		}
		else if (ImGui::Selectable(KS_SymbolString(spr->name), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			ged->curSpriteIdx = i;
			SetCurAtlasItems();
			EndEditingCurrentSpriteName();

			if (ImGui::IsMouseDoubleClicked(0))
				BeginEditingCurrentSpriteName();
		}
		ImGui::NextColumn();
		ImGui::PushID(i);
		if (ImGui::InputInt2("##size", spr->size.v))
		{
			printf("Recalc sprite %d %d\n", spr->size.x, spr->size.y);
		}
		ImGui::SameLine();
		Color tint((u32)spr->tint);
		if (ImGui::ColorEdit4("tint", tint.f, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
		{
			spr->tint = ColorU(tint);
		}
		ImGui::PopID();
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::SameLine();

	// Sprite Frames
	ImGui::BeginChild("frames", ImVec2(0.0f, childHeight));
	Color tint((u32)ged->curSprite->tint);
	ImVec4 iTint(tint.r, tint.g, tint.b, tint.a);
	ged->hoveredFrameIdx = -1;
	r32 framesVisWidth = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	ImGuiStyle& style = ImGui::GetStyle();
	for (i32 i = 0; i < ged->curSprite->numFrames; i++)
	{
		SpriteFrame* frame = &ged->curSprite->frames[i];
		ImVec2 tluv(frame->topLeftUV.x, frame->topLeftUV.y);
		ImVec2 bruv(frame->bottomRightUV.x, frame->bottomRightUV.y);
		ImVec2 size(ged->curSprite->size.x * 2, ged->curSprite->size.y * 2);
		if (ImGui::ImageButton((ImTextureID)ged->curAtlas->bitmap, size, tluv, bruv, 1, ImVec4(0.0, 0.0, 0.0, 1.0), iTint))
		{
			ged->curFrameIdx = (u32)i;
			SetCurAtlasItems();
		}
		if (ImGui::IsItemHovered())
			ged->hoveredFrameIdx = i;
		r32 lastX = ImGui::GetItemRectMax().x;
		r32 nextX = lastX + style.ItemSpacing.x + size.x;
		if (i < (i32)ged->curSprite->numFrames - 1 && nextX < framesVisWidth)
			ImGui::SameLine();
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Atlas texture
	ImGui::DragFloat("##scale", &ged->atlasImgScale, 1.0f, 1.0f / 256.0f, 256.0f, "%.3f", 2.0f); ImGui::SameLine();
	ImGui::Text("Ctrl-click to zoom in, alt-click to zoom out");
	ImGui::BeginChild("atlas");
	r32 imgW = ged->curAtlas->bitmap->width * ged->atlasImgScale;
	r32 imgH = ged->curAtlas->bitmap->height * ged->atlasImgScale;
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 imgUL = ImGui::GetCursorScreenPos();
	ImGui::Image((ImTextureID)ged->curAtlas->bitmap, ImVec2(imgW, imgH));
	if (ImGui::IsItemClicked())
	{
		if (io.KeyCtrl)
		{
			ged->atlasImgScale = Min(ged->atlasImgScale * 2.0f, 256.0f);
		}
		else if (io.KeyAlt)
		{
			ged->atlasImgScale = Max(ged->atlasImgScale * 0.5f, 1.0f / 256.0f);
		}
		if (fabs(ged->atlasImgScale - 1.0f) < 0.01f)
			ged->atlasImgScale = 1.0f;
	}

	// Draw highlights around current sprites' frames
	ImDrawList* dl = ImGui::GetWindowDrawList();
	for (u32 i = 0; i < ged->curSprite->numFrames; i++)
	{
		u32 color;
		if (i == ged->curFrameIdx)
			color = 0xBFBF2FFF;
		else if (i == ged->hoveredFrameIdx)
			color = 0xCFCF8FFF;
		else
			color = 0x8F8F8FFF;

		SpriteFrame *frame = &ged->curSprite->frames[i];
		ImVec2 mins(imgUL.x + frame->topLeftUV.x * imgW, imgUL.y + frame->topLeftUV.y * imgH);
		ImVec2 maxs(imgUL.x + frame->bottomRightUV.x * imgW, imgUL.y + frame->bottomRightUV.y * imgH);
		dl->AddRect(mins, maxs, color, 0, 0, 1.0f);
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

