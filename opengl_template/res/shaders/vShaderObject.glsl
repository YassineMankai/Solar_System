#version 330 core            // Minimal GL version support expected from the GPU

layout(location=0) in vec3 vPosition; // The 1st input attribute is the position (CPU side: glVertexAttrib 0)
layout(location=1) in vec3 vNormal; // The 2nd input attribute is the normal (CPU side: glVertexAttrib 1)
layout(location=2) in vec2 vTexCoord;

out vec3 fNormal;
out vec3 fColor;
out vec3 fPosition;
out vec2 fTexCoord;

uniform mat4 viewMat, projMat, modelMat;

void main() {
    gl_Position = projMat * viewMat * modelMat * vec4(vPosition, 1.0); // mandatory to rasterize properly
	fNormal = mat3(transpose(inverse(modelMat))) * vNormal;
    fPosition = vec3(modelMat * vec4(vPosition, 1.0));
    fTexCoord = vTexCoord;
}
