#define QML_CLI_SHORTHANDS
#include "cli.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_PASSED \
  fprintf(stderr, SGR_GREEN "%s passed\n" SGR_RESET, __FUNCTION__)
#define TEST_CASE(...) \
  cliParse(1 + sizeof((char *[]){__VA_ARGS__}) / sizeof(char *), (char *[]){"test", __VA_ARGS__})

void testPosArgs()
{
  TEST_CASE("arg1", "arg2");

  assert(cliProgramName() != NULL);
  assert(strcmp(cliProgramName(), "test") == 0);
  assert(cliPosC() == 2);
  assert(strcmp(cliPos(0), "arg1") == 0);
  assert(strcmp(cliPos(1), "arg2") == 0);

  TEST_PASSED;
}

void testFlags()
{
  TEST_CASE("-f", "--flag");

  assert(cliFlag("f"));
  assert(cliFlag("flag"));
  assert(!cliFlag("x"));
  assert(!cliFlag("no-flag"));

  TEST_PASSED;
}

void testParams()
{
  TEST_CASE("-p=value", "--param=othervalue", "------param-with-dashes=123");

  assert(strcmp(cliParam("p"), "value") == 0);
  assert(strcmp(cliParam("param"), "othervalue") == 0);
  assert(strcmp(cliParam("param-with-dashes"), "123") == 0);
  assert(cliParam("x") == NULL);

  TEST_PASSED;
}

void testParamsAndFlags()
{
  TEST_CASE("-f", "--flag", "-p=value");

  assert(cliFlag("f"));
  assert(cliFlag("flag"));
  assert(!cliFlag("x"));
  assert(!cliFlag("no-flag"));
  assert(strcmp(cliParam("p"), "value") == 0);
  assert(cliParam("flag") == NULL);

  TEST_PASSED;
}

void testAll()
{
  TEST_CASE("arg1", "arg2", "-f", "--flag", "-p=value", "--param=othervalue");

  assert(cliProgramName() != NULL);
  assert(strcmp(cliProgramName(), "test") == 0);
  assert(cliPosC() == 2);
  assert(strcmp(cliPos(0), "arg1") == 0);
  assert(strcmp(cliPos(1), "arg2") == 0);
  assert(cliFlag("f"));
  assert(cliFlag("flag"));
  assert(!cliFlag("x"));
  assert(!cliFlag("no-flag"));
  assert(strcmp(cliParam("p"), "value") == 0);
  assert(strcmp(cliParam("param"), "othervalue") == 0);
  assert(cliParam("x") == NULL);

  TEST_PASSED;
}

int main(int argc, char *argv[])
{
  /* use default memory functions */
  qmlCliSetMemoryFuncs(NULL, NULL, NULL);
  testPosArgs();
  testFlags();
  testParams();
  testParamsAndFlags();
  testAll();
  qmlCliFree();
  return 0;
}
