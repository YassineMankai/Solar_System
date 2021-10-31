#version 330 core	     // Minimal GL version support expected from the GPU

in vec3 fNormal;
in vec3 fPosition;
in vec2 fTexCoord;

out vec4 color;	  // Shader output: the color response attached to this fragment

uniform vec3 lColor;
uniform vec3 camPos;
uniform sampler2D text;

void main() {
	vec3 texColor = texture(text, fTexCoord).rgb;

	vec3 n = normalize(fNormal);
	
	vec3 l = normalize(vec3(0.0, 0.0, 0.0) - fPosition); 

	vec3 v = normalize(camPos - fPosition);

	vec3 r = normalize(2*dot(n,l)*n - l);

	float ambientCoef = 0.2;

	float diffuseCoef = max(dot(n,l),0);

	float specularCoef = 0.7*pow(max(dot(v,r),0),32);
	
	vec3 res = (ambientCoef+diffuseCoef+specularCoef)*lColor*texColor;
	color = vec4(res, 1.0); 
}
