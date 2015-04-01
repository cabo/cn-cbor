/**
 * \file
 * \brief
 * CBOR parsing
 */

#ifndef CN_CBOR_H
#define CN_CBOR_H

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <stdint.h>

/**
 * All of the different kinds of CBOR values.
 */
typedef enum cn_cbor_type {
  /** false */
  CN_CBOR_FALSE,
  /** true */
  CN_CBOR_TRUE,
  /** null */
  CN_CBOR_NULL,
  /** undefined */
  CN_CBOR_UNDEF,
  /** Positive integers */
  CN_CBOR_UINT,
  /** Negative integers */
  CN_CBOR_INT,
  /** Byte string */
  CN_CBOR_BYTES,
  /** UTF-8 string */
  CN_CBOR_TEXT,
  /** Byte string, in chunks.  Each chunk is a child. */
  CN_CBOR_BYTES_CHUNKED,
  /** UTF-8 string, in chunks.  Each chunk is a child */
  CN_CBOR_TEXT_CHUNKED,
  /** Array of CBOR values.  Each array element is a child, in order */
  CN_CBOR_ARRAY,
  /** Map of key/value pairs.  Each key and value is a child, alternating. */
  CN_CBOR_MAP,
  /** Tag describing the next value.  The next value is the single child. */
  CN_CBOR_TAG,
  /** Simple value, other than the defined ones */
  CN_CBOR_SIMPLE,
  /** Doubles, floats, and half-floats */
  CN_CBOR_DOUBLE,
  /** An error has occurred */
  CN_CBOR_INVALID
} cn_cbor_type;

/**
 * Flags used during parsing.  Not useful for consumers of the
 * `cn_cbor` structure.
 */
typedef enum cn_cbor_flags {
  /** The count field will be used for parsing */
  CN_CBOR_FL_COUNT = 1,
  /** An indefinite number of children */
  CN_CBOR_FL_INDEF = 2,
  /** Not used yet; the structure must free the v.str pointer when the
     structure is freed */
  CN_CBOR_FL_OWNER = 0x80,            /* of str */
} cn_cbor_flags;

/**
 * A CBOR value
 */
typedef struct cn_cbor {
  /** The type of value */
  cn_cbor_type type;
  /** Flags used at parse time */
  cn_cbor_flags flags;
  /** Data associated with the value; different branches of the union are
      used depending on the `type` field. */
  union {
    /** CN_CBOR_BYTES, CN_CBOR_TEXT */
    const char* str;
    /** CN_CBOR_INT */
    long sint;
    /** CN_CBOR_UINT */
    unsigned long uint;
    /** CN_CBOR_DOUBLE */
    double dbl;
    /** for use during parsing */
    unsigned long count;
  } v;                          /* TBD: optimize immediate */
  /** Number of children.
    * @note: for maps, this is 2x the number of entries */
  int length;
  /** The first child value */
  struct cn_cbor* first_child;
  /** The last child value */
  struct cn_cbor* last_child;
  /** The sibling after this one, or NULL if this is the last */
  struct cn_cbor* next;
  /** The parent of this value, or NULL if this is the root */
  struct cn_cbor* parent;
} cn_cbor;

/**
 * All of the different kinds of errors
 */
typedef enum cn_cbor_error {
  /** No error has occurred */
  CN_CBOR_NO_ERROR,
  /** More data was expected while parsing */
  CN_CBOR_ERR_OUT_OF_DATA,
  /** Some extra data was left over at the end of parsing */
  CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED,
  /** A map should be alternating keys and values.  A break was found
      when a value was expected */
  CN_CBOR_ERR_ODD_SIZE_INDEF_MAP,
  /** A break was found where it wasn't expected */
  CN_CBOR_ERR_BREAK_OUTSIDE_INDEF,
  /** Indefinite encoding works for bstrs, strings, arrays, and maps.
      A different major type tried to use it. */
  CN_CBOR_ERR_MT_UNDEF_FOR_INDEF,
  /** Additional Information values 28-30 are reserved */
  CN_CBOR_ERR_RESERVED_AI,
  /** A chunked encoding was used for a string or bstr, and one of the elements
      wasn't the expected (string/bstr) type */
  CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING,
  /** An invalid parameter was passed to a function */
  CN_CBOR_ERR_INVALID_PARAMETER,
  /** Allocation failed */
  CN_CBOR_ERR_OUT_OF_MEMORY
} cn_cbor_error;

/**
 * Strings matching the `cn_cbor_error` conditions.
 *
 * @todo: turn into a function to make the type safety more clear?
 */
extern const char *cn_cbor_error_str[];

/**
 * Errors
 */
typedef struct cn_cbor_errback {
  /** The position in the input where the erorr happened */
  int pos;
  /** The error, or CN_CBOR_NO_ERROR if none */
  cn_cbor_error err;
} cn_cbor_errback;

#ifdef USE_CBOR_CONTEXT

/**
 * Allocate and zero out memory.  `count` elements of `size` are required,
 * as for `calloc(3)`.  The `context` is the `cn_cbor_context` passed in
 * earlier to the CBOR routine.
 *
 * @param[in] count   The number of items to allocate
 * @param[in] size    The size of each item
 * @param[in] context The allocation context
 */
typedef void* (*cn_calloc_func)(size_t count, size_t size, void *context);

/**
 * Free memory previously allocated with a context.  If using a pool allocator,
 * this function will often be a no-op, but it must be supplied in order to
 * prevent the CBOR library from calling `free(3)`.
 *
 * @note: it may be that this is never needed; if so, it will be removed for
 * clarity and speed.
 *
 * @param  context [description]
 * @return         [description]
 */
typedef void (*cn_free_func)(void *ptr, void *context);

/**
 * The allocation context.
 */
typedef struct cn_cbor_context {
    /** The pool `calloc` routine.  Must allocate and zero. */
    cn_calloc_func calloc_func;
    /** The pool `free` routine.  Often a no-op, but required. */
    cn_free_func  free_func;
    /** Typically, the pool object, to be used when calling `calloc_func`
      * and `free_func` */
    void *context;
} cn_cbor_context;

/** When USE_CBOR_CONTEXT is defined, many functions take an extra `context`
  * parameter */
#define CBOR_CONTEXT , cn_cbor_context *context
/** When USE_CBOR_CONTEXT is defined, some functions take an extra `context`
  * parameter at the beginning */
#define CBOR_CONTEXT_COMMA cn_cbor_context *context,

#else

#define CBOR_CONTEXT
#define CBOR_CONTEXT_COMMA

#endif

/**
 * Decode an array of CBOR bytes into structures.
 *
 * @param[in]  buf          The array of bytes to parse
 * @param[in]  len          The number of bytes in the array
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out] errp         Error, if NULL is returned
 * @return                  The parsed CBOR structure, or NULL on error
 */
const cn_cbor* cn_cbor_decode(const uint8_t *buf, size_t len CBOR_CONTEXT, cn_cbor_errback *errp);

/**
 * Get a value from a CBOR map that has the given string as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The string to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
const cn_cbor* cn_cbor_mapget_string(const cn_cbor* cb, const char* key);

/**
 * Get a value from a CBOR map that has the given integer as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The int to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
const cn_cbor* cn_cbor_mapget_int(const cn_cbor* cb, int key);

/**
 * Get the item with the given index from a CBOR array.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  idx          The array index
 * @return                  The matching value, or NULL if the index is invalid
 */
const cn_cbor* cn_cbor_index(const cn_cbor* cb, int idx);

/**
 * Free the given CBOR structure.
 *
 * @param[in]  cb           The CBOR value to free
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 */
void cn_cbor_free(const cn_cbor* cb CBOR_CONTEXT);

/**
 * Write a positive integer, along with type information.
 *
 * @param[in]  buf        The buffer into which to write
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer
 * @param[in]  typ        Which type to write
 * @param[in]  val        The value to write
 * @return                -1 on fail, or number of bytes written
 */
ssize_t cbor_encoder_write_positive(uint8_t *buf,
                                    size_t buf_offset,
                                    size_t buf_size,
                                    cn_cbor_type typ,
                                    uint64_t val);

/**
 * Write a negative number.  No type information is needed for negative numbers.
 *
 * @param[in]  buf        The buffer into which to write
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer
 * @param[in]  val        The value to write
 * @return                -1 on fail, or number of bytes written
 */
ssize_t cbor_encoder_write_negative(uint8_t *buf,
                                    size_t buf_offset,
                                    size_t buf_size,
                                    int64_t val);

/**
 * Write floating point number.
 *
 * @param[in]  buf        The buffer into which to write
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer
 * @param[in]  val        The value to write
 * @return                -1 on fail, or number of bytes written
 */
ssize_t cbor_encoder_write_double(uint8_t *buf,
                                  size_t buf_offset,
                                  size_t buf_size,
                                  double val);

/**
 * Write a CBOR value and all of the child values.
 *
 * @param[in]  buf        The buffer into which to write
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer
 * @param[in]  cb         [description]
 * @return                -1 on fail, or number of bytes written
 */
ssize_t cbor_encoder_write(uint8_t *buf,
                           size_t buf_offset,
                           size_t buf_size,
                           const cn_cbor *cb);

cn_cbor* cn_cbor_map_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp);

cn_cbor* cn_cbor_data_create(const char* data, int len
                             CBOR_CONTEXT,
                             cn_cbor_errback *errp);

cn_cbor* cn_cbor_string_create(const char* data
                               CBOR_CONTEXT,
                               cn_cbor_errback *errp);

cn_cbor* cn_cbor_int_create(int64_t value
                            CBOR_CONTEXT,
                            cn_cbor_errback *errp);

void cn_cbor_mapput_int(cn_cbor* cb_map,
                        int64_t key, cn_cbor* cb_value
                        CBOR_CONTEXT,
                        cn_cbor_errback *errp);

void cn_cbor_mapput_string(cn_cbor* cb_map,
                           char* key, cn_cbor* cb_value
                           CBOR_CONTEXT,
                           cn_cbor_errback *errp);

cn_cbor* cn_cbor_array_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp);

void cn_cbor_array_append(cn_cbor* cb_array,
                          cn_cbor* cb_value,
                          cn_cbor_errback *errp);

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_H */
