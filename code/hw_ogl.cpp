//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Platform portable OGL routines. TODO: Should we link this with both the
// gameplay dll and the platform exe, or expose through the game dll interface?
//

#include "basictypes.h"

#include "hw_ogl.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include "imgui.h"
#include "bitmap.h"
#include "hwi.h"
#include <new>

#define FR() ((float)rand()) / (float)RAND_MAX

const u32    kMaxTextures               = 128;
const size_t kOglHardwareMemSize        = 1024 * 1024;
const size_t kMaxRenderBitmapStackDepth = 32;

struct OglBitmap
{
	Bitmap *bitmap;
	GLuint  texture;
	GLuint  fbo;
	u32     textureIdx;
};

enum StateDirtyBits
{
	QOS_FrameBuffer = 1 << 0,
	QOS_Blend       = 1 << 1,
};

struct OglHwi;

struct OglGlobals
{
	// Blit game framebuffer
	GLuint blitTexture;
	GLuint blitProgram;

	GLuint quadVao;
	GLuint quadVbo;

	GLint samplerUniformLocation;

	// ImGui draw support
	GLuint fontTexture;
	GLuint drawUIProgram, drawUIVertexProgram, drawUIFragmentProgram;
	i32    uiTexLocation, uiProjMtxLocation;
	i32    uiVtxPosLocation, uiVtxUVLocation, uiVtxColorLocation;
	GLuint uiVbo, uiElements;

	OglBitmap textures[kMaxTextures];
	u32       numTextures;

	Bitmap *renderBitmapStack[kMaxRenderBitmapStackDepth];
	i32     renderBitmapStackPos;

	u32 stateDirty;
	i32 inBeginFrame;

	Bitmap *screenBitmap;
};

static OglGlobals *gOgl;

static void CheckGl()
{
	GLenum err;
	bool   errored = false;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		errored = true;
		fprintf(stderr, "GL Error: 0x%x\n", err);
	}

	Assert(!errored);
}

// TODO: Change shader load to use platform functionality once we have proper files / asset system
internal char *GetShaderText(const char *fileName)
{
	struct stat statBuf;
	if (stat(fileName, &statBuf) != 0)
	{
		Assert(0 && "Couldn't stat a shader file");
	}

	char *shaderBuf = (char *)malloc(statBuf.st_size + 1);
	memset(shaderBuf, 0, statBuf.st_size + 1);
	Assert(shaderBuf);

#pragma warning(disable : 4996)
	FILE *f = fopen(fileName, "rb");
	fread(shaderBuf, 1, statBuf.st_size, f);
	fclose(f);

	return shaderBuf;
}

internal void FreeShaderText(void *text)
{
	free(text);
}

static GLuint LoadGlslShader(GLenum shaderType, const char *shaderFile)
{
	GLuint shader       = glCreateShader(shaderType);
	char * shaderSource = GetShaderText(shaderFile);
	int    sourceLen    = -1;

	glShaderSource(shader, 1, &shaderSource, &sourceLen);
	glCompileShader(shader);
	FreeShaderText(shaderSource);

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus != GL_TRUE)
	{
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		char *log = (char *)malloc(logLen + 1);
		glGetShaderInfoLog(shader, logLen + 1, nullptr, log);
		printf("Shader compile failed! Log follows:\n");
		printf("%s", log);
		free(log);
	}

	return shader;
}

void QiOgl_Init()
{
	if (!gladLoadGL())
	{
		Assert(0 && "Couldn't init OpenGL");
	}

	// Set up screen blit
	glGenTextures(1, &gOgl->blitTexture);
	glBindTexture(GL_TEXTURE_2D, gOgl->blitTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Create program for blit
	GLuint vs = LoadGlslShader(GL_VERTEX_SHADER, "blit_vs.glsl");
	GLuint fs = LoadGlslShader(GL_FRAGMENT_SHADER, "blit_fs.glsl");

	gOgl->blitProgram = glCreateProgram();
	glAttachShader(gOgl->blitProgram, vs);
	glAttachShader(gOgl->blitProgram, fs);
	glLinkProgram(gOgl->blitProgram);

	GLint status;
	glGetProgramiv(gOgl->blitProgram, GL_LINK_STATUS, &status);
	Assert(status == GL_TRUE);

	glUseProgram(gOgl->blitProgram);
	gOgl->samplerUniformLocation = glGetUniformLocation(gOgl->blitProgram, "tex");

	// Create program for imgui
	vs = LoadGlslShader(GL_VERTEX_SHADER, "imgui_vs.glsl");
	fs = LoadGlslShader(GL_FRAGMENT_SHADER, "imgui_fs.glsl");

	gOgl->drawUIProgram = glCreateProgram();
	glAttachShader(gOgl->drawUIProgram, vs);
	glAttachShader(gOgl->drawUIProgram, fs);
	glLinkProgram(gOgl->drawUIProgram);

	glGetProgramiv(gOgl->drawUIProgram, GL_LINK_STATUS, &status);
	Assert(status == GL_TRUE);

	glUseProgram(gOgl->drawUIProgram);
	gOgl->uiTexLocation      = glGetUniformLocation(gOgl->drawUIProgram, "tex");
	gOgl->uiProjMtxLocation  = glGetUniformLocation(gOgl->drawUIProgram, "projMtx");
	gOgl->uiVtxPosLocation   = glGetAttribLocation(gOgl->drawUIProgram, "pos");
	gOgl->uiVtxUVLocation    = glGetAttribLocation(gOgl->drawUIProgram, "uv");
	gOgl->uiVtxColorLocation = glGetAttribLocation(gOgl->drawUIProgram, "color");

	glGenBuffers(1, &gOgl->uiVbo);
	glGenBuffers(1, &gOgl->uiElements);

	glGenVertexArrays(1, &gOgl->quadVao);
	glGenBuffers(1, &gOgl->quadVbo);

	void QiOgl_CreateFontsTexture();
	QiOgl_CreateFontsTexture();
}

static void QiOgl_InitBlitBufferState()
{
	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	};

	glDisable(GL_SCISSOR_TEST);
	glBindVertexArray(gOgl->quadVao);
	glBindBuffer(GL_ARRAY_BUFFER, gOgl->quadVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void *)(sizeof(GLfloat) * 3));
}

static void IGC_SetRenderBitmap(const ImDrawList *, const ImDrawCmd *cmd)
{
	static GLenum drawNone[] = {GL_NONE};

#if 1
	if (cmd->UserCallbackData == nullptr)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CheckGl();
		return;
	}

	Bitmap *renderBitmap = (Bitmap *)cmd->UserCallbackData;
	Assert(renderBitmap->hardwareId != nullptr);
	OglBitmap *oglBitmap = (OglBitmap *)renderBitmap->hardwareId;
	Assert(oglBitmap->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, oglBitmap->fbo);
	CheckGl();
#endif
}

static void IGC_ClearCurrentTarget(const ImDrawList *, const ImDrawCmd* cmd)
{
#if 1
	Color clearColor((u32)(uintptr_t)cmd->UserCallbackData);

	glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

static void QiOgl_PushRenderBitmap(Bitmap *renderBitmap)
{
	Assert(gOgl->renderBitmapStackPos < kMaxRenderBitmapStackDepth - 1);
	gOgl->renderBitmapStack[gOgl->renderBitmapStackPos++] = renderBitmap;

	ImDrawList *dl = ImGui::GetBackgroundDrawList();
	Assert(dl);
	dl->AddCallback(IGC_SetRenderBitmap, renderBitmap);
}

static void QiOgl_PopRenderBitmap()
{
	Assert(gOgl->renderBitmapStackPos > 0);
	Bitmap *renderBitmap                                = gOgl->renderBitmapStack[--gOgl->renderBitmapStackPos];
	gOgl->renderBitmapStack[gOgl->renderBitmapStackPos] = nullptr;

	ImDrawList *dl = ImGui::GetBackgroundDrawList();
	Assert(dl);
	dl->AddCallback(IGC_SetRenderBitmap, renderBitmap);
}

void QiOgl_BeginFrame()
{
	QiOgl_Clear();

	Assert(gOgl->inBeginFrame == 0);
	gOgl->inBeginFrame++;

	Assert(gOgl->renderBitmapStackPos == 0);
	QiOgl_PushRenderBitmap(gOgl->screenBitmap);

	ImDrawList *dl = ImGui::GetBackgroundDrawList();
	Assert(dl);
	Color clearColor(FR(), FR(), FR(), 1.0f);
	dl->AddCallback(IGC_ClearCurrentTarget, (void *)(uintptr_t)((u32)clearColor));
}

void QiOgl_EndFrame()
{
	gOgl->inBeginFrame--;
	Assert(gOgl->inBeginFrame == 0);

	QiOgl_PopRenderBitmap();
	Assert(gOgl->renderBitmapStackPos == 0);

	Assert(gOgl->screenBitmap && gOgl->screenBitmap->hardwareId);
	OglBitmap *oglBmp = (OglBitmap *)gOgl->screenBitmap->hardwareId;
	ImGuiIO &  io     = ImGui::GetIO();

	ImDrawList *dl = ImGui::GetBackgroundDrawList();
	Assert(dl);
	dl->AddCallback(IGC_SetRenderBitmap, nullptr);
	dl->AddImage((void *)(uintptr_t)oglBmp->texture, ImVec2(0.0, 0.0), io.DisplaySize);
}

struct TexCoordRect
{
	ImVec2 ul;
	ImVec2 br;
};

static void QiOgl_PixelRectToTexCoords(const Bitmap *bitmap, const Rect *pixelRect, TexCoordRect *texCoordRect)
{
	r32 iw           = 1.0f / (r32)bitmap->width;
	r32 ih           = 1.0f / (r32)bitmap->height;
	texCoordRect->ul = ImVec2(pixelRect->left * iw, pixelRect->top * ih);
	texCoordRect->br = ImVec2((pixelRect->left + pixelRect->width) * iw, (pixelRect->top + pixelRect->height) * ih);
}

static void QiOgl_Blit(const Bitmap *bitmap, const Rect *srcRect, const Rect *destRect, ColorU tint)
{
	Assert(gOgl->inBeginFrame > 0);

	ImVec2       ul(destRect->left, destRect->top);
	ImVec2       br(destRect->left + destRect->width, destRect->top + destRect->height);
	TexCoordRect srcUVs;

	QiOgl_PixelRectToTexCoords(bitmap, srcRect, &srcUVs);

	Assert(bitmap->hardwareId);
	OglBitmap *oglBmp = (OglBitmap *)bitmap->hardwareId;

	ImDrawList *dl = ImGui::GetBackgroundDrawList();
	Assert(dl);
	dl->AddImage((void *)(uintptr_t)oglBmp->texture, ul, br, srcUVs.ul, srcUVs.br, (u32)tint);
}

void QiOgl_LoadBitmapToTex(GLuint tex, const Bitmap *bitmap)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	CheckGl();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->pixels);
	CheckGl();
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGl();
}

void QiOgl_RegisterBitmap(Bitmap *bitmap, bool canBeTarget);
void QiOgl_UnregisterBitmap(Bitmap *bitmap);

static bool QiOgl_IsBitmapRegistered(const Bitmap *bitmap)
{
	return bitmap->hardwareId != nullptr;
}

void QiOgl_LoadBitmap(Bitmap *bitmap)
{
	if (!bitmap->hardwareId)
	{
		QiOgl_RegisterBitmap(bitmap, false);
		Assert(bitmap->hardwareId);
	}
	OglBitmap *oglBmp = (OglBitmap *)bitmap->hardwareId;
	QiOgl_LoadBitmapToTex(oglBmp->texture, bitmap);
}

void QiOgl_RegisterBitmap(Bitmap *bitmap, bool canBeTarget)
{
	if (bitmap->hardwareId)
	{
		QiOgl_UnregisterBitmap(bitmap);
	}

	Assert(gOgl->numTextures < kMaxTextures);
	OglBitmap *oglBmp = &gOgl->textures[gOgl->numTextures];
	memset(oglBmp, 0, sizeof(*oglBmp));
	oglBmp->textureIdx = gOgl->numTextures++;
	glGenTextures(1, &oglBmp->texture);
	CheckGl();
	oglBmp->bitmap = bitmap;
	bitmap->hardwareId = oglBmp;

	glBindTexture(GL_TEXTURE_2D, oglBmp->texture);

	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	CheckGl();

	glBindTexture(GL_TEXTURE_2D, 0);

	if (canBeTarget)
	{
		QiOgl_LoadBitmap(bitmap);
		glGenFramebuffers(1, &oglBmp->fbo);
		CheckGl();

		glBindFramebuffer(GL_FRAMEBUFFER, oglBmp->fbo);
		CheckGl();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oglBmp->texture, 0);
		CheckGl();

		GLenum drawColor0[] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, drawColor0);
		CheckGl();

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		Assert(status == GL_FRAMEBUFFER_COMPLETE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CheckGl();
	}
}

void QiOgl_UnregisterBitmap(Bitmap *bitmap)
{
	if (!bitmap->hardwareId)
		return;

	OglBitmap *oglBmp = (OglBitmap *)bitmap->hardwareId;
	glDeleteTextures(1, &oglBmp->texture);

	u32 curIdx = oglBmp->textureIdx;
	Assert(oglBmp == &gOgl->textures[curIdx]);
	u32 lastIdx                       = --gOgl->numTextures;
	gOgl->textures[curIdx]            = gOgl->textures[lastIdx];
	gOgl->textures[curIdx].textureIdx = curIdx;

	bitmap->hardwareId = nullptr;
}

void QiOgl_Clear()
{
	glClearColor(FR(), FR(), FR(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void QiOgl_BlitBufferToScreen(const Bitmap *bitmap)
{
#if 0
	QiOgl_InitBlitBufferState();
	QiOgl_BitmapToTexture(gOgl->blitTexture, bitmap);
	glBindVertexArray(gOgl->quadVao);
	glUseProgram(gOgl->blitProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gOgl->blitTexture);
	glUniform1i(gOgl->samplerUniformLocation, 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
#endif
}

void QiOgl_SetupImGuiState(ImDrawData *drawData, i32 fbWidth, i32 fbHeight, GLuint vao)
{
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	CheckGl();

	// Setup viewport, orthographic projection matrix
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport
	// apps.
	glViewport(0, 0, (GLsizei)fbWidth, (GLsizei)fbHeight);
	float       L              = drawData->DisplayPos.x;
	float       R              = drawData->DisplayPos.x + drawData->DisplaySize.x;
	float       T              = drawData->DisplayPos.y;
	float       B              = drawData->DisplayPos.y + drawData->DisplaySize.y;
	const float orthoMtx[4][4] = {
		{2.0f / (R - L), 0.0f, 0.0f, 0.0f},
		{0.0f, 2.0f / (T - B), 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f, 0.0f},
		{(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
	};
	glUseProgram(gOgl->drawUIProgram);
	glUniform1i(gOgl->uiTexLocation, 0);
	glUniformMatrix4fv(gOgl->uiProjMtxLocation, 1, GL_FALSE, &orthoMtx[0][0]);
	CheckGl();


	// Bind vertex/index buffers and setup attributes for ImDrawVert
	glBindVertexArray(vao);
	CheckGl();
	glBindBuffer(GL_ARRAY_BUFFER, gOgl->uiVbo);
	CheckGl();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gOgl->uiElements);
	CheckGl();

	glEnableVertexAttribArray(0);
	CheckGl();
	glEnableVertexAttribArray(1);
	CheckGl();
	glEnableVertexAttribArray(2);
	CheckGl();

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, col));
	CheckGl();
}

void QiOgl_DrawImGui(ImDrawData *drawData)
{
	i32 fbWidth  = (i32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	i32 fbHeight = (i32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

	if (fbWidth <= 0 || fbHeight <= 0)
		return;

	GLint lastViewport[4];
	glGetIntegerv(GL_VIEWPORT, lastViewport);
	GLint lastScissorBox[4];
	glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);
	GLenum lastBlendSrcRgb;
	glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&lastBlendSrcRgb);
	GLenum lastBlendDstRgb;
	glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&lastBlendDstRgb);
	GLenum lastBlendSrcAlpha;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&lastBlendSrcAlpha);
	GLenum lastBlendDstAlpha;
	glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&lastBlendDstAlpha);
	GLenum lastBlendEquationRgb;
	glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&lastBlendEquationRgb);
	GLenum lastBlendEquationAlpha;
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&lastBlendEquationAlpha);
	GLboolean lastEnableBlend       = glIsEnabled(GL_BLEND);
	GLboolean lastEnableCullFace    = glIsEnabled(GL_CULL_FACE);
	GLboolean lastEnableDepthTest   = glIsEnabled(GL_DEPTH_TEST);
	GLboolean lastEnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);

	QiOgl_SetupImGuiState(drawData, fbWidth, fbHeight, vao);

	ImVec2 clipOffset = drawData->DisplayPos;
	ImVec2 clipScale  = drawData->FramebufferScale;

	for (int drawIdx = 0; drawIdx < drawData->CmdListsCount; drawIdx++)
	{
		const ImDrawList *cmdList = drawData->CmdLists[drawIdx];

		// Upload vtx data
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid *)cmdList->VtxBuffer.Data, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid *)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);
		CheckGl();

		for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
		{
			const ImDrawCmd *cmd = &cmdList->CmdBuffer[cmdIdx];

			if (cmd->UserCallback != nullptr)
			{
				if (cmd->UserCallback == ImDrawCallback_ResetRenderState)
					QiOgl_SetupImGuiState(drawData, fbWidth, fbHeight, vao);
				else
					cmd->UserCallback(cmdList, cmd);
			}
			else
			{
				static volatile u32 screenBlits = 0;
				ImVec4 clipRect;
				clipRect.x = (cmd->ClipRect.x - clipOffset.x) * clipScale.x;
				clipRect.y = (cmd->ClipRect.y - clipOffset.y) * clipScale.y;
				clipRect.z = (cmd->ClipRect.z - clipOffset.x) * clipScale.x;
				clipRect.w = (cmd->ClipRect.w - clipOffset.y) * clipScale.y;

				if (clipRect.x < fbWidth && clipRect.y < fbHeight && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
				{
					glScissor(clipRect.x, (int)(fbHeight - clipRect.w), (int)(clipRect.z - clipRect.x), (int)(clipRect.w - clipRect.y));
					CheckGl();

					GLuint curTexture = (GLuint)(intptr_t)cmd->TextureId;
					if (curTexture == ((OglBitmap *)(gOgl->screenBitmap->hardwareId))->texture)
					{
						screenBlits++;
					}
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)cmd->TextureId);
					CheckGl();
					glDrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount, GL_UNSIGNED_SHORT, (void *)(intptr_t)(cmd->IdxOffset * sizeof(ImDrawIdx)));
					CheckGl();
				}
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGl();
	glDeleteVertexArrays(1, &vao);
	CheckGl();
	glBlendEquationSeparate(lastBlendEquationRgb, lastBlendEquationAlpha);
	glBlendFuncSeparate(lastBlendSrcRgb, lastBlendDstRgb, lastBlendSrcAlpha, lastBlendDstAlpha);
	if (lastEnableBlend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
	if (lastEnableCullFace)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	if (lastEnableDepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	if (lastEnableScissorTest)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
	glViewport(lastViewport[0], lastViewport[1], (GLsizei)lastViewport[2], (GLsizei)lastViewport[3]);
	glScissor(lastScissorBox[0], lastScissorBox[1], (GLsizei)lastScissorBox[2], (GLsizei)lastScissorBox[3]);
}

void QiOgl_CreateFontsTexture()
{
	ImGuiIO &io = ImGui::GetIO();
	u8 *     pixels;
	i32      width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	glGenTextures(1, &gOgl->fontTexture);
	glBindTexture(GL_TEXTURE_2D, gOgl->fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	io.Fonts->TexID = (ImTextureID)(intptr_t)gOgl->fontTexture;
}

void QiOgl_DestroyFontTexture()
{
	if (gOgl->fontTexture)
	{
		ImGuiIO &io = ImGui::GetIO();
		glDeleteTextures(1, &gOgl->fontTexture);
		io.Fonts->TexID   = 0;
		gOgl->fontTexture = 0;
	}
}

// HWI interface
struct OglHwi : public Hwi
{
	OglHwi() {}
	virtual ~OglHwi() {}

	void BeginFrame() override
	{
		ImGui::NewFrame();
		QiOgl_BeginFrame();
	}

	void EndFrame() override
	{
		QiOgl_EndFrame();
		QiOgl_Clear();
		ImGui::EndFrame();
		ImGui::Render();
		QiOgl_DrawImGui(ImGui::GetDrawData());
	}

	void RegisterBitmap(Bitmap *bitmap, bool canBeRenderTarget) override { QiOgl_RegisterBitmap(bitmap, canBeRenderTarget); }

	void UnregisterBitmap(Bitmap *bitmap) override { QiOgl_UnregisterBitmap(bitmap); }

	void UploadBitmap(Bitmap *bitmap) override { QiOgl_LoadBitmap(bitmap); }

	void SetScreenTarget(Bitmap *screenTarget) override
	{
		if (!QiOgl_IsBitmapRegistered(screenTarget))
			QiOgl_RegisterBitmap(screenTarget, true);
		gOgl->screenBitmap = screenTarget;
	}

	void PushRenderTarget(Bitmap *targetBitmap) override { QiOgl_PushRenderBitmap(targetBitmap); }

	void PopRenderTarget() override { QiOgl_PopRenderBitmap(); }

	void BlitStretched(Bitmap *srcBitmap, const Rect *srcRectPixels, const Rect *destRectPixels, ColorU tint) override
	{
		QiOgl_Blit(srcBitmap, srcRectPixels, destRectPixels, tint);
	}

	void Blit(Bitmap *srcBitmap, v2 srcXY, v2 destXY, v2 size, ColorU tint) override
	{
		Rect srcRect, destRect;

		srcRect.left   = srcXY.x;
		srcRect.top    = srcXY.y;
		srcRect.width  = size.x;
		srcRect.height = size.y;

		destRect.left   = destXY.x;
		destRect.top    = destXY.y;
		destRect.width  = size.x;
		destRect.height = size.y;

		QiOgl_Blit(srcBitmap, &srcRect, &destRect, tint);
	}

	static inline void CvtRectToImVecs(const Rect *rect, ImVec2 &mins, ImVec2 &maxs)
	{
		mins.x = rect->left;
		mins.y = rect->top;
		maxs.x = rect->left + rect->width;
		maxs.y = rect->top + rect->height;
	}

	void PushClipRect(Rect *clipRect) override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		ImVec2 mins, maxs;
		CvtRectToImVecs(clipRect, mins, maxs);
		dl->PushClipRect(mins, maxs, false);
	}

	void PopClipRect() override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		dl->PopClipRect();
	}

	void PushBlendState(BlendState blend) override {}

	void PopBlendState() override {}

	void DrawLine(v2 p0, v2 p1, ColorU color) override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		dl->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), 1.0f, (u32)color);
	}

	void DrawRect(const Rect *rect, ColorU color) override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		ImVec2 mins, maxs;
		CvtRectToImVecs(rect, mins, maxs);
		dl->AddRect(mins, maxs, (u32)color, 0, 0, 1.0f);
	}

	void FillRect(const Rect *rect, ColorU color) override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		ImVec2 mins, maxs;
		CvtRectToImVecs(rect, mins, maxs);
		dl->AddRectFilled(mins, maxs, (u32)color, 0, 0);
	}

	void DrawBezier(v2 a, v2 b, v2 c, v2 d, ColorU color) override
	{
		ImDrawList *dl = ImGui::GetBackgroundDrawList();
		Assert(dl);
		ImVec2 ia(a.x, a.y), ib(b.x, b.y), ic(c.x, c.y), id(d.x, d.y);
		dl->AddBezierCurve(ia, ib, ic, id, (u32)color, 1.0f);
	}

	void ResetState() override {}

	void Finalize() override {}
};

Hwi *gHwi = nullptr;

void QiOgl_InitSystem(const SubSystem *sys, bool isReinit)
{
	gOgl         = (OglGlobals *)sys->globalPtr;
	void *hwiPtr = (void *)(gOgl + 1);
	gHwi         = new (hwiPtr) OglHwi();

	if (!isReinit)
	{
		memset(gOgl, 0, sizeof(*gOgl));
		QiOgl_Init();
	}
}

SubSystem HardwareSubsystem = {"OglHardware", QiOgl_InitSystem, sizeof(OglGlobals) + sizeof(OglHwi) + kOglHardwareMemSize};
