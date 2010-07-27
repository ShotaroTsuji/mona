#include <monapi.h>
#define MUNIT_GLOBAL_VALUE_DEFINED
#include <monapi/MUnit.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <fenv.h>


int test_log();
int test_intr();
int test_fpu();

int main(int argc, char* argv[])
{
    if (argc > 1) {
        printf(argv[1]);
        return -2;
    } else {
        test_fpu();
        test_log();
	test_intr();

        TEST_RESULTS(math);
        return 0;
    }
}

extern "C" double logb(double);

int test_log()
{
    double r = 0.0;

    r = log(0.0);
    EXPECT_TRUE(isinf(r));

    r = log(-INFINITY);
    EXPECT_TRUE(r == 0.0);

    r = log(-1.0);
    EXPECT_TRUE(r == 0.0);

    r = log(INFINITY);
    EXPECT_TRUE(isinf(r));

    r = logb(1.0);
    EXPECT_TRUE( r == 0.0 );
    r = logb(0x1.2p+10);
    EXPECT_TRUE( r == 10.0 );
    r = logb(0x1.0p-1074);
    EXPECT_TRUE( r == -1074.0 );
    r = logb(0x1.0p-1064);
    EXPECT_TRUE( r == -1064.0 );
    r = logb(0x1.0p-1030);
    EXPECT_TRUE( r == -1030.0 );

    return 0;
}

extern "C" int __bitscanforward(int32_t x);
extern "C" int __bitscanreverse(int32_t x);
extern "C" int __byteswap(int32_t x);
extern "C" int __bittest(int32_t base, int offset);

int test_intr()
{
  int r;

  r = __bitscanforward(0x00000010);
  EXPECT_TRUE(r == 4);
  r = __bitscanforward(0x00000001);
  EXPECT_TRUE(r == 0);
  r = __bitscanforward(0x00000004);
  EXPECT_TRUE(r == 2);
  r = __bitscanforward(0x80000000);
  EXPECT_TRUE(r == 31);
  r = __bitscanforward(0x00100040);
  EXPECT_TRUE(r == 6);

  r = __bitscanreverse(0x00000010);
  EXPECT_TRUE(r == 4);
  r = __bitscanreverse(0x00000001);
  EXPECT_TRUE(r == 0);
  r = __bitscanreverse(0x00000004);
  EXPECT_TRUE(r == 2);
  r = __bitscanreverse(0x80000000);
  EXPECT_TRUE(r == 31);
  r = __bitscanreverse(0x00100040);
  EXPECT_TRUE(r == 20);
  r = __bitscanreverse(0x30100040);
  EXPECT_TRUE(r == 29);

  r = __byteswap(0x11223344);
  EXPECT_TRUE(r == 0x44332211);
  r = __byteswap(0xcafe0000);
  EXPECT_TRUE(r == 0x0000feca);

  r = __bittest(0x00000001, 0);
  EXPECT_TRUE(r);
  r = __bittest(0x00000001, 5);
  EXPECT_TRUE(!r);
  r = __bittest(0x00000030, 5);
  EXPECT_TRUE(r);
  r = __bittest(0xFFFFFFFF, 13);
  EXPECT_TRUE(r);
  r = __bittest(0xFFFEFFFF, 16);
  EXPECT_TRUE(!r);
  r = __bittest(0x80000000, 31);
  EXPECT_TRUE(r);

  return 0;
}

int test_fpu()
{
  fenv_t env;
  double r = 1.0e+100;

  fegetenv(&env);
  _printf("FPU/x87\n");
  _printf("Controll Word: %x\n", env.__control);
  _printf("Status Word  : %x\n", env.__status);
  _printf("Tag Word     : %x\n", env.__tag);

  _printf("Test #O\n");
  fprintf(stdout, "r = %f\n", r);
  for( int i = 0 ; i < 200 ; i++ ) r *= r;
  fegetenv(&env);
  _printf("Status Word  : %x\n", env.__status);
  fprintf(stdout, "r = %f\n", r);

  _printf("Test #U\n");
  r = 1.0e-100;
  for( int i = 0 ; i < 200 ; i++ ) r *= r;
  fegetenv(&env);
  _printf("Status Word  : %x\n", env.__status);
  fprintf(stdout, "r = %f\n", r);

  _printf("Test #Z\n");
  r = 0.0;
  r = r/r;
  fegetenv(&env);
  _printf("Status Word  : %x\n", env.__status);
  fprintf(stdout, "r = %f\n", r);

  return 0;
}
