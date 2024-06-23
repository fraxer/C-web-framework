{* include /header.tpl *}

{% if users[0].authorized %}
user <{{users[0].name}}> authorized
{% elseif users[1].authorized %}
user <{{users[1].name}}> authorized
{% elseif users[2].authorized %}
user <{{users[2].name}}> authorized
{% else %}
nothing authorized users
{%endif%}

Users:
{% for user, index in users %}
    Id: {{index}}
    Name: {{user.name}}
    Authorized: {% if user.authorized %}authorized{% else %}not authorized{% endif %}
    Tasks:
    {% for task, index in user.tasks %}
        #{{index}}: {{task}}
    {% endfor %}
{% endfor %}