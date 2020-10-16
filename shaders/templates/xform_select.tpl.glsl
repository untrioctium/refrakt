int get_xform_id(float ratio) {

{% for xform in xforms %}
	{% if loop.is_first %}
	float sum = fp[{{xform.weight}}];
	if(sum >= ratio) return {{loop.index}};
	{% else if loop.is_last %}
	return {{loop.index}};
	{% else %}
	sum += fp[{{xform.weight}}];
	if(sum >= ratio) return {{loop.index}};
	{% endif %}
{% endfor %}
}