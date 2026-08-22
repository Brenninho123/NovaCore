#ifndef NOVACORE_NCSCRIPT_H
#define NOVACORE_NCSCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NC_VALUE_NULL,
    NC_VALUE_BOOL,
    NC_VALUE_NUMBER,
    NC_VALUE_STRING
} NCValueType;

typedef struct NCValue {
    NCValueType type;
    double number;
    int boolean;
    char* string;
} NCValue;

typedef NCValue (*NCNativeFn)(NCValue* args, int argCount);

void NCScript_Init(void);
void NCScript_Shutdown(void);

void NCScript_RegisterNative(const char* name, NCNativeFn fn);

int NCScript_RunString(const char* source);
int NCScript_RunFile(const char* path);

const char* NCScript_GetLastError(void);

#ifdef __cplusplus
}
#endif

#endif
