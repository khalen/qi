#ifndef __QI_LEXER_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Simple tokenizer / lexer
//

#define MAX_PUTBACK 32
#define MAX_ERRMSG  512

struct Lexer
{
    char* curChar;
    char* firstChar;
    char* lastChar;
    size_t charBufSize;
    u32 curLine;
    u32 linePos;

    char* curTokenChar;
    char* firstTokenChar;
    char* lastTokenChar;
    char tokenBufSize;
    char putbacks[MAX_PUTBACK];
    u32 numPutbacks;

    char errorMsg[MAX_ERRMSG];
};

typedef i32 LexerToken;

#define TOK_ERROR -1
#define TOK_EOF 0
#define TOK_SYMBOL 1
#define TOK_NUMBER 2
#define TOK_NEWLINE 3
#define TOK_SPACE 4
#define TOK_STRING 5
#define TOK_MAX 10

Lexer* Lex_Init(Lexer* lex, char* buffer, size_t bufSize, char* tokenBuffer, size_t tokenBufSize);
char Lex_GetNextChar(Lexer* lex);
LexerToken Lex_GetNextToken(Lexer* lex);
void Lex_UngetChar(Lexer* lex, char c);
char* Lex_GetTokenText(Lexer* lex);

#define __QI_LEXER_H
#endif // #ifndef __QI_LEXER_H
