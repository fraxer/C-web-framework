{* include /views/header *}

Hello {{name}}, {{first}} a!
.{{ object . deep_object . name }}.
.{{ object.array1[2] }}.
.{{ object . deep_object . моя переменная }}.
.{{ object.array2[0].array[1][1].name }}.
.{{ object.array3[1][1] }}.

{% if if_true %}
    if_true 1
{% endif %}

{% if !if_false %}
    !if_false 1
{% endif %}

{% if if_true %}    if is true
{% elseif !if_true %}    elseifff
{% elseif !if_true %}    elseifff
{% else %}    elseeee
{% endif %}


root variable в котором будет первый уровень текста
каждое вхождение тега увеличивает уровень вложенности



      {{ tag }}       {% if %} text3 {% endif %}
text1           text2                            text4

                                      {{ tag }}
      {{ tag }}       {% for %} text3           text5 {% endfor %}
text1           text2                                              text5

                                                     {{ tag }}
                                      {% if %} text4           text5 {% endif %}
      {{ tag }}       {% for %} text3                                            text7 {% endfor %}
text1           text2                                                                               text8


      {* include /views/header *}
text1                             text2



typedef struct dataitem {
    jsontok_t* json_token;

    struct dataitem* prev;
    struct dataitem* next;
} dataitem_t;

typedef struct datalist {
    dataitem_t* dataitem;
    dataitem_t* last_dataitem;
} datalist_t;

typedef struct tag {
    int type; // var, cond, loop, inc
    datalist_t* datalist;

    struct tag* next;
} tag_t;

typedef struct variable {
    tag_t base;

    variable_item_t* item;
    variable_item_t* last_item;
} variable_t;

typedef struct condition_item {
    int reverse;

    variable_item_t* item;
    variable_item_t* last_item;
} condition_item_t;

typedef struct condition {
    tag_t base;

    condition_item_t* item;
    condition_item_t* last_item;
} variable_t;

typedef struct loop {
    tag_t base;

    char* element_name;
    char* key_name;

    variable_item_t* item;
    variable_item_t* last_item;
} variable_t;

typedef struct include {
    tag_t base;

    char* path;
} variable_t;