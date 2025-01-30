/*
CLI v1.0
--------

Should compile with any C99 compiler.

If you don't want CLI to use <stdlib.h>:
  #define QML_CLI_NO_STDLIB
This means that if you pass NULL to qmlCliSetMemoryFuncs, there will not be
any fallback functions and no memory will be allocated.

If you don't want to use response files (which requires using fopen() etc.):
  #define QML_CLI_NO_RESPONSE_FILE
This will also disable warnings output in case you attempt to load a response
file.

It is best to set these via compiler switches, for example:
  GCC: -DQML_CLI_NO_STDLIB
  MSVC: /DQML_CLI_NO_RESPONSE_FILE
Or in your build system, in Bip for example:
  [cli.c]
  define = { QML_CLI_NO_STDLIB = 1 }
*/

#include "cli.h"
#include <string.h>

#ifndef QML_CLI_NO_STDLIB
#include <stdlib.h>
#endif

#ifndef QML_CLI_NO_RESPONSE_FILE
#include <stdio.h>
#endif

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

typedef struct qmlCliParamPairS {
  /* points to the entire argument, including name and value */
  char *data;
  /* the length of the name. value is at data[nameLen+1] */
  int nameLen;
} qmlCliParamPair;

typedef struct qmlCliS {
  /* non-zero if windows console has been initialized */
  short winInit;
  char *programName;
  qmlCliAllocPfn alloc;
  qmlCliReallocPfn realloc;
  qmlCliFreePfn free;

  int posCount, posCap;
  /* if qmlCliSetMemoryFuncs was called, this is heap allocated. otherwise, this
     points to static memory (gStaticPos in this case) */
  char **posArgs;
  int flagCount, flagCap;
  char **flags;
  int paramCount, paramCap;
  qmlCliParamPair *params;
  int responseCount, responseCap;
  char **responseFiles;
} qmlCli;

static qmlCli gCli;

static char *gStaticPos[QML_CLI_MAX_POS_ARGS];
static char *gStaticFlags[QML_CLI_MAX_FLAGS];
static qmlCliParamPair gStaticParams[QML_CLI_MAX_PARAMS];

void qmlCliSetMemoryFuncs(qmlCliAllocPfn allocPfn, qmlCliReallocPfn reallocPfn, qmlCliFreePfn freePfn)
{
#ifdef QML_CLI_NO_STDLIB
  gCli.alloc = allocPfn;
  gCli.realloc = reallocPfn;
  gCli.free = freePfn;
#else
  gCli.alloc = allocPfn == NULL ? malloc : allocPfn;
  gCli.realloc = reallocPfn == NULL ? realloc : reallocPfn;
  gCli.free = freePfn == NULL ? free : freePfn;
#endif
}

void qmlCliFree()
{
  if(gCli.posCap > 0 && gCli.posArgs != NULL) {
    gCli.free(gCli.posArgs);
    gCli.posCount = 0;
    gCli.posCap = 0;
  }
  if(gCli.flagCap > 0 && gCli.flags != NULL) {
    gCli.free(gCli.flags);
    gCli.flagCount = 0;
    gCli.flagCap = 0;
  }
  if(gCli.paramCap > 0 && gCli.params != NULL) {
    gCli.free(gCli.params);
    gCli.paramCount = 0;
    gCli.paramCap = 0;
  }
  if(gCli.responseCap > 0 && gCli.responseFiles != NULL) {
    for(int i = 0; i < gCli.responseCount; ++i) {
      gCli.free(gCli.responseFiles[i]);
    }
    gCli.responseCount = 0;
    gCli.responseCap = 0;
    gCli.free(gCli.responseFiles);
  }
}

static void qmlCliMemInit()
{
  /* if any memory allocation function is not set, default to using static
     memory. the xxxCap fields are deliberately set to 0 here so we don't
     accidentally try to reallocate static memory */
  if(gCli.alloc == NULL || gCli.realloc == NULL || gCli.free == NULL) {
    gCli.posCap = 0;
    gCli.posArgs = gStaticPos;
    gCli.flagCap = 0;
    gCli.flags = gStaticFlags;
    gCli.paramCap = 0;
    gCli.params = gStaticParams;
    gCli.responseCap = 0;
    gCli.responseFiles = NULL; /* can't use response files with static memory */
    return;
  }

  /* only allocate once if called multiple times (e.g. Parse may be called
     multiple times which in turn calls MemInit) */
  if(gCli.posArgs == NULL) {
    gCli.posCap = QML_CLI_MAX_POS_ARGS;
    gCli.posArgs = malloc(gCli.posCap * sizeof(gCli.posArgs[0]));
  }

  if(gCli.flags == NULL) {
    gCli.flagCap = QML_CLI_MAX_FLAGS;
    gCli.flags = malloc(gCli.flagCap * sizeof(gCli.flags[0]));
  }

  if(gCli.params == NULL) {
    gCli.paramCap = QML_CLI_MAX_PARAMS;
    gCli.params = malloc(gCli.paramCap * sizeof(gCli.params[0]));
  }

  if(gCli.responseFiles == NULL) {
    gCli.responseCap = 4;
    gCli.responseFiles = malloc(gCli.responseCap * sizeof(gCli.responseFiles[0]));
  }
}

void qmlCliWinInit(void)
{
#ifdef _WIN32
  if(gCli.winInit++ > 0) {
    return;
  }
  HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(stdOut, &mode);
  SetConsoleMode(stdOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  /* we don't care enough to restore it, cry about it */
#endif
}

static char *qmlCliLoadResponseFile(char *fileName, size_t *fileSize)
{
#ifdef QML_CLI_NO_RESPONSE_FILE
  return NULL;
#else
  /* we need to allocate memory to store file contents */
  if(gCli.alloc == NULL || gCli.free == NULL) {
    return NULL;
  }

  FILE *fp = fopen(fileName, "rt");
  if(fp == NULL) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  if(size <= 0) {
    return NULL;
  }
  fseek(fp, 0, SEEK_SET);

  char *data = gCli.alloc((size+1)*sizeof(char)); /* extra space for a NUL */

  unsigned long count = fread(data, sizeof(char), size , fp);
  fclose(fp);
  if(count < size) {
    gCli.free(data);
    return NULL;
  }
  /* ensure it's NUL terminated */
  data[size] = '\0';

  int incrCount = gCli.responseCount + 1;
  if(incrCount > gCli.responseCap) {
    gCli.responseCap *= 2;
    gCli.responseFiles = gCli.realloc(gCli.responseFiles, gCli.responseCap*sizeof(gCli.responseFiles[0]));
  }
  gCli.responseFiles[gCli.responseCount++] = data;

  if(fileSize != NULL) {
    *fileSize = (size_t)size;
  }
  return data;
#endif
}

static void qmlCliAddPos(char *arg)
{
  int incrCount = gCli.posCount + 1;
  /* dynamically allocated, expand if necessary */
  if(gCli.posCap > 0) {
    if(incrCount > gCli.posCap) {
      gCli.posCap *= 2;
      gCli.posArgs = gCli.realloc(gCli.posArgs, gCli.posCap*sizeof(gCli.posArgs[0]));
    }
  } else if(incrCount > QML_CLI_MAX_POS_ARGS) {
    /* static memory, can't expand it :( */
    return;
  }

  gCli.posArgs[gCli.posCount++] = arg;
}

static void qmlCliAddFlag(char *arg)
{
  int incrCount = gCli.flagCount + 1;
  /* dynamically allocated, expand if necessary */
  if(gCli.flagCap > 0) {
    if(incrCount > gCli.flagCap) {
      gCli.flagCap *= 2;
      gCli.flags = gCli.realloc(gCli.flags, gCli.flagCap*sizeof(gCli.flags[0]));
    }
  } else if(incrCount > QML_CLI_MAX_FLAGS) {
    /* static memory, can't expand it :( */
    return;
  }

  gCli.flags[gCli.flagCount++] = arg;
}

static int cliCliGetParamNameLen(char *arg)
{
  for(int i = 0; arg[i] != '\0'; ++i) {
    if(arg[i] == '=') {
      return i;
    }
  }
  /* not a parameter, but a flag instead */
  return -1;
}

static void qmlCliAddParam(char *arg, int nameLen)
{
  int incrCount = gCli.paramCount + 1;
  /* dynamically allocated, expand if necessary */
  if(gCli.paramCap > 0) {
    if(incrCount > gCli.paramCap) {
      gCli.paramCap *= 2;
      gCli.params = gCli.realloc(gCli.params, gCli.paramCap*sizeof(gCli.params[0]));
    }
  } else if(incrCount > QML_CLI_MAX_PARAMS) {
    /* static memory, can't expand it :( */
    return;
  }

  qmlCliParamPair *pair = &gCli.params[gCli.paramCount++];
  pair->data = arg;
  pair->nameLen = nameLen;
}

static void qmlCliAddArg(char *arg);

static int qmlCliIsSpace(char chr)
{
  return chr == ' ' || chr == '\t' || chr == '\r' || chr == '\n';
}

static size_t qmlCliParseArg(char *str)
{
  size_t advance = 0;
  while(qmlCliIsSpace(str[advance])) {
    ++advance;
  }
  if(str[advance] == '\0') {
    return advance;
  }

  size_t start = advance;
  if(str[start] == '"' || str[start] == '\'') {
    char quote = str[start++];
    while(1) {
      ++advance;
      if(str[advance] == '\0') {
        break;
      }
      if(str[advance] != quote) {
        continue;
      }
      break;
    }
  } else {
    while(!qmlCliIsSpace(str[advance])) {
      ++advance;
    }
  }

  str[advance] = '\0';
  qmlCliAddArg(&str[start]);

  return advance;
}

static void qmlCliParseResponseFile(char *fileName)
{
  size_t size;
  char *data = qmlCliLoadResponseFile(fileName, &size);
  if(data == NULL) {
    /* we don't include <stdio.h> if QML_CLI_NO_RESPONSE_FILE is defined,
       therefore fprintf is unavailable. not like the user would care if that
       macro is defined anyway */
    #ifndef QML_CLI_NO_RESPONSE_FILE
    fprintf(stderr,
      "\x1b[33mCould not load response file '%s'.\x1b[0m\n",
      fileName);
    #endif
    return;
  }

  size_t i = 0;
  while(i < size) {
    i += qmlCliParseArg(&data[i]);
    ++i;
  }
}

static void qmlCliAddArg(char *arg)
{
  if(arg[0] == '@') {
    qmlCliParseResponseFile(&arg[1]);
    return;
  }

  if(arg[0] != '-') {
    qmlCliAddPos(arg);
    return;
  }

  while(arg[0] == '-') {
    ++arg;
  }

  int nameLen = cliCliGetParamNameLen(arg);
  if(nameLen == -1) {
    qmlCliAddFlag(arg);
  } else {
    qmlCliAddParam(arg, nameLen);
  }
}

void qmlCliParse(int argc, char **argv)
{
  qmlCliWinInit();
  qmlCliMemInit();

  gCli.programName = NULL;
  gCli.posCount = 0;
  gCli.flagCount = 0;
  gCli.paramCount = 0;

  if(argc < 1) {
    return; // ???
  }

  gCli.programName = argv[0];
  for(int i = 1; i < argc; ++i) {
    qmlCliAddArg(argv[i]);
  }
}

char *qmlCliProgramName(void)
{
  return gCli.programName;
}

int qmlCliPosC(void)
{
  return gCli.posCount;
}

char *qmlCliPos(int i)
{
  if(i < gCli.posCount) {
    return gCli.posArgs[i];
  }
  return NULL;
}

int qmlCliFlag(char *flag)
{
  for(int i = 0; i < gCli.flagCount; ++i) {
    if(strcmp(gCli.flags[i], flag) == 0) {
      return 1;
    }
  }
  return 0;
}

char *qmlCliParam(char *param)
{
  for(int i = 0; i < gCli.paramCount; ++i) {
    qmlCliParamPair *pair = &gCli.params[i];
    for(int j = 0; j < pair->nameLen; ++j) {
      if(param[j] != pair->data[j]) {
        goto next_arg;
      }
    }
    if(param[pair->nameLen] == '\0') {
      return pair->data + pair->nameLen + 1;
    }
  next_arg:
    ;
  }
  return NULL;
}
