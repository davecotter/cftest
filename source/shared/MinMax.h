//	MinMax.h
#ifndef _H_MinMax
#define	_H_MinMax

#define	math_min(a, b)		((a < b) ? a : b)
#define	math_max(a, b)		((a > b) ? a : b)

#define	kLogTenOfTwo		3.321928094887362347870319429489390175864831
#define	math_log2(_n)		(log10((float)_n) * kLogTenOfTwo)
#define math_exp2(_n)		(pow(2.0f, (float)_n))

#define HAVE_MATH_ROUND
#define math_round(_foo) 	((_foo) >= 0 ? ((_foo) + 0.5f) : ((_foo) - 0.5f))

#define math_ceil(_foo) 	ceil(_foo)

int		math_gcd(int a, int b);
void	math_reduce(int& x, int& y);


#endif	//	_H_MinMax
