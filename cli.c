/* CLI v1.0 */

#include "cli.h"
#include <string.h>

#ifndef QML_CLI_NO_STDLIB
#include <stdlib.h>
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
  }
  if(gCli.flagCap > 0 && gCli.flags != NULL) {
    gCli.free(gCli.flags);
  }
  if(gCli.paramCap > 0 && gCli.params != NULL) {
    gCli.free(gCli.params);
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
    char *arg = argv[i];

    if(arg[0] != '-') {
      qmlCliAddPos(arg);
      continue;
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
