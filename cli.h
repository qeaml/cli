/*
cli.h
---------
CLI parser, v1.0
*/

#ifndef QML_CLI_H_
#define QML_CLI_H_

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
 * @brief Initialize the terminal/console.
 *
 * Only affects Windows. Called by qmlCliParse().
 */
QML_CLI_API
void qmlCliInit(void);

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
#define cliInit qmlCliInit
#define cliParse qmlCliParse
#define cliProgramName qmlCliProgramName
#define cliPosC qmlCliPosC
#define cliPos qmlCliPos
#define cliFlag qmlCliFlag
#define cliParam qmlCliParam
#endif

#endif