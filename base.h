#ifndef _BASE_H_
#define _BASE_H_

// marker for inline functions
// if _INLINE_ is attached to the function before its name declared
// that function is always inlined by compiler
#ifndef _INLINE_
#define _INLINE_ inline __attribute__((always_inline))
#endif

#endif
