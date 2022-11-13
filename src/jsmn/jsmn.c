#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  #include <stdio.h>
#include "jsmn.h"

static const size_t TOKENS_BUFFER_SIZE = 4096;

void jsmn_insert_symbol(jsmn_parser_t *parser);

/**
 * Increase\decrease token pool
 */
int jsmn_alloc_tokens(jsmn_parser_t *parser, size_t tokens_count) {

  jsmntok_t* tokens = (jsmntok_t*)realloc(parser->tokens, sizeof(jsmntok_t) * tokens_count);

  if (tokens == NULL) {
    return -1;
  }

  parser->tokens = tokens;
  parser->tokens_count = tokens_count;

  return 0;
}

/**
 * Allocates a fresh unused token from the token pool.
 */
jsmntok_t *jsmn_alloc_token(jsmn_parser_t *parser) {
  jsmntok_t *tok;

  if (parser->toknext >= parser->tokens_count) {
    if (jsmn_alloc_tokens(parser, parser->tokens_count + TOKENS_BUFFER_SIZE) == -1) return NULL;
  }

  tok = &parser->tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
  tok->parent = -1;
  tok->index = parser->toknext - 1;
  tok->string_len = 0;
  tok->string = NULL;
  tok->child = NULL;
  tok->sibling = NULL;
  tok->last_sibling = NULL;
  return tok;
}

/**
 * Fills token type and boundaries.
 */
void jsmn_fill_token(jsmntok_t *token, const jsmntype_t type, const int start, jsmn_parser_t *parser) {
  token->type = type;
  token->start = start;
  token->end = parser->pos;
  token->size = 0;
  token->string = &parser->string_internal[start];
  token->string_len = parser->pos - start;

  // printf("%.*s\n", parser->pos - start, token->string);

  token->string[token->string_len] = 0;

  // printf("%s\n", token->string);
}

void jsmn_set_child_or_sibling(jsmn_parser_t* parser, jsmntok_t* token) {
  if (!parser->tokens[parser->toksuper].child) {
    parser->tokens[parser->toksuper].child = token;
    parser->tokens[parser->toksuper].last_sibling = token;
  } else {

    jsmntok_t* token_child = parser->tokens[parser->toksuper].last_sibling;

    token_child->sibling = token;

    parser->tokens[parser->toksuper].last_sibling = token;
  }
}

/**
 * Fills next available token with JSON primitive.
 */
int jsmn_parse_primitive(jsmn_parser_t *parser) {
  jsmntok_t *token;
  int start;
  int start_internal;

  start = parser->dirty_pos;
  start_internal = parser->pos;

  for (; parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
    switch (parser->string[parser->dirty_pos]) {
    /* In strict mode primitive must be followed by "," or "}" or "]" */
    case ':':
    case '\t':
    case '\r':
    case '\n':
    case ' ':
    case ',':
    case ']':
    case '}':
      goto found;
    default:
                   /* to quiet a warning from gcc*/
      break;
    }
    if (parser->string[parser->dirty_pos] < 32 || parser->string[parser->dirty_pos] >= 127) {
      parser->dirty_pos = start;
      return JSMN_ERROR_INVAL;
    }

    jsmn_insert_symbol(parser);
  }
  /* In strict mode primitive must be followed by a comma/object/array */
  parser->dirty_pos = start;
  return JSMN_ERROR_PART;

  found:
  if (parser->tokens == NULL) {
    parser->dirty_pos--;
    return 0;
  }
  token = jsmn_alloc_token(parser);
  if (token == NULL) {
    parser->dirty_pos = start;
    return JSMN_ERROR_NOMEM;
  }
  jsmn_fill_token(token, JSMN_PRIMITIVE, start_internal, parser);
  token->parent = parser->toksuper;
  parser->dirty_pos--;

  jsmn_set_child_or_sibling(parser, token);

  /* Skip symbol, because jsmn_fill_token write \0 to end string */
  parser->pos++;

  return 0;
}

/**
 * Fills next token with JSON string.
 */
int jsmn_parse_string(jsmn_parser_t *parser) {
  jsmntok_t *token;

  int start = parser->dirty_pos;
  int start_internal = parser->pos;

  jsmn_insert_symbol(parser);
  
  /* Skip starting quote */
  parser->dirty_pos++;
  
  for (; parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
    char c = parser->string[parser->dirty_pos];

    /* Quote: end of string */
    if (c == '\"') {
      if (parser->tokens == NULL) {
        return 0;
      }

      token = jsmn_alloc_token(parser);

      if (token == NULL) {
        parser->dirty_pos = start;
        return JSMN_ERROR_NOMEM;
      }

      jsmn_fill_token(token, JSMN_STRING, start_internal + 1, parser);

      token->parent = parser->toksuper;

      jsmn_set_child_or_sibling(parser, token);

      /* Skip symbol, because jsmn_fill_token write \0 to end string */
      parser->pos++;

      return 0;
    }

    /* Backslash: Quoted symbol expected */
    if (c == '\\' && parser->dirty_pos + 1 < parser->string_len) {
      int i;
      parser->dirty_pos++;
      switch (parser->string[parser->dirty_pos]) {
      /* Allowed escaped symbols */
      case '\"':
      case '/':
      case 'b':
      case 'f':
      case 'r':
      case 'n':
      case 't':
        break;
      case '\\':
        break;
      /* Allows escaped symbol \uXXXX */
      case 'u':
        parser->dirty_pos++;
        for (i = 0; i < 4 && parser->dirty_pos < parser->string_len && parser->string[parser->dirty_pos] != '\0'; i++) {
          /* If it isn't a hex character we have an error */
          if (!((parser->string[parser->dirty_pos] >= 48 && parser->string[parser->dirty_pos] <= 57) ||   /* 0-9 */
                (parser->string[parser->dirty_pos] >= 65 && parser->string[parser->dirty_pos] <= 70) ||   /* A-F */
                (parser->string[parser->dirty_pos] >= 97 && parser->string[parser->dirty_pos] <= 102))) { /* a-f */
            parser->dirty_pos = start;
            return JSMN_ERROR_INVAL;
          }
          parser->dirty_pos++;
        }
        parser->dirty_pos--;
        break;
      /* Unexpected symbol */
      default:
        parser->dirty_pos = start;
        return JSMN_ERROR_INVAL;
      }
    }

    jsmn_insert_symbol(parser);
  }
  parser->dirty_pos = start;
  return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int jsmn_parse(jsmn_parser_t *parser) {
  int r;
  int i;
  jsmntok_t *token;
  int count = parser->toknext;
  size_t len = strlen(parser->string);

  for (; parser->dirty_pos < len && parser->string[parser->dirty_pos] != '\0'; parser->dirty_pos++) {
    char c;
    jsmntype_t type;

    c = parser->string[parser->dirty_pos];

    switch (c) {
    case '{':
    case '[':
      count++;
      if (parser->tokens == NULL) {
        break;
      }
      token = jsmn_alloc_token(parser);
      if (token == NULL) {
        return JSMN_ERROR_NOMEM;
      }
      if (parser->toksuper != -1) {
        jsmntok_t *t = &parser->tokens[parser->toksuper];
        /* In strict mode an object or array can't become a key */
        if (t->type == JSMN_OBJECT) {
          return JSMN_ERROR_INVAL;
        }
        t->size++;
        token->parent = parser->toksuper;

        jsmn_set_child_or_sibling(parser, token);
      }
      token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
      token->start = parser->dirty_pos;
      parser->toksuper = parser->toknext - 1;
      jsmn_insert_symbol(parser);
      break;
    case '}':
    case ']':
      if (parser->tokens == NULL) {
        break;
      }
      type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);

      if (parser->toknext < 1) {
        return JSMN_ERROR_INVAL;
      }
      token = &parser->tokens[parser->toknext - 1];
      for (;;) {
        if (token->start != -1 && token->end == -1) {
          if (token->type != type) {
            return JSMN_ERROR_INVAL;
          }
          token->end = parser->dirty_pos + 1;
          parser->toksuper = token->parent;
          break;
        }
        if (token->parent == -1) {
          if (token->type != type || parser->toksuper == -1) {
            return JSMN_ERROR_INVAL;
          }
          break;
        }
        token = &parser->tokens[token->parent];
      }
      jsmn_insert_symbol(parser);

      break;
    case '\"':
      r = jsmn_parse_string(parser);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && parser->tokens != NULL) {
        parser->tokens[parser->toksuper].size++;
      }
      break;
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    case ':':
      parser->toksuper = parser->toknext - 1;
      jsmn_insert_symbol(parser);
      break;
    case ',':
      if (parser->tokens != NULL && parser->toksuper != -1 &&
          parser->tokens[parser->toksuper].type != JSMN_ARRAY &&
          parser->tokens[parser->toksuper].type != JSMN_OBJECT) {
        parser->toksuper = parser->tokens[parser->toksuper].parent;
      }
      jsmn_insert_symbol(parser);
      break;
    /* In strict mode primitives are: numbers and booleans */
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 't':
    case 'f':
    case 'n':
      /* And they must not be keys of the object */
      if (parser->tokens != NULL && parser->toksuper != -1) {
        const jsmntok_t *t = &parser->tokens[parser->toksuper];
        if (t->type == JSMN_OBJECT ||
            (t->type == JSMN_STRING && t->size != 0)) {
          return JSMN_ERROR_INVAL;
        }
      }

      r = jsmn_parse_primitive(parser);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && parser->tokens != NULL) {
        parser->tokens[parser->toksuper].size++;
      }
      break;

    /* Unexpected char in strict mode */
    default:
      return JSMN_ERROR_INVAL;
    }
  }

  if (parser->tokens != NULL) {
    for (i = parser->toknext - 1; i >= 0; i--) {
      /* Unmatched opened object or array */
      if (parser->tokens[i].start != -1 && parser->tokens[i].end == -1) {
        return JSMN_ERROR_PART;
      }
    }
  }

  // printf("string: %s\n", parser->string_internal);

  return count;
}

void jsmn_insert_symbol(jsmn_parser_t *parser) {
    parser->string_internal[parser->pos] = parser->string[parser->dirty_pos];

    // char c = parser->string[parser->dirty_pos];

    // printf("ins: %c\n", c);
    // printf("%d, %d, %.*s\n", parser->pos, parser->string_internal[parser->pos], parser->pos, parser->string_internal);

    parser->pos++;
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
int jsmn_init(jsmn_parser_t *parser, const char* string) {
  parser->dirty_pos = 0;
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;

  size_t string_len = strlen(string);

  parser->string_internal = (char*)malloc(string_len + 1);
  parser->string = string;
  parser->string_len = string_len;

  parser->tokens = NULL;
  parser->tokens_count = 0;

  if (jsmn_alloc_tokens(parser, TOKENS_BUFFER_SIZE) == -1) return -1;

  return 0;
}

void jsmn_free(jsmn_parser_t *parser) {
  if (parser->string_internal) {
    free(parser->string_internal);
  }

  if (parser->tokens) {
    free(parser->tokens);
  }

  parser->dirty_pos = 0;
  parser->pos = 0;
  parser->toknext = 0;
  parser->tokens_count = 0;
  parser->toksuper = 0;
  parser->string_len = 0;
  parser->string = NULL;
  parser->string_internal = NULL;
  parser->tokens = NULL;
}

jsmntok_t* jsmn_get_root_token(jsmn_parser_t* parser) {
  if (parser->tokens) {
    return &parser->tokens[0];
  }

  return NULL;
}

int jsmn_field_string(jsmntok_t* token, const char *str) {
  if (token->type == JSMN_STRING && strlen(str) == token->string_len &&
      strncmp(token->string, str, token->string_len) == 0) {
    return 1;
  }

  return 0;
}

int jsmn_is_object(jsmntok_t* token) {
  return token && token->type == JSMN_OBJECT;
}

jsmntok_t* jsmn_object_get_field(jsmntok_t* token, const char* field) {
  if (token) {

    jsmntok_t* token_it = token->child;

    while (token_it) {
      if (jsmn_field_string(token_it, field)) {
        return token_it->child;
      }

      token_it = token_it->sibling;
    }

    // for (int i = 0; i < 15; i++) {
    //   jsmntok_t* token = &parser->tokens[i];

    //   printf("str, %.*s, index: %d, parent: %d, addr: %p, child: %p, sibling: %p\n", token->end - token->start, parser->string + token->start, token->index, token->parent, token, token->child, token->sibling);
    // }
  }

  return NULL;
}

const char* jsmn_get_value(jsmntok_t* token) {
  if (token && (token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE)) {
    return token->string;
  }

  return NULL;
}

const char* jsmn_get_array_value(jsmntok_t* token, int index) {
  if (token && token->type == JSMN_ARRAY && index >= 0 && index < token->size) {
    int i = 0;

    jsmntok_t* token_it = token->child;

    while (token_it) {
      if (i == index) {
        return jsmn_get_value(token_it);
      }

      token_it = token_it->sibling;

      i++;
    }
  }

  return NULL;
}