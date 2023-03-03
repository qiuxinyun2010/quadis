#if __GNUC__ >= 4
#define redis_unreachable __builtin_unreachable
#else
#define redis_unreachable abort
#endif

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif