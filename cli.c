/* CLI v1.0 */

#include "cli.h"
#include <string.h>

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
  int posArgC;
  char *posArgV[QML_CLI_MAX_POS_ARGS];
  int flagC;
  char *flagV[QML_CLI_MAX_FLAGS];
  int paramC;
  qmlCliParamPair paramV[QML_CLI_MAX_PARAMS];
} qmlCli;

static qmlCli gCli;

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
  if(gCli.posArgC < QML_CLI_MAX_POS_ARGS) {
    gCli.posArgV[gCli.posArgC++] = arg;
  }
}

static void qmlCliAddFlag(char *arg)
{
  if(gCli.flagC < QML_CLI_MAX_FLAGS) {
    gCli.flagV[gCli.flagC++] = arg;
  }
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
  if(gCli.paramC < QML_CLI_MAX_PARAMS) {
    qmlCliParamPair *param = &gCli.paramV[gCli.paramC++];
    param->data = arg;
    param->nameLen = nameLen;
  }
}

void qmlCliParse(int argc, char **argv)
{
#ifdef _WIN32
  qmlCliWinInit();
#endif

  gCli.programName = NULL;
  gCli.posArgC = 0;
  gCli.flagC = 0;
  gCli.paramC = 0;

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
  return gCli.posArgC;
}

char *qmlCliPos(int i)
{
  if(i < gCli.posArgC) {
    return gCli.posArgV[i];
  }
  return NULL;
}

int qmlCliFlag(char *flag)
{
  for(int i = 0; i < gCli.flagC; ++i) {
    if(strcmp(gCli.flagV[i], flag) == 0) {
      return 1;
    }
  }
  return 0;
}

char *qmlCliParam(char *param)
{
  for(int i = 0; i < gCli.paramC; ++i) {
    qmlCliParamPair *pair = &gCli.paramV[i];
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