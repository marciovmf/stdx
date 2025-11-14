#include <stdio.h>

/* Intentionally eclectic header to stress comment→symbol association and tag parsing. */
/* Non-ASCII in comments for encoding checks: São, naïve, façade, über. */


/**
 * @brief Backslash-continued macro with a trailing backslash on an empty line.
 * @note Some scanners accidentally stop at the first '\n'.
 */
#define DOX_ADD(a,b) ((a) + (b))    \
/**/


/**
 * Computes the highest value between two values.
 * @param a first value
 * however the description continues here.
 * @param b second value
 * @return the highest value between a and b
 */
#define MAX(a, b) (a > b ? a : b)


/**
 * @brief HELLO! Backslash-continued macro with a trailing backslash on an empty line.
 * @note Some scanners accidentally stop at the first '\n'.
 */
#define DOX_ADD(a,b) ((a) + (b))    \
/**/


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
 * A simple point struct
 */
struct Point
{
  int x, y, z;
};

/** 
 * This function returns a pointer to a function causing it's declaration to be
 * messy and hard to parse. Our code should correctly identify this is a
 * function and even get it's name.
 */
void (*my_crazy_function(void))(int, int);

/**
 * @brief Main function. Notice that I can add an sign on the description without
 * confusing the parser.
 * @param argc number or arguments.
 * @param argv       array of arguments
 * @return 0 on success, non zero otherwise
 */
int main(int argc, char** argv)
{
  printf("Hello, World!\n");
  return 0;
}


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
