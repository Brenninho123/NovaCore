#include "NCScript.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NC_MAX_NATIVES 128
#define NC_MAX_RECURSION 256

typedef enum {
    TOK_EOF, TOK_NUMBER, TOK_STRING, TOK_IDENT,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_ASSIGN, TOK_EQEQ, TOK_NOTEQ, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_COMMA, TOK_SEMI,
    TOK_VAR, TOK_FUNC, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_RETURN,
    TOK_TRUE, TOK_FALSE, TOK_NULLTOK
} NCTokenType;

typedef struct {
    NCTokenType type;
    const char* start;
    int length;
    double numberValue;
    int line;
} NCToken;

typedef struct {
    const char* source;
    int position;
    int line;
    NCToken* tokens;
    int tokenCount;
    int tokenCapacity;
} NCLexer;

typedef enum {
    NODE_NUMBER, NODE_STRING, NODE_BOOL, NODE_NULL, NODE_IDENTIFIER,
    NODE_BINARY, NODE_UNARY, NODE_ASSIGN, NODE_LOGICAL,
    NODE_VAR_DECL, NODE_BLOCK, NODE_IF, NODE_WHILE,
    NODE_CALL, NODE_FUNC_DECL, NODE_RETURN, NODE_PROGRAM, NODE_EXPR_STMT
} NCNodeType;

typedef struct NCNode {
    NCNodeType type;
    double numberValue;
    char* stringValue;
    int boolValue;
    struct NCNode* left;
    struct NCNode* right;
    struct NCNode* extra;
    struct NCNode** list;
    int listCount;
    int listCapacity;
    char** paramNames;
    int paramCount;
} NCNode;

typedef struct NCVarEntry {
    char* name;
    NCValue value;
} NCVarEntry;

typedef struct NCEnv {
    NCVarEntry* vars;
    int count;
    int capacity;
    struct NCEnv* parent;
} NCEnv;

typedef struct {
    char* name;
    NCNativeFn fn;
} NCNativeEntry;

typedef struct {
    int didReturn;
    NCValue returnValue;
} NCExecResult;

static NCNativeEntry g_natives[NC_MAX_NATIVES];
static int g_nativeCount = 0;

static char g_lastError[256];
static int g_recursionDepth = 0;

static NCEnv* g_globalEnv = NULL;

static char* NCStrdupRange(const char* start, int length) {
    char* result = (char*)malloc((size_t)length + 1);
    memcpy(result, start, (size_t)length);
    result[length] = '\0';
    return result;
}

static char* NCStrdup(const char* s) {
    int length = (int)strlen(s);
    return NCStrdupRange(s, length);
}

static void NCSetError(const char* message) {
    strncpy(g_lastError, message, sizeof(g_lastError) - 1);
    g_lastError[sizeof(g_lastError) - 1] = '\0';
}

static NCValue NCNull(void) {
    NCValue value;
    value.type = NC_VALUE_NULL;
    value.number = 0;
    value.boolean = 0;
    value.string = NULL;
    return value;
}

static NCValue NCNumber(double n) {
    NCValue value;
    value.type = NC_VALUE_NUMBER;
    value.number = n;
    value.boolean = 0;
    value.string = NULL;
    return value;
}

static NCValue NCBool(int b) {
    NCValue value;
    value.type = NC_VALUE_BOOL;
    value.number = 0;
    value.boolean = b;
    value.string = NULL;
    return value;
}

static NCValue NCString(const char* s) {
    NCValue value;
    value.type = NC_VALUE_STRING;
    value.number = 0;
    value.boolean = 0;
    value.string = NCStrdup(s);
    return value;
}

static int NCIsTruthy(NCValue value) {
    if (value.type == NC_VALUE_NULL) return 0;
    if (value.type == NC_VALUE_BOOL) return value.boolean;
    if (value.type == NC_VALUE_NUMBER) return value.number != 0.0;
    if (value.type == NC_VALUE_STRING) return value.string != NULL && value.string[0] != '\0';
    return 0;
}

static void NCLexerAddToken(NCLexer* lexer, NCTokenType type, const char* start, int length, double numberValue) {
    if (lexer->tokenCount >= lexer->tokenCapacity) {
        lexer->tokenCapacity = lexer->tokenCapacity == 0 ? 64 : lexer->tokenCapacity * 2;
        lexer->tokens = (NCToken*)realloc(lexer->tokens, sizeof(NCToken) * (size_t)lexer->tokenCapacity);
    }

    NCToken token;
    token.type = type;
    token.start = start;
    token.length = length;
    token.numberValue = numberValue;
    token.line = lexer->line;

    lexer->tokens[lexer->tokenCount] = token;
    lexer->tokenCount++;
}

static int NCIsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int NCIsDigit(char c) {
    return c >= '0' && c <= '9';
}

static NCTokenType NCKeywordType(const char* start, int length) {
    struct { const char* word; NCTokenType type; } keywords[] = {
        { "var", TOK_VAR }, { "func", TOK_FUNC }, { "if", TOK_IF },
        { "else", TOK_ELSE }, { "while", TOK_WHILE }, { "return", TOK_RETURN },
        { "true", TOK_TRUE }, { "false", TOK_FALSE }, { "null", TOK_NULLTOK }
    };
    int i;
    for (i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++) {
        int keywordLength = (int)strlen(keywords[i].word);
        if (keywordLength == length && strncmp(keywords[i].word, start, (size_t)length) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENT;
}

static int NCLexTokenize(NCLexer* lexer) {
    while (1) {
        char c = lexer->source[lexer->position];

        if (c == '\0') {
            NCLexerAddToken(lexer, TOK_EOF, &lexer->source[lexer->position], 0, 0);
            break;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->position++;
            continue;
        }

        if (c == '\n') {
            lexer->line++;
            lexer->position++;
            continue;
        }

        if (c == '/' && lexer->source[lexer->position + 1] == '/') {
            while (lexer->source[lexer->position] != '\0' && lexer->source[lexer->position] != '\n') {
                lexer->position++;
            }
            continue;
        }

        if (NCIsDigit(c)) {
            int start = lexer->position;
            while (NCIsDigit(lexer->source[lexer->position])) lexer->position++;
            if (lexer->source[lexer->position] == '.' && NCIsDigit(lexer->source[lexer->position + 1])) {
                lexer->position++;
                while (NCIsDigit(lexer->source[lexer->position])) lexer->position++;
            }
            int length = lexer->position - start;
            char* buffer = NCStrdupRange(&lexer->source[start], length);
            double value = atof(buffer);
            free(buffer);
            NCLexerAddToken(lexer, TOK_NUMBER, &lexer->source[start], length, value);
            continue;
        }

        if (c == '"') {
            lexer->position++;
            int start = lexer->position;
            while (lexer->source[lexer->position] != '"' && lexer->source[lexer->position] != '\0') {
                if (lexer->source[lexer->position] == '\n') lexer->line++;
                lexer->position++;
            }
            int length = lexer->position - start;
            if (lexer->source[lexer->position] == '"') {
                lexer->position++;
            }
            NCLexerAddToken(lexer, TOK_STRING, &lexer->source[start], length, 0);
            continue;
        }

        if (NCIsAlpha(c)) {
            int start = lexer->position;
            while (NCIsAlpha(lexer->source[lexer->position]) || NCIsDigit(lexer->source[lexer->position])) {
                lexer->position++;
            }
            int length = lexer->position - start;
            NCTokenType type = NCKeywordType(&lexer->source[start], length);
            NCLexerAddToken(lexer, type, &lexer->source[start], length, 0);
            continue;
        }

        int start = lexer->position;

        switch (c) {
            case '+': lexer->position++; NCLexerAddToken(lexer, TOK_PLUS, &lexer->source[start], 1, 0); break;
            case '-': lexer->position++; NCLexerAddToken(lexer, TOK_MINUS, &lexer->source[start], 1, 0); break;
            case '*': lexer->position++; NCLexerAddToken(lexer, TOK_STAR, &lexer->source[start], 1, 0); break;
            case '/': lexer->position++; NCLexerAddToken(lexer, TOK_SLASH, &lexer->source[start], 1, 0); break;
            case '%': lexer->position++; NCLexerAddToken(lexer, TOK_PERCENT, &lexer->source[start], 1, 0); break;
            case '(': lexer->position++; NCLexerAddToken(lexer, TOK_LPAREN, &lexer->source[start], 1, 0); break;
            case ')': lexer->position++; NCLexerAddToken(lexer, TOK_RPAREN, &lexer->source[start], 1, 0); break;
            case '{': lexer->position++; NCLexerAddToken(lexer, TOK_LBRACE, &lexer->source[start], 1, 0); break;
            case '}': lexer->position++; NCLexerAddToken(lexer, TOK_RBRACE, &lexer->source[start], 1, 0); break;
            case ',': lexer->position++; NCLexerAddToken(lexer, TOK_COMMA, &lexer->source[start], 1, 0); break;
            case ';': lexer->position++; NCLexerAddToken(lexer, TOK_SEMI, &lexer->source[start], 1, 0); break;
            case '=':
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_EQEQ, &lexer->source[start], 2, 0);
                } else {
                    NCLexerAddToken(lexer, TOK_ASSIGN, &lexer->source[start], 1, 0);
                }
                break;
            case '!':
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_NOTEQ, &lexer->source[start], 2, 0);
                } else {
                    NCLexerAddToken(lexer, TOK_NOT, &lexer->source[start], 1, 0);
                }
                break;
            case '<':
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_LE, &lexer->source[start], 2, 0);
                } else {
                    NCLexerAddToken(lexer, TOK_LT, &lexer->source[start], 1, 0);
                }
                break;
            case '>':
                lexer->position++;
                if (lexer->source[lexer->position] == '=') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_GE, &lexer->source[start], 2, 0);
                } else {
                    NCLexerAddToken(lexer, TOK_GT, &lexer->source[start], 1, 0);
                }
                break;
            case '&':
                lexer->position++;
                if (lexer->source[lexer->position] == '&') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_AND, &lexer->source[start], 2, 0);
                } else {
                    NCSetError("Unexpected character '&'");
                    return 0;
                }
                break;
            case '|':
                lexer->position++;
                if (lexer->source[lexer->position] == '|') {
                    lexer->position++;
                    NCLexerAddToken(lexer, TOK_OR, &lexer->source[start], 2, 0);
                } else {
                    NCSetError("Unexpected character '|'");
                    return 0;
                }
                break;
            default:
                NCSetError("Unexpected character in source");
                return 0;
        }
    }
    return 1;
}

typedef struct {
    NCToken* tokens;
    int position;
} NCParser;

static NCNode* NCNodeNew(NCNodeType type) {
    NCNode* node = (NCNode*)calloc(1, sizeof(NCNode));
    node->type = type;
    return node;
}

static void NCNodeListAdd(NCNode* node, NCNode* child) {
    if (node->listCount >= node->listCapacity) {
        node->listCapacity = node->listCapacity == 0 ? 8 : node->listCapacity * 2;
        node->list = (NCNode**)realloc(node->list, sizeof(NCNode*) * (size_t)node->listCapacity);
    }
    node->list[node->listCount] = child;
    node->listCount++;
}

static NCToken* NCParserPeek(NCParser* parser) {
    return &parser->tokens[parser->position];
}

static NCToken* NCParserAdvance(NCParser* parser) {
    NCToken* token = &parser->tokens[parser->position];
    if (token->type != TOK_EOF) parser->position++;
    return token;
}

static int NCParserCheck(NCParser* parser, NCTokenType type) {
    return NCParserPeek(parser)->type == type;
}

static int NCParserMatch(NCParser* parser, NCTokenType type) {
    if (NCParserCheck(parser, type)) {
        NCParserAdvance(parser);
        return 1;
    }
    return 0;
}

static NCToken* NCParserExpect(NCParser* parser, NCTokenType type, const char* message) {
    if (NCParserCheck(parser, type)) {
        return NCParserAdvance(parser);
    }
    NCSetError(message);
    return NULL;
}

static NCNode* NCParseExpr(NCParser* parser);
static NCNode* NCParseStatement(NCParser* parser);
static NCNode* NCParseBlock(NCParser* parser);

static NCNode* NCParsePrimary(NCParser* parser) {
    if (NCParserMatch(parser, TOK_NUMBER)) {
        NCToken* token = &parser->tokens[parser->position - 1];
        NCNode* node = NCNodeNew(NODE_NUMBER);
        node->numberValue = token->numberValue;
        return node;
    }

    if (NCParserMatch(parser, TOK_STRING)) {
        NCToken* token = &parser->tokens[parser->position - 1];
        NCNode* node = NCNodeNew(NODE_STRING);
        node->stringValue = NCStrdupRange(token->start, token->length);
        return node;
    }

    if (NCParserMatch(parser, TOK_TRUE)) {
        NCNode* node = NCNodeNew(NODE_BOOL);
        node->boolValue = 1;
        return node;
    }

    if (NCParserMatch(parser, TOK_FALSE)) {
        NCNode* node = NCNodeNew(NODE_BOOL);
        node->boolValue = 0;
        return node;
    }

    if (NCParserMatch(parser, TOK_NULLTOK)) {
        return NCNodeNew(NODE_NULL);
    }

    if (NCParserMatch(parser, TOK_IDENT)) {
        NCToken* token = &parser->tokens[parser->position - 1];
        NCNode* node = NCNodeNew(NODE_IDENTIFIER);
        node->stringValue = NCStrdupRange(token->start, token->length);
        return node;
    }

    if (NCParserMatch(parser, TOK_LPAREN)) {
        NCNode* expr = NCParseExpr(parser);
        NCParserExpect(parser, TOK_RPAREN, "Expected ')' after expression");
        return expr;
    }

    NCSetError("Expected expression");
    return NULL;
}

static NCNode* NCParseCall(NCParser* parser) {
    NCNode* expr = NCParsePrimary(parser);

    while (NCParserMatch(parser, TOK_LPAREN)) {
        NCNode* call = NCNodeNew(NODE_CALL);
        call->left = expr;

        if (!NCParserCheck(parser, TOK_RPAREN)) {
            do {
                NCNode* arg = NCParseExpr(parser);
                NCNodeListAdd(call, arg);
            } while (NCParserMatch(parser, TOK_COMMA));
        }

        NCParserExpect(parser, TOK_RPAREN, "Expected ')' after arguments");
        expr = call;
    }

    return expr;
}

static NCNode* NCParseUnary(NCParser* parser) {
    if (NCParserCheck(parser, TOK_NOT) || NCParserCheck(parser, TOK_MINUS)) {
        NCToken* op = NCParserAdvance(parser);
        NCNode* node = NCNodeNew(NODE_UNARY);
        node->numberValue = op->type == TOK_NOT ? 1 : 0;
        node->left = NCParseUnary(parser);
        return node;
    }
    return NCParseCall(parser);
}

static NCNode* NCParseFactor(NCParser* parser) {
    NCNode* left = NCParseUnary(parser);

    while (NCParserCheck(parser, TOK_STAR) || NCParserCheck(parser, TOK_SLASH) || NCParserCheck(parser, TOK_PERCENT)) {
        NCToken* op = NCParserAdvance(parser);
        NCNode* node = NCNodeNew(NODE_BINARY);
        node->stringValue = NCStrdupRange(op->start, op->length);
        node->left = left;
        node->right = NCParseUnary(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseTerm(NCParser* parser) {
    NCNode* left = NCParseFactor(parser);

    while (NCParserCheck(parser, TOK_PLUS) || NCParserCheck(parser, TOK_MINUS)) {
        NCToken* op = NCParserAdvance(parser);
        NCNode* node = NCNodeNew(NODE_BINARY);
        node->stringValue = NCStrdupRange(op->start, op->length);
        node->left = left;
        node->right = NCParseFactor(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseComparison(NCParser* parser) {
    NCNode* left = NCParseTerm(parser);

    while (NCParserCheck(parser, TOK_LT) || NCParserCheck(parser, TOK_LE) ||
           NCParserCheck(parser, TOK_GT) || NCParserCheck(parser, TOK_GE)) {
        NCToken* op = NCParserAdvance(parser);
        NCNode* node = NCNodeNew(NODE_BINARY);
        node->stringValue = NCStrdupRange(op->start, op->length);
        node->left = left;
        node->right = NCParseTerm(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseEquality(NCParser* parser) {
    NCNode* left = NCParseComparison(parser);

    while (NCParserCheck(parser, TOK_EQEQ) || NCParserCheck(parser, TOK_NOTEQ)) {
        NCToken* op = NCParserAdvance(parser);
        NCNode* node = NCNodeNew(NODE_BINARY);
        node->stringValue = NCStrdupRange(op->start, op->length);
        node->left = left;
        node->right = NCParseComparison(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseLogicAnd(NCParser* parser) {
    NCNode* left = NCParseEquality(parser);

    while (NCParserMatch(parser, TOK_AND)) {
        NCNode* node = NCNodeNew(NODE_LOGICAL);
        node->numberValue = 0;
        node->left = left;
        node->right = NCParseEquality(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseLogicOr(NCParser* parser) {
    NCNode* left = NCParseLogicAnd(parser);

    while (NCParserMatch(parser, TOK_OR)) {
        NCNode* node = NCNodeNew(NODE_LOGICAL);
        node->numberValue = 1;
        node->left = left;
        node->right = NCParseLogicAnd(parser);
        left = node;
    }

    return left;
}

static NCNode* NCParseAssignment(NCParser* parser) {
    NCNode* expr = NCParseLogicOr(parser);

    if (NCParserMatch(parser, TOK_ASSIGN)) {
        NCNode* value = NCParseAssignment(parser);

        if (expr->type == NODE_IDENTIFIER) {
            NCNode* node = NCNodeNew(NODE_ASSIGN);
            node->stringValue = NCStrdup(expr->stringValue);
            node->left = value;
            return node;
        }

        NCSetError("Invalid assignment target");
        return NULL;
    }

    return expr;
}

static NCNode* NCParseExpr(NCParser* parser) {
    return NCParseAssignment(parser);
}

static NCNode* NCParseVarDecl(NCParser* parser) {
    NCToken* name = NCParserExpect(parser, TOK_IDENT, "Expected variable name");
    NCNode* node = NCNodeNew(NODE_VAR_DECL);
    node->stringValue = NCStrdupRange(name->start, name->length);

    if (NCParserMatch(parser, TOK_ASSIGN)) {
        node->left = NCParseExpr(parser);
    }

    NCParserExpect(parser, TOK_SEMI, "Expected ';' after variable declaration");
    return node;
}

static NCNode* NCParseFuncDecl(NCParser* parser) {
    NCToken* name = NCParserExpect(parser, TOK_IDENT, "Expected function name");
    NCNode* node = NCNodeNew(NODE_FUNC_DECL);
    node->stringValue = NCStrdupRange(name->start, name->length);

    NCParserExpect(parser, TOK_LPAREN, "Expected '(' after function name");

    if (!NCParserCheck(parser, TOK_RPAREN)) {
        do {
            NCToken* param = NCParserExpect(parser, TOK_IDENT, "Expected parameter name");
            node->paramNames = (char**)realloc(node->paramNames, sizeof(char*) * (size_t)(node->paramCount + 1));
            node->paramNames[node->paramCount] = NCStrdupRange(param->start, param->length);
            node->paramCount++;
        } while (NCParserMatch(parser, TOK_COMMA));
    }

    NCParserExpect(parser, TOK_RPAREN, "Expected ')' after parameters");
    node->left = NCParseBlock(parser);
    return node;
}

static NCNode* NCParseIf(NCParser* parser) {
    NCParserExpect(parser, TOK_LPAREN, "Expected '(' after 'if'");
    NCNode* condition = NCParseExpr(parser);
    NCParserExpect(parser, TOK_RPAREN, "Expected ')' after condition");

    NCNode* node = NCNodeNew(NODE_IF);
    node->left = condition;
    node->right = NCParseBlock(parser);

    if (NCParserMatch(parser, TOK_ELSE)) {
        if (NCParserCheck(parser, TOK_IF)) {
            NCParserAdvance(parser);
            node->extra = NCParseIf(parser);
        } else {
            node->extra = NCParseBlock(parser);
        }
    }

    return node;
}

static NCNode* NCParseWhile(NCParser* parser) {
    NCParserExpect(parser, TOK_LPAREN, "Expected '(' after 'while'");
    NCNode* condition = NCParseExpr(parser);
    NCParserExpect(parser, TOK_RPAREN, "Expected ')' after condition");

    NCNode* node = NCNodeNew(NODE_WHILE);
    node->left = condition;
    node->right = NCParseBlock(parser);
    return node;
}

static NCNode* NCParseReturn(NCParser* parser) {
    NCNode* node = NCNodeNew(NODE_RETURN);

    if (!NCParserCheck(parser, TOK_SEMI)) {
        node->left = NCParseExpr(parser);
    }

    NCParserExpect(parser, TOK_SEMI, "Expected ';' after return value");
    return node;
}

static NCNode* NCParseBlock(NCParser* parser) {
    NCParserExpect(parser, TOK_LBRACE, "Expected '{'");

    NCNode* block = NCNodeNew(NODE_BLOCK);

    while (!NCParserCheck(parser, TOK_RBRACE) && !NCParserCheck(parser, TOK_EOF)) {
        NCNode* statement = NCParseStatement(parser);
        if (!statement) return NULL;
        NCNodeListAdd(block, statement);
    }

    NCParserExpect(parser, TOK_RBRACE, "Expected '}'");
    return block;
}

static NCNode* NCParseStatement(NCParser* parser) {
    if (NCParserMatch(parser, TOK_VAR)) return NCParseVarDecl(parser);
    if (NCParserMatch(parser, TOK_FUNC)) return NCParseFuncDecl(parser);
    if (NCParserMatch(parser, TOK_IF)) return NCParseIf(parser);
    if (NCParserMatch(parser, TOK_WHILE)) return NCParseWhile(parser);
    if (NCParserMatch(parser, TOK_RETURN)) return NCParseReturn(parser);
    if (NCParserCheck(parser, TOK_LBRACE)) return NCParseBlock(parser);

    NCNode* expr = NCParseExpr(parser);
    if (!expr) return NULL;

    NCParserExpect(parser, TOK_SEMI, "Expected ';' after expression");

    NCNode* stmt = NCNodeNew(NODE_EXPR_STMT);
    stmt->left = expr;
    return stmt;
}

static NCNode* NCParseProgram(NCParser* parser) {
    NCNode* program = NCNodeNew(NODE_PROGRAM);

    while (!NCParserCheck(parser, TOK_EOF)) {
        NCNode* statement = NCParseStatement(parser);
        if (!statement) return NULL;
        NCNodeListAdd(program, statement);
    }

    return program;
}

static NCEnv* NCEnvNew(NCEnv* parent) {
    NCEnv* env = (NCEnv*)calloc(1, sizeof(NCEnv));
    env->parent = parent;
    return env;
}

static void NCEnvDefine(NCEnv* env, const char* name, NCValue value) {
    int i;
    for (i = 0; i < env->count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            env->vars[i].value = value;
            return;
        }
    }

    if (env->count >= env->capacity) {
        env->capacity = env->capacity == 0 ? 8 : env->capacity * 2;
        env->vars = (NCVarEntry*)realloc(env->vars, sizeof(NCVarEntry) * (size_t)env->capacity);
    }

    env->vars[env->count].name = NCStrdup(name);
    env->vars[env->count].value = value;
    env->count++;
}

static int NCEnvSet(NCEnv* env, const char* name, NCValue value) {
    NCEnv* current = env;
    while (current) {
        int i;
        for (i = 0; i < current->count; i++) {
            if (strcmp(current->vars[i].name, name) == 0) {
                current->vars[i].value = value;
                return 1;
            }
        }
        current = current->parent;
    }
    return 0;
}

static int NCEnvGet(NCEnv* env, const char* name, NCValue* outValue) {
    NCEnv* current = env;
    while (current) {
        int i;
        for (i = 0; i < current->count; i++) {
            if (strcmp(current->vars[i].name, name) == 0) {
                *outValue = current->vars[i].value;
                return 1;
            }
        }
        current = current->parent;
    }
    return 0;
}

static NCValue NCEval(NCNode* node, NCEnv* env);
static NCExecResult NCExec(NCNode* node, NCEnv* env);

static double NCToNumber(NCValue value) {
    if (value.type == NC_VALUE_NUMBER) return value.number;
    if (value.type == NC_VALUE_BOOL) return value.boolean ? 1.0 : 0.0;
    if (value.type == NC_VALUE_STRING) return atof(value.string);
    return 0.0;
}

static NCValue NCEvalBinary(NCNode* node, NCEnv* env) {
    NCValue left = NCEval(node->left, env);
    NCValue right = NCEval(node->right, env);
    const char* op = node->stringValue;

    if (strcmp(op, "+") == 0) {
        if (left.type == NC_VALUE_STRING || right.type == NC_VALUE_STRING) {
            char leftBuf[64];
            char rightBuf[64];
            const char* leftStr = left.type == NC_VALUE_STRING ? left.string : (snprintf(leftBuf, sizeof(leftBuf), "%g", NCToNumber(left)), leftBuf);
            const char* rightStr = right.type == NC_VALUE_STRING ? right.string : (snprintf(rightBuf, sizeof(rightBuf), "%g", NCToNumber(right)), rightBuf);

            size_t totalLength = strlen(leftStr) + strlen(rightStr);
            char* combined = (char*)malloc(totalLength + 1);
            strcpy(combined, leftStr);
            strcat(combined, rightStr);

            NCValue result = NCString(combined);
            free(combined);
            return result;
        }
        return NCNumber(NCToNumber(left) + NCToNumber(right));
    }

    if (strcmp(op, "-") == 0) return NCNumber(NCToNumber(left) - NCToNumber(right));
    if (strcmp(op, "*") == 0) return NCNumber(NCToNumber(left) * NCToNumber(right));
    if (strcmp(op, "/") == 0) return NCNumber(NCToNumber(left) / NCToNumber(right));
    if (strcmp(op, "%") == 0) return NCNumber(fmod(NCToNumber(left), NCToNumber(right)));

    if (strcmp(op, "<") == 0) return NCBool(NCToNumber(left) < NCToNumber(right));
    if (strcmp(op, "<=") == 0) return NCBool(NCToNumber(left) <= NCToNumber(right));
    if (strcmp(op, ">") == 0) return NCBool(NCToNumber(left) > NCToNumber(right));
    if (strcmp(op, ">=") == 0) return NCBool(NCToNumber(left) >= NCToNumber(right));

    if (strcmp(op, "==") == 0) {
        if (left.type == NC_VALUE_STRING && right.type == NC_VALUE_STRING) {
            return NCBool(strcmp(left.string, right.string) == 0);
        }
        return NCBool(NCToNumber(left) == NCToNumber(right));
    }

    if (strcmp(op, "!=") == 0) {
        if (left.type == NC_VALUE_STRING && right.type == NC_VALUE_STRING) {
            return NCBool(strcmp(left.string, right.string) != 0);
        }
        return NCBool(NCToNumber(left) != NCToNumber(right));
    }

    return NCNull();
}

static NCValue NCEvalCall(NCNode* node, NCEnv* env) {
    if (node->left->type != NODE_IDENTIFIER) {
        NCSetError("Only named functions can be called");
        return NCNull();
    }

    const char* name = node->left->stringValue;

    NCValue args[32];
    int argCount = node->listCount > 32 ? 32 : node->listCount;
    int i;
    for (i = 0; i < argCount; i++) {
        args[i] = NCEval(node->list[i], env);
    }

    for (i = 0; i < g_nativeCount; i++) {
        if (strcmp(g_natives[i].name, name) == 0) {
            return g_natives[i].fn(args, argCount);
        }
    }

    NCValue funcValue;
    if (NCEnvGet(g_globalEnv, name, &funcValue)) {
        NCSetError("Value is not callable");
        return NCNull();
    }

    NCSetError("Undefined function");
    return NCNull();
}

static NCEnv* NCFindFuncEnv(NCEnv* env, const char* name, NCNode** outNode) {
    (void)env;
    (void)name;
    (void)outNode;
    return NULL;
}

static NCValue NCCallUserFunction(NCNode* funcNode, NCValue* args, int argCount) {
    if (g_recursionDepth >= NC_MAX_RECURSION) {
        NCSetError("Maximum recursion depth exceeded");
        return NCNull();
    }

    g_recursionDepth++;

    NCEnv* callEnv = NCEnvNew(g_globalEnv);

    int i;
    for (i = 0; i < funcNode->paramCount; i++) {
        NCValue argValue = i < argCount ? args[i] : NCNull();
        NCEnvDefine(callEnv, funcNode->paramNames[i], argValue);
    }

    NCExecResult result = NCExec(funcNode->left, callEnv);

    g_recursionDepth--;

    if (result.didReturn) {
        return result.returnValue;
    }

    return NCNull();
}

static NCValue NCEval(NCNode* node, NCEnv* env) {
    if (!node) return NCNull();

    switch (node->type) {
        case NODE_NUMBER:
            return NCNumber(node->numberValue);
        case NODE_STRING:
            return NCString(node->stringValue);
        case NODE_BOOL:
            return NCBool(node->boolValue);
        case NODE_NULL:
            return NCNull();
        case NODE_IDENTIFIER: {
            NCValue value;
            if (NCEnvGet(env, node->stringValue, &value)) {
                return value;
            }
            NCSetError("Undefined variable");
            return NCNull();
        }
        case NODE_ASSIGN: {
            NCValue value = NCEval(node->left, env);
            if (!NCEnvSet(env, node->stringValue, value)) {
                NCEnvDefine(g_globalEnv, node->stringValue, value);
            }
            return value;
        }
        case NODE_BINARY:
            return NCEvalBinary(node, env);
        case NODE_UNARY: {
            NCValue operand = NCEval(node->left, env);
            if (node->numberValue == 1) {
                return NCBool(!NCIsTruthy(operand));
            }
            return NCNumber(-NCToNumber(operand));
        }
        case NODE_LOGICAL: {
            NCValue left = NCEval(node->left, env);
            if (node->numberValue == 1) {
                if (NCIsTruthy(left)) return left;
            } else {
                if (!NCIsTruthy(left)) return left;
            }
            return NCEval(node->right, env);
        }
        case NODE_CALL: {
            if (node->left->type == NODE_IDENTIFIER) {
                const char* name = node->left->stringValue;

                int i;
                for (i = 0; i < g_nativeCount; i++) {
                    if (strcmp(g_natives[i].name, name) == 0) {
                        NCValue args[32];
                        int argCount = node->listCount > 32 ? 32 : node->listCount;
                        for (i = 0; i < argCount; i++) {
                            args[i] = NCEval(node->list[i], env);
                        }
                        return g_natives[0].fn(args, argCount);
                    }
                }
            }
            return NCEvalCall(node, env);
        }
        default:
            return NCNull();
    }
}

static NCExecResult NCExecNoReturn(void) {
    NCExecResult result;
    result.didReturn = 0;
    result.returnValue = NCNull();
    return result;
}

static NCExecResult NCExec(NCNode* node, NCEnv* env) {
    if (!node) return NCExecNoReturn();

    switch (node->type) {
        case NODE_PROGRAM:
        case NODE_BLOCK: {
            NCEnv* blockEnv = node->type == NODE_BLOCK ? NCEnvNew(env) : env;
            int i;
            for (i = 0; i < node->listCount; i++) {
                NCExecResult result = NCExec(node->list[i], blockEnv);
                if (result.didReturn) {
                    return result;
                }
            }
            return NCExecNoReturn();
        }
        case NODE_VAR_DECL: {
            NCValue value = node->left ? NCEval(node->left, env) : NCNull();
            NCEnvDefine(env, node->stringValue, value);
            return NCExecNoReturn();
        }
        case NODE_FUNC_DECL: {
            NCValue placeholder = NCNull();
            NCEnvDefine(g_globalEnv, node->stringValue, placeholder);

            int i;
            for (i = 0; i < g_nativeCount; i++) {
                if (strcmp(g_natives[i].name, node->stringValue) == 0) {
                    break;
                }
            }

            g_natives[g_nativeCount].name = NCStrdup(node->stringValue);
            g_natives[g_nativeCount].fn = NULL;
            g_nativeCount++;

            return NCExecNoReturn();
        }
        case NODE_IF: {
            NCValue condition = NCEval(node->left, env);
            if (NCIsTruthy(condition)) {
                return NCExec(node->right, env);
            } else if (node->extra) {
                return NCExec(node->extra, env);
            }
            return NCExecNoReturn();
        }
        case NODE_WHILE: {
            while (NCIsTruthy(NCEval(node->left, env))) {
                NCExecResult result = NCExec(node->right, env);
                if (result.didReturn) {
                    return result;
                }
            }
            return NCExecNoReturn();
        }
        case NODE_RETURN: {
            NCExecResult result;
            result.didReturn = 1;
            result.returnValue = node->left ? NCEval(node->left, env) : NCNull();
            return result;
        }
        case NODE_EXPR_STMT: {
            NCEval(node->left, env);
            return NCExecNoReturn();
        }
        default:
            return NCExecNoReturn();
    }
}

void NCScript_Init(void) {
    g_nativeCount = 0;
    g_recursionDepth = 0;
    g_lastError[0] = '\0';
    g_globalEnv = NCEnvNew(NULL);
}

void NCScript_Shutdown(void) {
    g_globalEnv = NULL;
}

void NCScript_RegisterNative(const char* name, NCNativeFn fn) {
    if (g_nativeCount >= NC_MAX_NATIVES) {
        NCSetError("Too many native functions registered");
        return;
    }

    g_natives[g_nativeCount].name = NCStrdup(name);
    g_natives[g_nativeCount].fn = fn;
    g_nativeCount++;
}

int NCScript_RunString(const char* source) {
    if (!g_globalEnv) {
        NCScript_Init();
    }

    NCLexer lexer;
    lexer.source = source;
    lexer.position = 0;
    lexer.line = 1;
    lexer.tokens = NULL;
    lexer.tokenCount = 0;
    lexer.tokenCapacity = 0;

    if (!NCLexTokenize(&lexer)) {
        free(lexer.tokens);
        return 0;
    }

    NCParser parser;
    parser.tokens = lexer.tokens;
    parser.position = 0;

    NCNode* program = NCParseProgram(&parser);

    if (!program) {
        free(lexer.tokens);
        return 0;
    }

    NCExec(program, g_globalEnv);

    free(lexer.tokens);
    return 1;
}

int NCScript_RunFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        NCSetError("Failed to open script file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc((size_t)size + 1);
    fread(buffer, 1, (size_t)size, file);
    buffer[size] = '\0';
    fclose(file);

    int result = NCScript_RunString(buffer);
    free(buffer);
    return result;
}

const char* NCScript_GetLastError(void) {
    return g_lastError;
}
