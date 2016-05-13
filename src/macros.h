#ifndef MACROS_H
#define MACROS_H
/*
	counts number of arguments; supports 0 args
	not exactly standard-compliant
 */
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(, ##__VA_ARGS__, 10,9,8,7,6,5,4,3,2,1,0)
#define VA_NUM_ARGS_IMPL(_0,_1,_2,_3,_4,_5,_6,_7,_8_,_9,_10,N,...) N



/*
	gets the length of an array
*/
/* 0[x] guards against [] overloading, the division against using with a pointer */
#define REDFST_LEN(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))



/*
	compile-time assert
*/
#define REDFST_CASSERT(condition) ((void)sizeof(char[1 - 2*!(condition)]))



#define REDFST_LIKELY(x)    __builtin_expect (!!(x), 1)
#define REDFST_UNLIKELY(x)  __builtin_expect (!!(x), 0)

#endif
