#ifndef DOXTER_CHALLENGES_H
#define DOXTER_CHALLENGES_H

/* Intentionally eclectic header to stress comment→symbol association and tag parsing. */
/* Non-ASCII in comments for encoding checks: São, naïve, façade, über. */

#ifdef _WIN32
  #define API __declspec(dllexport)
  #define CALL __stdcall
#else
  #define API
  #define CALL
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define ATTR_NONNULL_1 __attribute__((nonnull(1)))
  #define ATTR_MALLOC    __attribute__((malloc))
  #define ATTR_PURE      __attribute__((pure))
#else
  #define ATTR_NONNULL_1
  #define ATTR_MALLOC
  #define ATTR_PURE
#endif

/** 
 * @brief A function-like macro with a do-while body.
 * @param x value to print
 * @note Backslash-continued lines must be treated as one logical definition.
 */
#define DOX_PRINT(x) do {           \
  puts(#x);                         \
} while (0)

/**
 * @brief Backslash-continued macro with a trailing backslash on an empty line.
 * @note Some scanners accidentally stop at the first '\n'.
 */
#define DOX_ADD(a,b) ((a) + (b))    \
/**/

/**
 * @brief A simple constant-like macro.
 */
#define DOX_PI 3.14159265358979323846

// This string literal should not start or end a doc comment:
static const char* kTrickyString =
  "/** not a comment start */"
  " still not a comment "
  " end marker: */";

/**
 * @brief An enum typedef with multiple declared names.
 * @note Tools often capture only the first name; check both.
 */
typedef enum DoxColorTag
{
  DOX_RED   = 1,
  DOX_GREEN = 2,
  DOX_BLUE  = 4
} DoxColor, DoxPaint;

/**
 * @brief Anonymous enum assigned to a typedef.
 * @note There is no tag name; only the typedef name exists.
 */
typedef enum
{
  DOX_Small = 0,
  DOX_Large = 1
} DoxSizeClass;

/**
 * @brief Named struct then instance declaration on the same line.
 * @note The typedef name is separate from the tag name.
 */
struct DoxPoint
{
  int x;
  int y;
};

/**
 * @brief Anonymous struct with typedef; also declares two variables.
 * @note Some tools must decide how to anchor this: struct vs typedef vs variables.
 */
typedef struct
{
  float w, h;
} DoxSize, *DoxSizePtr, DoxSizeArray[2];

/**
 * @brief Function pointer typedef with argument names.
 * @param user user data pointer
 * @return int status code
 */
typedef int (*DoxCallback)(void* user);

/**
 * @brief Function pointer returning a pointer to function (spicy).
 * @return pointer to a callback with (void*) → int
 */
typedef int (*(*DoxMetaFactory)(void))(void*);

/**
 * @brief API macro + attributes + storage qualifiers before the function name.
 * @param a first operand
 * @param b second operand
 * @return sum of a and b
 */
API static inline ATTR_PURE int
dox_add(int a, int b)
{
  return a + b;
}

/**
 * @brief Attributes after return type, before name, with calling convention.
 * @param p memory to zero
 * @param n number of bytes
 * @return pointer p
 */
API void CALL ATTR_NONNULL_1
dox_bzero(void* p, unsigned n)
{
  unsigned i = 0u;
  for (; i < n; ++i)
  {
    ((unsigned char*) p)[i] = 0u;
  }
}

/**
 * @brief Mixed qualifiers, arrays, restrict, and const on parameters.
 * @param dst output buffer (n elements)
 * @param src input buffer  (n elements)
 * @param n   element count
 * @return dst
 */
API float* 
dox_axpy(float* restrict dst, const float* restrict src, float a, int n)
{
  for (int i = 0; i < n; ++i)
  {
    dst[i] = a * src[i] + dst[i];
  }
  return dst;
}

/**
 * @brief Variadic function with a function-pointer parameter.
 * @param cb callback invoked with user
 * @param user user data
 * @return last printed length (implementation detail)
 */
int
dox_logf(DoxCallback cb, void* user, const char* fmt, ...);

/**
 * @brief A declaration split awkwardly over multiple lines, with tabs.
 * @param count number of items
 * @return 0 on success
 */
API int
dox_multiline_decl
(	int		count	);

/**
 * @brief Function returning a function pointer (declaration only).
 * @return pointer to a function (void*)→int
 */
DoxCallback
dox_make_callback_factory(void);

/**
 * @brief Struct with bitfields, anonymous union, and trailing comma in enum.
 * @note Bitfield width and anonymous union often trip signature slicing.
 */
typedef struct DoxPacked
{
  unsigned kind:3;
  unsigned flags:5;
  union
  {
    DoxSize  size;
    DoxPoint point;
  };
} DoxPacked;

/**
 * @brief Macro that expands to a declaration with attributes.
 * @note Some tools should probably ignore targets created by macros.
 */
#if defined(__GNUC__) || defined(__clang__)
  #define DOX_INLINE_PURE static inline __attribute__((pure))
#else
  #define DOX_INLINE_PURE static inline
#endif

/**
 * @brief Inline helper declared via attribute macro.
 * @param x input
 * @return x*x
 */
DOX_INLINE_PURE int
dox_square(int x)
{
  return x * x;
}

/**
 * @brief A macro with parentheses and commas inside; ensure tokenization doesn’t choke. */
#define DOX_TUPLE(a,b) a, b

/** @brief Declaration that uses a macro which expands to multiple declarators. */
int
dox_take_tuple(int DOX_TUPLE(first, second));

/**
 * @brief A typedef to an array type; some parsers misreport the name.
 * @note The symbol of interest is the typedef name, not the element type.
 */
typedef int DoxTenInts[10];

/**
 * @brief Complex declarator: pointer to array of 5 pointers to function(int)→int.
 * @note The name is 'dox_fptr_arr', the type is intentionally dense.
 */
int (*(*dox_fptr_arr)[5])(int);

/**
 * @brief Nested parens and line breaks in a prototype.
 * @param cb callback
 * @param user user data
 * @return int
 */
int
dox_call_cb
(
  int (*cb)(void* user),
  void* user
);

/**
 * @brief Enum with trailing comma and explicit values.
 */
typedef enum
{
  DOX_A = 10,
  DOX_B = 20,
  DOX_C = 30,
} DoxEnumTrailingComma;

/**
 * @brief K&R-style spacing (still valid C), to test whitespace normalization.
 * @param a a
 * @param b b
 */
API int
dox_weird_ws(  int   a ,   int   b  )
{
  return a - b;
}

/**
 * @brief Documentation for a macro that looks like a function.
 * @param f function-like thing
 */
#define DOX_WRAP(f) (f)

/** @brief A declaration preceded by a single-line comment that contains '/**'.
 *  The tool must attach this block (not the line above) to the target.
 */
// /** fake opener */
int dox_single_line_noise(void);

/** @brief Simple definition to ensure codegen includes at least one real body. */
int
dox_logf(DoxCallback cb, void* user, const char* fmt, ...)
{
  (void) cb;
  (void) user;
  (void) fmt;
  return 0;
}

#endif /* DOXTER_CHALLENGES_H */
