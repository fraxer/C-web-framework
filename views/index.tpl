{% for item in object.array2[0].array[0] %}key: {{index}} - {{item.name}} - {{item.sur}}  {% if item.bool %} bool true {% endif %} asd
{% endfor %}

{% for item in object.array1 %}key: {{index}} - {{item}}
{% endfor %}

{% for item, idx in object.array1 %}key: {{idx}} - {{item}}
{% endfor %}

{% for item, idx in object.array1 %}key: {{idx}} - {{item}}
{% endfor %}

{% for item, idx in arr %}key: {{idx}}
    {% for item2, idx2 in item.list %}key: {{idx}} - {{idx2}} - {{item.name}} - {{item2}}
    {% endfor %}
{% endfor %}