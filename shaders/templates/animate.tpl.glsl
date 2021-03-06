#version 460

layout(std430,binding = 5) buffer parameter_buffer
{
	float fp[];
};

layout(std430, binding = 9) buffer fp_inflated_buffer
{
	float fp_inflated[];
};

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

uniform float temporal_sample_width = 0.0416666667;
uniform float rads_per_second = 0.31415926535;

void main() {
	int half_width = int(gl_NumWorkGroups.x * gl_WorkGroupSize.x / 2);
	int sample_pos = int(gl_GlobalInvocationID.x) - half_width;
	float dt = sample_pos / float(half_width) * temporal_sample_width;

	int inflated_offset = int(gl_GlobalInvocationID.x) * {{size}};

	{% for xform in xforms %}
	// xform {{loop.index}}
	/* weight */      fp_inflated[inflated_offset + {{xform.weight}}] = fp[{{xform.weight}}];
	/* color */       fp_inflated[inflated_offset + {{xform.color}}] = fp[{{xform.color}}];
	/* color_speed */ fp_inflated[inflated_offset + {{xform.color_speed}}] = fp[{{xform.color_speed}}];
	/* opacity */     fp_inflated[inflated_offset + {{xform.opacity}}] = fp[{{xform.opacity}}];

	{% for param,offset in xform.param %}
	// {{param}}
	fp_inflated[inflated_offset + {{offset}}] = fp[{{offset}}];
	{% endfor %}

	{% for var,offset in xform.variations %}
	// {{var}}
	fp_inflated[inflated_offset + {{offset}}] = fp[{{offset}}];
	{% endfor %}

	float sino_{{loop.index}} = sin(rads_per_second * dt * fp[{{xform.rotation_frequency}}]);
	float coso_{{loop.index}} = cos(rads_per_second * dt * fp[{{xform.rotation_frequency}}]);
	/* affine a */ fp_inflated[inflated_offset + {{xform.affine.0}}] = fp[{{xform.affine.0}}] * coso_{{loop.index}} + fp[{{xform.affine.2}}] * sino_{{loop.index}};
	/* affine b */ fp_inflated[inflated_offset + {{xform.affine.1}}] = fp[{{xform.affine.1}}] * coso_{{loop.index}} + fp[{{xform.affine.3}}] * sino_{{loop.index}};
	/* affine c */ fp_inflated[inflated_offset + {{xform.affine.2}}] = fp[{{xform.affine.2}}] * coso_{{loop.index}} - fp[{{xform.affine.0}}] * sino_{{loop.index}};
	/* affine d */ fp_inflated[inflated_offset + {{xform.affine.3}}] = fp[{{xform.affine.3}}] * coso_{{loop.index}} - fp[{{xform.affine.1}}] * sino_{{loop.index}};
	/* affine e */ fp_inflated[inflated_offset + {{xform.affine.4}}] = fp[{{xform.affine.4}}];
	/* affine f */ fp_inflated[inflated_offset + {{xform.affine.5}}] = fp[{{xform.affine.5}}];

	{% if existsIn(xform, "post") %}
	fp_inflated[inflated_offset + {{xform.post.0}}] = fp[{{xform.post.0}}];
	fp_inflated[inflated_offset + {{xform.post.1}}] = fp[{{xform.post.1}}];
	fp_inflated[inflated_offset + {{xform.post.2}}] = fp[{{xform.post.2}}];
	fp_inflated[inflated_offset + {{xform.post.3}}] = fp[{{xform.post.3}}];
	fp_inflated[inflated_offset + {{xform.post.4}}] = fp[{{xform.post.4}}];
	fp_inflated[inflated_offset + {{xform.post.5}}] = fp[{{xform.post.5}}];
	{% endif %}
	{% endfor %}

	{% if exists("final_xform") %}
		// xform final
	/* weight */      fp_inflated[inflated_offset + {{final_xform.weight}}] = fp[{{final_xform.weight}}];
	/* color */       fp_inflated[inflated_offset + {{final_xform.color}}] = fp[{{final_xform.color}}];
	/* color_speed */ fp_inflated[inflated_offset + {{final_xform.color_speed}}] = fp[{{final_xform.color_speed}}];
	/* opacity */     fp_inflated[inflated_offset + {{final_xform.opacity}}] = fp[{{final_xform.opacity}}];

	{% for param,offset in final_xform.param %}
	// {{param}}
	fp_inflated[inflated_offset + {{offset}}] = fp[{{offset}}];
	{% endfor %}

	{% for var,offset in final_xform.variations %}
	// {{var}}
	fp_inflated[inflated_offset + {{offset}}] = fp[{{offset}}];
	{% endfor %}

	float sino_f = sin(rads_per_second * dt * fp[{{final_xform.rotation_frequency}}]);
	float coso_f = cos(rads_per_second * dt * fp[{{final_xform.rotation_frequency}}]);
	/* affine a */ fp_inflated[inflated_offset + {{final_xform.affine.0}}] = fp[{{final_xform.affine.0}}] * coso_f + fp[{{final_xform.affine.2}}] * sino_f;
	/* affine b */ fp_inflated[inflated_offset + {{final_xform.affine.1}}] = fp[{{final_xform.affine.1}}] * coso_f + fp[{{final_xform.affine.3}}] * sino_f;
	/* affine c */ fp_inflated[inflated_offset + {{final_xform.affine.2}}] = fp[{{final_xform.affine.2}}] * coso_f - fp[{{final_xform.affine.0}}] * sino_f;
	/* affine d */ fp_inflated[inflated_offset + {{final_xform.affine.3}}] = fp[{{final_xform.affine.3}}] * coso_f - fp[{{final_xform.affine.1}}] * sino_f;
	/* affine e */ fp_inflated[inflated_offset + {{final_xform.affine.4}}] = fp[{{final_xform.affine.4}}];
	/* affine f */ fp_inflated[inflated_offset + {{final_xform.affine.5}}] = fp[{{final_xform.affine.5}}];

	{% if existsIn(final_xform, "post") %}
	fp_inflated[inflated_offset + {{final_xform.post.0}}] = fp[{{final_xform.post.0}}];
	fp_inflated[inflated_offset + {{final_xform.post.1}}] = fp[{{final_xform.post.1}}];
	fp_inflated[inflated_offset + {{final_xform.post.2}}] = fp[{{final_xform.post.2}}];
	fp_inflated[inflated_offset + {{final_xform.post.3}}] = fp[{{final_xform.post.3}}];
	fp_inflated[inflated_offset + {{final_xform.post.4}}] = fp[{{final_xform.post.4}}];
	fp_inflated[inflated_offset + {{final_xform.post.5}}] = fp[{{final_xform.post.5}}];
	{% endif %}
	{% endif %}
}
