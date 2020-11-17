//	MinMax.cpp

#include "stdafx.h"
#include "MinMax.h"

int		math_gcd(int a, int b)
{
	if (b == 0) {
		return a;
	}

	a %= b;

	return math_gcd(b,a);
}

void		math_reduce(int& x, int& y)
{
	int d(math_gcd(x, y));

	x /= d;
	y /= d;
}
