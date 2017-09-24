//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Platform portable OGL routines. TODO: Should we link this with both the
// gameplay dll and the platform exe, or expose through the game dll interface?
//

#include "basictypes.h"

#include "qi_ogl.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

internal struct
{
	GLuint blitTexture;
	GLuint blitProgram;

	GLuint quadVao;
	GLuint quadVbo;

    GLint samplerUniformLocation;
} gOgl;

// TODO: Change shader load to use platform functionality once we have proper files / asset system
internal char*
GetShaderText(const char* fileName)
{
	struct stat statBuf;
	if (stat(fileName, &statBuf) != 0)
	{
		Assert(0 && "Couldn't stat a shader file");
	}

	char* shaderBuf = (char*)malloc(statBuf.st_size + 1);
	memset(shaderBuf, 0, statBuf.st_size + 1);
	Assert(shaderBuf);

	FILE* f = fopen(fileName, "rb");
	fread(shaderBuf, 1, statBuf.st_size, f);
	fclose(f);

	return shaderBuf;
}

internal void
FreeShaderText(void* text)
{
	free(text);
}

internal GLuint
         LoadGlslShader(GLenum shaderType, const char* shaderFile)
{
	GLuint shader       = glCreateShader(shaderType);
	char*  shaderSource = GetShaderText(shaderFile);
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
		char* log = (char*)malloc(logLen + 1);
		glGetShaderInfoLog(shader, logLen + 1, nullptr, log);
		printf("Shader compile failed! Log follows:\n");
		printf("%s", log);
		free(log);
	}

	return shader;
}

void
QiOgl_Init()
{
	if (!gladLoadGL())
	{
		Assert(0 && "Couldn't init OpenGL");
	}

	// Set up screen blit
	glGenTextures(1, &gOgl.blitTexture);
    glBindTexture(GL_TEXTURE_2D, gOgl.blitTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint vs = LoadGlslShader(GL_VERTEX_SHADER, "blit_vs.glsl");
	GLuint fs = LoadGlslShader(GL_FRAGMENT_SHADER, "blit_fs.glsl");

	gOgl.blitProgram = glCreateProgram();
	glAttachShader(gOgl.blitProgram, vs);
	glAttachShader(gOgl.blitProgram, fs);
	glLinkProgram(gOgl.blitProgram);

	GLint status;
	glGetProgramiv(gOgl.blitProgram, GL_LINK_STATUS, &status);
	Assert(status == GL_TRUE);

    glUseProgram(gOgl.blitProgram);
    gOgl.samplerUniformLocation = glGetUniformLocation(gOgl.blitProgram, "tex");

    #if 1
	GLfloat vertices[] = {
	    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,
	    1.0f,  1.0f,  0.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f, 0.0f,
	};
    #else
	GLfloat vertices[] = {
	    0.0f, 0.0f, 0.0f,              0.0f, 1.0f,
        GAME_RES_X, 0.0f, 0.0f,        1.0f, 1.0f,
        GAME_RES_X, GAME_RES_Y, 0.0f,  1.0f, 0.0f,
        0.0f, GAME_RES_Y, 0.0f,        0.0f, 0.0f,
	};
    #endif

	glGenVertexArrays(1, &gOgl.quadVao);
	glGenBuffers(1, &gOgl.quadVbo);

	glBindVertexArray(gOgl.quadVao);
	glBindBuffer(GL_ARRAY_BUFFER, gOgl.quadVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*)(sizeof(GLfloat) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

static void CheckGL()
{
    GLenum err;
    bool errored = false;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        errored = true;
        fprintf(stderr, "GL Error: 0x%x\n", err);
    }

    Assert(!errored);
}

void
QiOgl_BitmapToTexture(GLuint tex, const Bitmap_s* bitmap)
{
	glBindTexture(GL_TEXTURE_2D, tex);
    CheckGL();
	glTexImage2D(
	    GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap->pixels);
    CheckGL();
	glBindTexture(GL_TEXTURE_2D, 0);
    CheckGL();
}

#define FR()  ((float)rand()) / RAND_MAX
void
QiOgl_Clear()
{
	glClearColor(FR(), FR(), FR(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
QiOgl_BlitBufferToScreen(const Bitmap_s* bitmap)
{
	QiOgl_BitmapToTexture(gOgl.blitTexture, bitmap);
	glBindVertexArray(gOgl.quadVao);
	glUseProgram(gOgl.blitProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gOgl.blitTexture);
    glUniform1i(gOgl.samplerUniformLocation, 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
