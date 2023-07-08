#ifndef _ZEPHYR_CONST_H
#define _ZEPHYR_CONST_H


/*
 * This returns a constant expression while determining if an argument is
 * a constant expression, most importantly without evaluating the argument.
 * Glory to Martin Uecker <Martin.Uecker@med.uni-goettingen.de>
 */
#define __is_constexpr(x) \
	(sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

/*
 * Force a compilation error if condition is true, but also produce a
 * result (of value 0 and type int), so the expression can be used
 * e.g. in a structure initializer (or where-ever else comma expressions
 * aren't permitted).
 */
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int:(-!!(e)); })))

/* This macro is defined in util.h
#define GENMASK_INPUT_CHECK(h, l) \
	(BUILD_BUG_ON_ZERO(__builtin_choose_expr( \
		__is_constexpr((l) > (h)), (l) > (h), 0)))


#define GENMASK_UL(h, l) \
	(GENMASK_INPUT_CHECK(h, l) + __GENMASK_UL(h, l))


#define __GENMASK(h, l) (((~0) - (1 << (l)) + 1) & (~0 >> (32 - 1 - (h))))

#define GENMASK(h, l) (GENMASK_INPUT_CHECK(h, l) + __GENMASK(h, l))

*/
#endif /* _ZEPHYR_CONST_H */
