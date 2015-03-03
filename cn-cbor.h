#ifndef CN_CBOR_H
#define CN_CBOR_H

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

typedef enum cn_cbor_type {
  CN_CBOR_NULL,
  CN_CBOR_FALSE,   CN_CBOR_TRUE,
  CN_CBOR_UINT,    CN_CBOR_INT,
  CN_CBOR_BYTES,   CN_CBOR_TEXT,
  CN_CBOR_BYTES_CHUNKED,   CN_CBOR_TEXT_CHUNKED, /* += 2 */
  CN_CBOR_ARRAY,   CN_CBOR_MAP,
  CN_CBOR_TAG,
  CN_CBOR_SIMPLE,  CN_CBOR_DOUBLE,
  CN_CBOR_INVALID
} cn_cbor_type;

typedef enum cn_cbor_flags {
  CN_CBOR_FL_COUNT = 1,
  CN_CBOR_FL_INDEF = 2,
  CN_CBOR_FL_OWNER = 0x80,            /* of str */
} cn_cbor_flags;

typedef struct cn_cbor {
  cn_cbor_type type;
  cn_cbor_flags flags;
  union {
    const char* str;
    long sint;
    unsigned long uint;
    double dbl;
    unsigned long count;        /* for use during filling */
  } v;                          /* TBD: optimize immediate */
  int length;
  struct cn_cbor* first_child;
  struct cn_cbor* last_child;
  struct cn_cbor* next;
  struct cn_cbor* parent;
} cn_cbor;

typedef enum cn_cbor_error {
  CN_CBOR_NO_ERROR,
  CN_CBOR_ERR_OUT_OF_DATA,
  CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED,
  CN_CBOR_ERR_ODD_SIZE_INDEF_MAP,
  CN_CBOR_ERR_BREAK_OUTSIDE_INDEF,
  CN_CBOR_ERR_MT_UNDEF_FOR_INDEF,
  CN_CBOR_ERR_RESERVED_AI,
  CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING,
  CN_CBOR_ERR_OUT_OF_MEMORY,
} cn_cbor_error;

extern const char *cn_cbor_error_str[];

typedef struct cn_cbor_errback {
  int pos;
  cn_cbor_error err;
} cn_cbor_errback;

const cn_cbor* cn_cbor_decode(const char* buf, size_t len, cn_cbor_errback *errp);
const cn_cbor* cn_cbor_mapget_string(const cn_cbor* cb, const char* key);
const cn_cbor* cn_cbor_mapget_int(const cn_cbor* cb, int key);
const cn_cbor* cn_cbor_index(const cn_cbor* cb, int idx);

const cn_cbor* cn_cbor_alloc(cn_cbor_type t);
void cn_cbor_free(const cn_cbor* js);

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_H */
