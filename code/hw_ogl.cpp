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

const u32    kMaxTextures        = 128;
const size_t kOglHardwareMemSize = 1024 * 1024;

struct OglBitmap
{
	Bitmap *bitmap;
	GLuint  texture;
};

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
};

static OglGlobals *gOgl;

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
	gOgl->uiVtxPosLocation   = glGetUniformLocation(gOgl->drawUIProgram, "pos");
	gOgl->uiVtxUVLocation    = glGetUniformLocation(gOgl->drawUIProgram, "uv");
	gOgl->uiVtxColorLocation = glGetUniformLocation(gOgl->drawUIProgram, "color");

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
static void CheckGL()
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

void QiOgl_BitmapToTexture(GLuint tex, const Bitmap *bitmap)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	CheckGL();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap->pixels);
	CheckGL();
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGL();
}

#define FR() ((float)rand()) / (float)RAND_MAX
void QiOgl_Clear()
{
	glClearColor(FR(), FR(), FR(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void QiOgl_BlitBufferToScreen(const Bitmap *bitmap)
{
	QiOgl_InitBlitBufferState();
	QiOgl_BitmapToTexture(gOgl->blitTexture, bitmap);
	glBindVertexArray(gOgl->quadVao);
	glUseProgram(gOgl->blitProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gOgl->blitTexture);
	glUniform1i(gOgl->samplerUniformLocation, 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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
	CheckGL();

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
	CheckGL();

	// Bind vertex/index buffers and setup attributes for ImDrawVert
	glBindBuffer(GL_ARRAY_BUFFER, gOgl->uiVbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gOgl->uiElements);
	CheckGL();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	CheckGL();

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)IM_OFFSETOF(ImDrawVert, col));
	CheckGL();
}

void QiOgl_DrawImGui(ImDrawData *drawData)
{
	i32 fbWidth  = (i32)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	i32 fbHeight = (i32)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

	if (fbWidth <= 0 || fbHeight <= 0)
		return;

	GLint last_viewport[4];
	glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4];
	glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLenum last_blend_src_rgb;
	glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb;
	glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha;
	glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb;
	glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha;
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&last_blend_equation_alpha);
	GLboolean last_enable_blend        = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face    = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test   = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

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
		CheckGL();

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
				ImVec4 clipRect;
				clipRect.x = (cmd->ClipRect.x - clipOffset.x) * clipScale.x;
				clipRect.y = (cmd->ClipRect.y - clipOffset.y) * clipScale.y;
				clipRect.z = (cmd->ClipRect.z - clipOffset.x) * clipScale.x;
				clipRect.w = (cmd->ClipRect.w - clipOffset.y) * clipScale.y;

				if (clipRect.x < fbWidth && clipRect.y < fbHeight && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
				{
					glScissor(clipRect.x, (int)(fbHeight - clipRect.w), (int)(clipRect.z - clipRect.x), (int)(clipRect.w - clipRect.y));
					CheckGL();

					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)cmd->TextureId);
					CheckGL();
					glDrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount, GL_UNSIGNED_SHORT, (void *)(intptr_t)(cmd->IdxOffset * sizeof(ImDrawIdx)));
					CheckGL();
				}
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGL();
	glDeleteVertexArrays(1, &vao);
	CheckGL();
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
	if (last_enable_cull_face)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	if (last_enable_depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
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

void QiOgl_RegisterBitmap(Bitmap *bitmap) {}

void QiOgl_InitSystem(const SubSystem *sys, bool isReinit)
{
	gOgl = (OglGlobals *)sys->globalPtr;

	if (!isReinit)
	{
		memset(gOgl, 0, sizeof(*gOgl));
		QiOgl_Init();
	}
}

SubSystem HardwareSubsystem = {"OglHardware", QiOgl_InitSystem, sizeof(OglGlobals) + kOglHardwareMemSize};

