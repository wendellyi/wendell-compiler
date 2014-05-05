/*
 * File: scan.c
 * The scanner implementation for the TINY compiler
 */

#include "globals.h"
#include "util.h"
#include "scan.h"

/* states in scanner DFA */
typedef enum
{
    START,
    INASSIGN,
    INCOMMENT,
    INNUM,
    INID,
    DONE
} StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];

/* BUFLEN = length of the input buffer for source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN];    /* hold current line */
static int linepos = 0;         /* current position in LineBuf */
static int bufsize = 0;         /* current size of buffer string */
static int EOF_Flag = FALSE;

/**
 * getNextChar fetches the next non-blank character from lineBuf,
 * reading in a new line if lineBuf is exhausted.
 */
static int getNextChar(void)
{
    /* 当行内的索引小于缓冲区内容的长度时，直接返回就可以了，
     * 否则需要再读取一行 */
    if (!(linepos < bufsize))
    {
        lineno++;
        if (fgets(lineBuf, BUFLEN-1, source))
        {
            if (EchoSource)
            {
                fprintf(listing, "%4d: %s", lineno, lineBuf);
            }

            bufsize = strlen(lineBuf);
            linepos = 0;
            return lineBuf[linepos++];
        }
        else
        {
            EOF_Flag = TRUE;
            return EOF;
        }
    }
    else
        return lineBuf[linepos++];
}

/**
 * ungetNextChar backtracks one character in lineBuf.
 * 
 */
static void ungetNextChar(void)
{
    if (!EOF_Flag)
        linepos--;
}

/**
 * lookup table of reserved words.
 * 
 */
static struct
{
    char * str;
    TokenType tok;
} reservedWords[MAXRESERVED] = {
    { "if", IF },
    { "then", THEN },
    { "else", ELSE },
    { "end", END },
    { "repeat", REPEAT },
    { "until", UNTIL },
    { "read", READ },
    { "write", WRITE }
};

static TokenType reservedLookup(char * s)
{
    int i;
    for (i = 0; i < MAXRESERVED; i++)
    {
        if (!strcmp(s, reservedWords[i].str))
        {
            return reservedWords[i].tok;
        }
    }

    return ID;
}

TokenType getToken(void)
{
    int tokenStringIndex = 0;
    TokenType currentToken;
    StateType state = START;
    int save;

    while (state != DONE)
    {
        int c = getNextChar();
        save = TRUE;
        switch (state)
        {
        case START:
            if (isdigit(c))
            {
                state = INNUM;
            }
            else if (isalpha(c))
            {
                state = INID;
            }
            else if (c == ':')
            {
                state = INASSIGN;
            }
            else if (c == ' ' || c == '\t' || c == '\n')
            {
                save = FALSE;
            }
            else if (c == '{')
            {
                save = FALSE;
                state = INCOMMENT;
            }
            else
            {
                state = DONE;
                switch (c)
                {
                case EOF:
                    save = FALSE;
                    currentToken = ENDFILE;
                    break;

                case '=':
                    currentToken = EQ;
                    break;

                case '<':
                    currentToken = LT;
                    break;

                case '+':
                    currentToken = PLUS;
                    break;

                case '-':
                    currentToken = MINUS;
                    break;

                case '*':
                    currentToken = TIMES;
                    break;

                case '/':
                    currentToken = OVER;
                    break;

                case '(':
                    currentToken = LPAREN;
                    break;

                case ')':
                    currentToken = RPAREN;
                    break;

                case ';':
                    currentToken = SEMI;
                    break;

                default:
                    currentToken = ERROR;
                    break;                    
                } /* switch */
            }     /* else */
            break;

        case INCOMMENT:
            save = FALSE;
            if (c == EOF)
            {
                state = DONE;
                currentToken = ENDFILE;
            }
            else if (c == ')')  /* 这个地方似乎有问题 */
            {
                state = START;
            }
            break;

        case INASSIGN:
            state = DONE;
            if (c == '=')
            {
                currentToken = ASSIGN;
            }
            else
            {
                ungetNextChar();
                save = FALSE;
                currentToken = ERROR;
            }
            break;

        case INNUM:
            if (!isdigit(c))
            {
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = NUM;
            }
            break;

        case INID:
            if (!isalpha(c))
            {
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = ID;
            }
            break;

        case DONE:
        default:
            fprintf(listing, "Scanner Bug: state= %d\n", state);
            state = DONE;
            currentToken = ERROR;
            break;
        }

        if ((save) && (tokenStringIndex <= MAXTOKENLEN))
        {
            tokenString[tokenStringIndex++] = (char)c;
        }

        if (state == DONE)
        {
            tokenString[tokenStringIndex] = 0;
            if (currentToken == ID)
            {
                currentToken = reservedLookup(tokenString);
            }
        }
    }

    /* 选择性打印日志 */
    if (TraceScan)
    {
        fprintf(listing, "\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }

    return currentToken;
}
