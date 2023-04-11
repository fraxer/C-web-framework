#ifndef JSMN_H
#define JSMN_H

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
  JSMN_UNDEFINED = 0,
  JSMN_OBJECT = 1,
  JSMN_ARRAY = 2,
  JSMN_STRING = 3,
  JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr {
  /* Not enough tokens were provided */
  JSMN_ERROR_NOMEM = -1,
  /* Invalid character inside JSON string */
  JSMN_ERROR_INVAL = -2,
  /* The string is not a full JSON packet, more bytes expected */
  JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct jsmntok {
  jsmntype_t type;
  int start;
  int end;
  int size;
  int parent;
  int index;
  size_t string_len;
  char* string;
  struct jsmntok* child;
  struct jsmntok* sibling;
  struct jsmntok* last_sibling;
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct jsmn_parser {
  unsigned int dirty_pos;    /* offset in the JSON string */
  unsigned int pos;          
  unsigned int toknext;      /* next token to allocate */
  unsigned int tokens_count; /* tokens count */
  int toksuper;     /* superior token node, e.g. parent object or array */
  size_t string_len;
  const char* string;
  char* string_internal;
  jsmntok_t* tokens;
} jsmn_parser_t;

/**
 * Create JSON parser over an array of tokens
 */
int jsmn_init(jsmn_parser_t* parser, const char* string);

/**
 * Free internal memory
 */
void jsmn_free(jsmn_parser_t* parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing
 * a single JSON object.
 */
int jsmn_parse(jsmn_parser_t* parser);

void jsmn_set_child_or_sibling(jsmn_parser_t* parser, jsmntok_t* token);

jsmntok_t* jsmn_get_root_token(const jsmn_parser_t* parser);

int jsmn_is_object(const jsmntok_t* token);

jsmntok_t* jsmn_object_get_field(const jsmntok_t* token, const char* field);

jsmntok_t* jsmn_object_find_key(const jsmntok_t* token, const char* field);

const char* jsmn_get_value(const jsmntok_t* token);

const char* jsmn_get_array_value(const jsmntok_t* token, int index);

#endif