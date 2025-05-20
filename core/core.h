#ifndef _CORE_H
#define _CORE_H

#define COREAPI

// _countof equivalence
#define lenof(a) (sizeof(a) / sizeof(*(a)))


// Memory Shorthand
#define alloc(n,obj) malloc((n) * sizeof(*(obj)))
#define allocz(n,obj) calloc((n), sizeof(*(obj)))
#define allocr(n,obj) realloc((obj), (n) * sizeof(*(obj)))

// Error Handling
enum ErrorEnum {
	ERR_FALSE = -1,
	ERR_OK,
	ERR_MEMORYOUT,
	ERR_OPENFILEFAILED,
	ERR_INVALIDFILE,
	ERR_INVALIDARG,
	ERR_CUSTOM
};
typedef unsigned error_t, err_t;


#define BEGIN err_t _e = ERR_OK;
#define END return _e;
#define OK(err) ((err) <= 0)
#define ERR(err) ((err) > 0)
#define THROW(err,lbl) do { _e = (err); goto lbl; } while (0)

#define ASSERT(expr,err,lbl) if (!(expr)) THROW(err, lbl)
#define ASSERT_ERR(expr,lbl) do { err_t _1 = (expr); ASSERT(OK(_1), _1, lbl); } while (0)
#define ASSERT_ARG(expr) if (!(expr)) return ERR_INVALIDARG
#define ASSERT_MEM(mem,lbl) ASSERT(mem, ERR_MEMORYOUT, lbl)

#endif // _CORE_H
