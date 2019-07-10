//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Simple lexer / tokenizer
//

#include "basictypes.h"

#include "qi.h"
#include "qi_lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

Lexer*
Lex_Init(Lexer* lex, char* buffer, size_t bufSize, char* tokenBuffer, size_t tokenBufSize)
{
	memset(lex, 0, sizeof(*lex));
	lex->curChar = lex->firstChar = buffer;
	lex->lastChar                 = buffer + bufSize;
	lex->charBufSize              = bufSize;

	lex->curTokenChar = lex->firstTokenChar = tokenBuffer;
	lex->lastTokenChar                      = tokenBuffer + tokenBufSize;
	lex->tokenBufSize                       = tokenBufSize;

	return lex;
}

char
Lex_GetNextChar(Lexer* lex)
{
	if (lex->numPutbacks > 0)
		return lex->putbacks[--lex->numPutbacks];

	if (lex->curChar == lex->lastChar)
		return 0;

	char c = *lex->curChar++;

	if (c == '\r')
		c = *lex->curChar++;

	if (c == '\n')
	{
		if (*lex->curChar == '\r')
			lex->curChar++;

		lex->curLine++;
		lex->linePos = 0;
	}

	lex->linePos++;

	return c;
}

void
Lex_UngetChar(Lexer* lex, char c)
{
	Assert(lex->numPutbacks < MAX_PUTBACK);
	lex->linePos--;
	lex->putbacks[lex->numPutbacks++] = c;
}

char
Lex_Peek(Lexer* lex)
{
	char c = Lex_GetNextChar(lex);
	Lex_UngetChar(lex, c);
	return c;
}

void
Lex_SetError(Lexer* lex, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf(lex->errorMsg, sizeof(lex->errorMsg), msg, args);
	va_end(args);
}

LexerToken
Lex_ReadStringUntil(Lexer* lex, LexerToken tok, char terminator)
{
	char c         = Lex_GetNextChar(lex);
	u32  startLine = lex->curLine;
	u32  startPos  = lex->linePos;

	while (c != terminator)
	{
		if (c == 0)
		{
			Lex_SetError(lex,
			             "Missing closing '%c' from %s opened at line: %d, col: %d\n",
			             terminator,
			             (tok == TOK_STRING ? "string" : "symbol"),
			             startLine,
			             startPos);
			return TOK_ERROR;
		}

		if (c == '\\')
		{
			c = Lex_GetNextChar(lex);
			switch (c)
			{
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 0:
				Lex_SetError(lex, "Unexpected EOF while reading escaped char in string\n");
				return TOK_ERROR;
			default:
				break;
			}
		}

		*lex->curTokenChar++ = c;
		c = Lex_GetNextChar(lex);
	}

    // Don't unget since we don't want the trailing terminator
	*lex->curTokenChar++ = 0;
	return tok;
}

void
Lex_SkipComment(Lexer* lex)
{
	char c = 0;
	do
	{
		c = Lex_GetNextChar(lex);
	} while (c != 0 && c != '\n');

	Lex_UngetChar(lex, c);
}

static bool
ValidHexDigit(char c)
{
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

void
Lex_ReadHexNumber(Lexer* lex)
{
	char c = Lex_GetNextChar(lex);
	while (c != 0 && ValidHexDigit(c))
	{
		*lex->curTokenChar++ = c;
		c                    = Lex_GetNextChar(lex);
	}

	Lex_UngetChar(lex, c);
    *lex->curTokenChar++ = 0;
}

void
Lex_ReadSymbol(Lexer* lex)
{
	char c = Lex_GetNextChar(lex);
	while (c != 0 && isalnum(c))
	{
		*lex->curTokenChar++ = c;
		c                    = Lex_GetNextChar(lex);
	}

	Lex_UngetChar(lex, c);
    *lex->curTokenChar++ = 0;
}

LexerToken
Lex_GetNextToken(Lexer* lex)
{
	lex->curTokenChar = lex->firstTokenChar;

	char c    = Lex_GetNextChar(lex);
	char peek = 0;
	switch (c)
	{
	case 0:
		return TOK_EOF;

	case '\n':
		return TOK_NEWLINE;

	case '\"':
		return Lex_ReadStringUntil(lex, TOK_STRING, '\"');

	case '`':
		return Lex_ReadStringUntil(lex, TOK_SYMBOL, '`');
		return TOK_SYMBOL;

	case '/':
		if (Lex_Peek(lex) == '/')
		{
			Lex_SkipComment(lex);
			*lex->curTokenChar++ = ' ';
			*lex->curTokenChar++ = 0;
			return TOK_SPACE;
		}
		else
		{
			Lex_UngetChar(lex, peek);
		}
		break;
	default:
	{
		// whitespace
		if (c == ' ' || c == '\t')
		{
			*lex->curTokenChar++ = ' ';
			*lex->curTokenChar++ = 0;

			do
			{
				c = Lex_GetNextChar(lex);
			} while (c != 0 && (c == ' ' || c == '\t'));

			Lex_UngetChar(lex, c);

			return TOK_SPACE;
		}

		// number (checking for a hex number first)
		else if (c == '0')
		{
			char next = Lex_GetNextChar(lex);
			if (next == 'x' || next == 'X')
			{
				char nextNext = Lex_Peek(lex);
				if (!ValidHexDigit(nextNext))
				{
					Lex_SetError(lex, "Malformed hexidecimal at line: %d  col: %d\n", lex->curLine, lex->linePos);
					return TOK_ERROR;
				}

				*lex->curTokenChar++ = c;
				*lex->curTokenChar++ = tolower(next);

				Lex_ReadHexNumber(lex);
				return TOK_NUMBER;
			}
			else
			{
				Lex_UngetChar(lex, next);
				goto number;
			}
		}

		else if (isdigit(c) || c == '.')
		{
		number:
			bool seenE   = false;
			bool seenDot = (c == '.');
			while (c != 0 && (isdigit(c) || (!seenDot && c == '.') || (!seenE && c == 'e')))
			{
				if (c == '.')
					seenDot = true;

				if (c == 'e')
					seenE = true;

				*lex->curTokenChar++ = c;
				c                    = Lex_GetNextChar(lex);
			}

			Lex_UngetChar(lex, c);
            *lex->curTokenChar++ = 0;
			return TOK_NUMBER;
		}

		// Identifier (symbol)
		else if (c == '_' || isalnum(c))
		{
			*lex->curTokenChar++ = c;

			Lex_ReadSymbol(lex);
			return TOK_SYMBOL;
		}
	}
    break;
    }

	*lex->curTokenChar++ = c;
	*lex->curTokenChar++ = 0;
	return c;
}

char* Lex_GetTokenText(Lexer* lex)
{
    return lex->firstTokenChar;
}
