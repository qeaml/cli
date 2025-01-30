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

void testResponse()
{
  TEST_CASE("non-response", "@response.txt");

  assert(cliProgramName() != NULL);
  assert(strcmp(cliProgramName(), "test") == 0);
  assert(cliPosC() == 8);
  assert(strcmp(cliPos(0), "non-response") == 0);
  assert(strcmp(cliPos(1), "response") == 0);
  assert(strcmp(cliPos(7), "foo bar") == 0);
  assert(cliFlag("flag-too"));
  assert(cliFlag("1203"));
  assert(!cliFlag("non-response"));
  assert(strcmp(cliParam("param"), "123") == 0);
  assert(strcmp(cliParam("quote"), "\"") == 0);
  assert(cliParam("non-response") == NULL);

  TEST_PASSED;
}

int main(int argc, char *argv[])
{
  /* use default memory functions */
  cliSetMemoryFuncs(NULL, NULL, NULL);
  testPosArgs();
  testFlags();
  testParams();
  testParamsAndFlags();
  testAll();
  testResponse();
  cliFree();
  return 0;
}
