#ifndef LIB_CTYPE
#define LIB_CTYPE

#define ROUND_UP(X, STEP) (((X) + (STEP)-1) / (STEP) * (STEP))
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

static inline int islower(int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static inline int isalpha(int c) { return islower(c) || isupper(c); }
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
static inline int isxdigit(int c) {
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int isspace(int c) {
  return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}
static inline int isblank(int c) { return c == ' ' || c == '\t'; }
static inline int isgraph(int c) { return c > 32 && c < 127; }
static inline int isprint(int c) { return c >= 32 && c < 127; }
static inline int iscntrl(int c) { return (c >= 0 && c < 32) || c == 127; }
static inline int isascii(int c) { return c >= 0 && c < 128; }
static inline int ispunct(int c) { return isprint(c) && !isalnum(c) && !isspace(c); }

static inline int tolower(int c) { return isupper(c) ? c - 'A' + 'a' : c; }
static inline int toupper(int c) { return islower(c) ? c - 'a' + 'A' : c; }

#define MAX(x, y)   ((x) > (y) ? (x) : (y))
#define MIN(x, y)   ((x) < (y) ? (x) : (y))

#endif /* LIB_CTYPE */