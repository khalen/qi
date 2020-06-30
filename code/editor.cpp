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
const size_t kEditorSpriteArenaSize = MB(4);
const size_t kFrameEditBufferCount = 32;

struct Editor
{
	SpriteAtlasTable atlases;
	MemoryArena editorSpriteArena;

	SpriteAtlas* curAtlas;
	i32 curAtlasIdx;

	Sprite* curSprite;
	i32 curSpriteIdx;
	char spriteNameTextEditBuf[64];
	bool isEditingCurSpriteName;

	SpriteFrame* curFrame;
	i32 curFrameIdx;
	i32 hoveredFrameIdx;

	r32 atlasImgScale;

	bool editorWindowOpen;

	bool isEditingCurSpriteFrames;
	SpriteFrame frameEditBuffer[kFrameEditBufferCount];
	u32 numEditFrames;
};
static Editor *ged = nullptr;

void SetCurAtlasItems();

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

static void EndEditingCurrentSpriteFrames()
{
	if (ged->isEditingCurSpriteFrames)
	{

	}
}

static void BeginEditingCurrentSpriteFrames()
{

}

void CopyAtlasesFromGame();
static void SetCurAtlasItems()
{
	Assert(ged->curAtlasIdx < ged->atlases.numAtlases);
	if (ged->curAtlas != &ged->atlases.atlases[ged->curAtlasIdx])
	{
		EndEditingCurrentSpriteName();
		ged->atlasImgScale = 1.0f;
		ged->curSpriteIdx = 0;
		ged->curFrameIdx  = 0;
		ged->hoveredFrameIdx = -1;
	}
	ged->curAtlas = &ged->atlases.atlases[ged->curAtlasIdx];

	Assert(ged->curSpriteIdx < ged->curAtlas->numSprites);
	Sprite* editSprite = ged->curAtlas->sprites[ged->curSpriteIdx];
	if (editSprite != ged->curSprite)
	{
#if 0
		// If we have moved this sprites' frames to the edit buffer, recopy it to the game and back to reset for editing the new sprite
		if (ged->curSprite->frames == ged->frameEditBuffer)
		{
			Game_CopyAtlases(&ged->atlases);
			CopyAtlasesFromGame();
		}
#endif
		ged->curSprite = ged->curAtlas->sprites[ged->curSpriteIdx];
		ged->curFrameIdx = 0;
	}

	Assert(ged->curFrameIdx < ged->curSprite->numFrames);
	ged->curFrame = &ged->curSprite->frames[ged->curFrameIdx];
}

static void CopyAtlasesFromGame()
{
	MA_Reset(&ged->editorSpriteArena);
	Game_CopyAtlasTableUsingArena(&ged->editorSpriteArena, &ged->atlases, Game_GetAtlasTable());

	ged->curFrameIdx = ged->curSpriteIdx = ged->curAtlasIdx = 0;
	SetCurAtlasItems();
}

void ShowSpritesTab()
{
	// Atlas combo
	if (ImGui::BeginCombo("Atlas", VS("%s (%s)", KS_SymbolString(ged->curAtlas->name), ged->curAtlas->imageFile), 0))
	{
		for (u32 i = 0; i < ged->atlases.numAtlases; i++)
		{
			if (ImGui::Selectable(VS("%s (%s)", KS_SymbolString(ged->atlases.atlases[i].name), ged->atlases.atlases[i].imageFile), i == ged->curAtlasIdx))
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
		ImGui::BeginGroup();
			ImGui::PushID(i);
			SpriteFrame *frame = &ged->curSprite->frames[i];
			ImVec2       tluv(frame->topLeftUV.x, frame->topLeftUV.y);
			ImVec2       bruv(frame->bottomRightUV.x, frame->bottomRightUV.y);
			ImVec2       size(ged->curSprite->size.x * 2, ged->curSprite->size.y * 2);
			if (ImGui::ImageButton((ImTextureID)ged->curAtlas->bitmap, size, tluv, bruv, -1, ImVec4(0.0, 0.0, 0.0, 1.0), iTint))
			{
				ged->curFrameIdx = (u32)i;
				SetCurAtlasItems();
			}
			else if (ImGui::IsItemHovered())
				ged->hoveredFrameIdx = i;
			r32 lastX = ImGui::GetItemRectMax().x;
			r32 nextX = lastX + style.ItemSpacing.x + size.x;
			ImGui::PushItemWidth(nextX - lastX);
			if (ImGui::InputFloat("##weight", &frame->weight, 0.0f, 0.0f, "%2.2f"))
			{
				printf("Weight change");
			}
			ImGui::PopItemWidth();
			ImGui::PopID();
		ImGui::EndGroup();
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
			color = 0xFFBFBF2F;
		else if (i == ged->hoveredFrameIdx)
			color = 0xFFCFCF8F;
		else
			color = 0xFF8F8F8F;

		SpriteFrame *frame = &ged->curSprite->frames[i];
		ImVec2 mins(imgUL.x + frame->topLeftUV.x * imgW, imgUL.y + frame->topLeftUV.y * imgH);
		ImVec2 maxs(imgUL.x + frame->bottomRightUV.x * imgW, imgUL.y + frame->bottomRightUV.y * imgH);
		dl->AddRect(mins, maxs, color, 0, 0, 1.0f);
	}

	if (ImGui::IsItemHovered())
	{
		ImVec2 mouseScreenPos = ImGui::GetMousePos();
		ImVec2 mouseImagePos = ImVec2(mouseScreenPos.x - imgUL.x, mouseScreenPos.y - imgUL.y);
		r32 cellX = ged->curAtlas->baseSize.x * ged->atlasImgScale;
		r32 cellY = ged->curAtlas->baseSize.y * ged->atlasImgScale;
		ImVec2 rectUL(imgUL.x + floor(mouseImagePos.x / cellX) * cellX, imgUL.y + floor(mouseImagePos.y / cellY) * cellY);
		ImVec2 rectBR(rectUL.x + cellX, rectUL.y + cellY);
		dl->AddRect(rectUL, rectBR, 0xFF00FF00, 0, 0, 1.0f);
	}
	ImGui::EndChild();

	// Highlight cell under cursor

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
		MA_InitBuffer(&ged->editorSpriteArena, ged + 1, kEditorSpriteArenaSize);
		CopyAtlasesFromGame();
		ged->editorWindowOpen = true;
	}
}

SubSystem EditorSubSystem = { "Editor", Editor_Init, sizeof(Editor) + kEditorAdditionalMem, nullptr };
