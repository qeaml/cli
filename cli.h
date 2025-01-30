/*
cli.h
---------
CLI parser, v1.0
*/

#ifndef QML_CLI_H_
#define QML_CLI_H_

#include <stddef.h>

#ifndef QML_CLI_API
#define QML_CLI_API
#endif

#ifndef QML_CLI_NO_ESCAPE
/* ANSI escape codes */
/* reference: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences */
#define ESC "\x1b"
#define SGR(no) ESC "[" #no "m"
#define SGR_RESET SGR(0)
#define SGR_BOLD SGR(1)
#define SGR_RED SGR(31)
#define SGR_GREEN SGR(32)
#define SGR_YELLOW SGR(33)
#define SGR_BLUE SGR(34)
#define SGR_MAGENTA SGR(35)
#define SGR_CYAN SGR(36)
#define SGR_WHITE SGR(37)
#endif

#define QML_CLI_MAX_POS_ARGS 256
#define QML_CLI_MAX_FLAGS    256
#define QML_CLI_MAX_PARAMS   256

/**
 * @brief Initialize the Windows Console.
 *
 * Called by qmlCliParse().
 */
QML_CLI_API
void qmlCliWinInit(void);

typedef void*(*qmlCliAllocPfn)(size_t);
typedef void*(*qmlCliReallocPfn)(void*,size_t);
typedef void(*qmlCliFreePfn)(void*);

/**
 * @brief Set the memory management functions used by the library.
 *
 * If you call this function but leave any of the three function pointers as
 * `NULL`, then the appropriate standard library function will be used.
 *
 * Calling this function allows the library to dynamically allocate and
 * reallocate memory as needed. This comes with some upsides:
 *
 * - Effectively infinite arguments (the library limits you to 256 of each by
 *   default)
 *
 * But it has its downsides too:
 *
 * - You must call qmlCliFree() once you're done, otherwise the memory allocated
 *   by the library will leak.
 *
 * @param allocPfn Custom allocation function or `NULL` for `malloc()`
 * @param reallocPfn Custom reallocation function or `NULL` for `realloc()`
 * @param freePfn Custom free function or `NULL` for `free()`
 */
QML_CLI_API
void qmlCliSetMemoryFuncs(qmlCliAllocPfn allocPfn, qmlCliReallocPfn reallocPfn, qmlCliFreePfn freePfn);

/**
 * @brief Free memory allocated by the library.
 *
 * This only has to be called if qmlCliSetMemoryFuncs() was called beforehand.
 * All data is stored in static memory otherwise.
 */
void qmlCliFree();

/**
 * @brief Parse command-line.
 *
 * @param argc Argument count
 * @param argv Argument array
 */
QML_CLI_API
void qmlCliParse(int argc, char *argv[]);

/**
 * @brief Get the program name.
 *
 * Returns NULL if qmlCliParse() has not been called.
 *
 * @return char* Pointer to program name or NULL
 */
QML_CLI_API
char *qmlCliProgramName(void);

/**
 * @brief Get the amount of positional arguments.
 *
 * @return int Positional argument count
 */
QML_CLI_API
int qmlCliPosC(void);

/**
 * @brief Get the value of a positional argument.
 *
 * @param i Argument index
 * @return char* Pointer to value or NULL
 */
QML_CLI_API
char *qmlCliPos(int i);

/**
 * @brief Check for a flag's presence.
 *
 * @param flag Flag name
 * @return int 1 if present, 0 otherwise
 */
QML_CLI_API
int qmlCliFlag(char *flag);

/**
 * @brief Get the value of a parameter.
 *
 * @param param Parameter name
 * @return char* Pointer to value or NULL if not found
 */
QML_CLI_API
char *qmlCliParam(char *param);

#ifdef QML_CLI_SHORTHANDS
#define cliWinInit qmlCliWinInit
#define cliParse qmlCliParse
#define cliProgramName qmlCliProgramName
#define cliPosC qmlCliPosC
#define cliPos qmlCliPos
#define cliFlag qmlCliFlag
#define cliParam qmlCliParam
#endif

#endif