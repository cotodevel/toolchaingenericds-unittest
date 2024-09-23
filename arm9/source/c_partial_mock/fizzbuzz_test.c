#include "fizzbuzz_test.h"
#include "fizzbuzz.h"
#include "opmock.h"
#include <stdlib.h>
#include "posixHandleTGDS.h"
#include "WoopsiTemplate.h"

static int test_fizzbuzz_with_15_callback (char *  sound, int calls)
{
  if((strcmp(sound, "FIZZ") == 0) 
     || (strcmp(sound, "BUZZ") == 0)
     || (strcmp(sound, "FIZZBUZZ") == 0))
    return 0;
  return 1;
}

void test_fizzbuzz_with_15()
{
  char *res = fizzbuzz(15);
  OP_ASSERT_EQUAL_CSTRING("FIZZBUZZ", res);
  TGDSARM9Free(res);
  OP_VERIFY();
}

void test_fizzbuzz_many_3()
{
  int i;
  for(i = 1; i < 1000; i++) {
    if((i % 3 == 0) && ((i % 5) != 0)) {   
      char *res = fizzbuzz(i);
      OP_ASSERT_EQUAL_CSTRING("FIZZ", res);
      TGDSARM9Free(res);
    }
  }
  OP_VERIFY();
}

void test_fizzbuzz_many_5()
{
  int i;
  for(i = 1; i < 1000; i++) {
    if((i % 3 != 0) && ((i % 5) == 0)) {
      char *res = fizzbuzz(i);
      OP_ASSERT_EQUAL_CSTRING("BUZZ", res);
      TGDSARM9Free(res);
    }
  }
  OP_VERIFY();
}
