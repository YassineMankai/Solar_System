#version 330 core	     // Minimal GL version support expected from the GPU

in vec2 fTexCoord;

out vec4 color;	  // Shader output: the color response attached to this fragment

uniform sampler2D text;

void main() {
	vec3 texColor = texture(text, fTexCoord).rgb;

	color = vec4(texColor, 1.0); 
}
