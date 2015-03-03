#ifndef CN_CBOR_C
#define CN_CBOR_C

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <arpa/inet.h>

#include "cn-cbor.h"
#include "cbor.h"

// can be redefined, e.g. for pool allocation
#ifndef CN_CBOR_CALLOC
#define CN_CBOR_CALLOC() calloc(1, sizeof(cn_cbor))
#define CN_CBOR_FREE(cb) free((void*)(cb))
#endif

#define CN_CBOR_FAIL(code) do { pb->err = code;  goto fail; } while(0)

 const char *cn_cbor_error_str[] = {
  "CN_CBOR_NO_ERROR",
  "CN_CBOR_ERR_OUT_OF_DATA",
  "CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED",
  "CN_CBOR_ERR_ODD_SIZE_INDEF_MAP",
  "CN_CBOR_ERR_BREAK_OUTSIDE_INDEF",
  "CN_CBOR_ERR_MT_UNDEF_FOR_INDEF",
  "CN_CBOR_ERR_RESERVED_AI",
  "CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING",
  "CN_CBOR_ERR_OUT_OF_MEMORY"
};

void cn_cbor_free(const cn_cbor* cb) {
  cn_cbor* p = (cn_cbor*) cb;
  while (p) {
    cn_cbor* p1;
    while ((p1 = p->first_child)) { /* go down */
      p = p1;
    }
    if (!(p1 = p->next)) {   /* go up next */
      if ((p1 = p->parent))
        p1->first_child = 0;
    }
    CN_CBOR_FREE(p);
    p = p1;
  }
}

static double decode_half(int half) {
  int exp = (half >> 10) & 0x1f;
  int mant = half & 0x3ff;
  double val;
  if (exp == 0) val = ldexp(mant, -24);
  else if (exp != 31) val = ldexp(mant + 1024, exp - 25);
  else val = mant == 0 ? INFINITY : NAN;
  return half & 0x8000 ? -val : val;
}

/* Fix these if you can't do non-aligned reads */
#define ntoh8p(p) (*(unsigned char*)(p))
#define ntoh16p(p) (ntohs(*(unsigned short*)(p)))
#define ntoh32p(p) (ntohl(*(unsigned long*)(p)))
static uint64_t ntoh64p(unsigned char *p) {
  uint64_t ret = ntoh32p(p);
  ret <<= 32;
  ret += ntoh32p(p+4);
  return ret;
}

static cn_cbor_type mt_trans[] = {
  CN_CBOR_UINT,    CN_CBOR_INT,
  CN_CBOR_BYTES,   CN_CBOR_TEXT,
  CN_CBOR_ARRAY,   CN_CBOR_MAP,
  CN_CBOR_TAG,     CN_CBOR_SIMPLE,
};

struct parse_buf {
  unsigned char *buf;
  unsigned char *ebuf;
  cn_cbor_error err;
};

#define TAKE(pos, ebuf, n, stmt)                \
  if (n > (size_t)(ebuf - pos))                 \
    CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_DATA);      \
  stmt;                                         \
  pos += n;

static cn_cbor *decode_item (struct parse_buf *pb, cn_cbor* top_parent) {
  unsigned char *pos = pb->buf;
  unsigned char *ebuf = pb->ebuf;
  cn_cbor* parent = top_parent;
  int ib;
  unsigned int mt;
  int ai;
  uint64_t val;
  cn_cbor* cb = NULL;
  union {
    float f;
    uint32_t u;
  } u32;
  union {
    double d;
    uint64_t u;
  } u64;

again:
  TAKE(pos, ebuf, 1, ib = ntoh8p(pos) );
  if (ib == IB_BREAK) {
    if (!(parent->flags & CN_CBOR_FL_INDEF))
      CN_CBOR_FAIL(CN_CBOR_ERR_BREAK_OUTSIDE_INDEF);
    switch (parent->type) {
    case CN_CBOR_BYTES: case CN_CBOR_TEXT:
      parent->type += 2;            /* CN_CBOR_* -> CN_CBOR_*_CHUNKED */
      break;
    case CN_CBOR_MAP:
      if (parent->length & 1)
        CN_CBOR_FAIL(CN_CBOR_ERR_ODD_SIZE_INDEF_MAP);
    default:;
    }
    goto complete;
  }
  mt = ib >> 5;
  ai = ib & 0x1f;
  val = ai;

  cb = CN_CBOR_CALLOC();
  if (!cb)
    CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);

  cb->type = mt_trans[mt];

  cb->parent = parent;
  if (parent->last_child) {
    parent->last_child->next = cb;
  } else {
    parent->first_child = cb;
  }
  parent->last_child = cb;
  parent->length++;

  switch (ai) {
  case AI_1: TAKE(pos, ebuf, 1, val = ntoh8p(pos)) ; break;
  case AI_2: TAKE(pos, ebuf, 2, val = ntoh16p(pos)) ; break;
  case AI_4: TAKE(pos, ebuf, 4, val = ntoh32p(pos)) ; break;
  case AI_8: TAKE(pos, ebuf, 8, val = ntoh64p(pos)) ; break;
  case 28: case 29: case 30: CN_CBOR_FAIL(CN_CBOR_ERR_RESERVED_AI);
  case AI_INDEF:
    if ((mt - MT_BYTES) <= MT_MAP) {
      cb->flags |= CN_CBOR_FL_INDEF;
      goto push;
    } else {
      CN_CBOR_FAIL(CN_CBOR_ERR_MT_UNDEF_FOR_INDEF);
    }
  }
  // process content
  switch (mt) {
  case MT_UNSIGNED:
    cb->v.uint = val;           /* to do: Overflow check */
    break;
  case MT_NEGATIVE:
    cb->v.sint = ~val;          /* to do: Overflow check */
    break;
  case MT_BYTES: case MT_TEXT:
    cb->v.str = (char *) pos;
    cb->length = val;
    TAKE(pos, ebuf, val, );
    break;
  case MT_MAP:
    val <<= 1;
    /* fall through */
  case MT_ARRAY:
    if ((cb->v.count = val)) {
      cb->flags |= CN_CBOR_FL_COUNT;
      goto push;
    }
    break;
  case MT_TAG:
    cb->v.uint = val;
    goto push;
  case MT_PRIM:
    switch (ai) {
    case VAL_NIL: cb->type = CN_CBOR_NULL; break;
    case VAL_FALSE: cb->type = CN_CBOR_FALSE; break;
    case VAL_TRUE: cb->type = CN_CBOR_TRUE; break;
    case AI_2: cb->type = CN_CBOR_DOUBLE; cb->v.dbl = decode_half(val); break;
    case AI_4:
      cb->type = CN_CBOR_DOUBLE;
      u32.u = val;
      cb->v.dbl = u32.f;
      break;
    case AI_8:
      cb->type = CN_CBOR_DOUBLE;
      u64.u = val;
      cb->v.dbl = u64.d;
      break;
    default: cb->v.uint = val;
    }
  }
fill:                           /* emulate loops */
  if (parent->flags & CN_CBOR_FL_INDEF) {
    if (parent->type == CN_CBOR_BYTES || parent->type == CN_CBOR_TEXT)
      if (cb->type != parent->type)
          CN_CBOR_FAIL(CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING);
    goto again;
  }
  if (parent->flags & CN_CBOR_FL_COUNT) {
    if (--parent->v.count)
      goto again;
  }
  /* so we are done filling parent. */
complete:                       /* emulate return from call */
  if (parent == top_parent) {
    if (pos != ebuf)            /* XXX do this outside */
      CN_CBOR_FAIL(CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED);
    pb->buf = pos;
    return cb;
  }
  cb = parent;
  parent = parent->parent;
  goto fill;
push:                           /* emulate recursive call */
  parent = cb;
  goto again;
fail:
  pb->buf = pos;
  return 0;
}

const cn_cbor* cn_cbor_decode(const char* buf, size_t len, cn_cbor_errback *errp) {
  cn_cbor catcher = {CN_CBOR_INVALID, 0, {0}, 0, NULL, NULL, NULL, NULL};
  struct parse_buf pb = {(unsigned char *)buf, (unsigned char *)buf+len, CN_CBOR_NO_ERROR};
  cn_cbor* ret = decode_item(&pb, &catcher);
  if (ret != NULL) {
    /* mark as top node */
    ret->parent = NULL;
  } else {
    if (catcher.first_child) {
      catcher.first_child->parent = 0;
      cn_cbor_free(catcher.first_child);
    }
    if (errp) {
      errp->err = pb.err;
      errp->pos = pb.buf - (unsigned char *)buf;
    }
    return NULL;
  }
  return ret;
}

const cn_cbor* cn_cbor_mapget_int(const cn_cbor* cb, int key) {
  cn_cbor* cp;
  assert(cb);
  for (cp = cb->first_child; cp && cp->next; cp = cp->next->next) {
    switch(cp->type) {
    case CN_CBOR_UINT:
      if (cp->v.uint == (unsigned long)key) {
        return cp->next;
      }
    case CN_CBOR_INT:
      if (cp->v.sint == (long)key) {
        return cp->next;
      }
      break;
    default:
      ; // skip non-integer keys
    }
  }
  return NULL;
}

const cn_cbor* cn_cbor_mapget_string(const cn_cbor* cb, const char* key) {
  cn_cbor *cp;
  int keylen;
  assert(cb);
  assert(key);
  keylen = strlen(key);
  for (cp = cb->first_child; cp && cp->next; cp = cp->next->next) {
    switch(cp->type) {
    case CN_CBOR_TEXT:
      if (keylen != cp->length) {
        continue;
      }
      if (strncmp(key, cp->v.str, cp->length) == 0) {
        return cp->next;
      }
      break;
    case CN_CBOR_BYTES:
      if (keylen != cp->length) {
        continue;
      }
      if (memcmp(key, cp->v.str, keylen) == 0) {
        return cp->next;
      }
    default:
      ; // skip non-string keys
    }
  }
  return NULL;
}

const cn_cbor* cn_cbor_index(const cn_cbor* cb, int idx) {
  cn_cbor *cp;
  int i = 0;
  assert(cb);
  for (cp = cb->first_child; cp; cp = cp->next) {
    if (i == idx) {
      return cp;
    }
    i++;
  }
  return NULL;
}

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
