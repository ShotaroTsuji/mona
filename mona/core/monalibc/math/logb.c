#include <math.h>
#include "../intrinsics.h"
#include "ieee_754.h"

double logb(double x)
{
	union __nc_ieee_754_double u;
	int offset = 0;
	int exp = 0;

	u.value = x;

	if( x == 0.0 ) return -HUGE_VAL;
	else if( isinf(x) ) return INFINITY;
	else if( isnan(x) ) return x;
	else if( isnormal(x) ) return (double)(u.n.exp-1023);

	// Then, x is subnormal.
	if( u.n.frac0 )
	  offset = __bitscanreverse((int32_t)u.n.frac0)+32;
	if( u.n.frac1 )
	  offset = __bitscanreverse((int32_t)u.n.frac1);

	exp = u.n.exp - 1023;
	offset -= 52;
	return (double)(exp+offset+1);
}
