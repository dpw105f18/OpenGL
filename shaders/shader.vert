#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition_modelSpace;
layout(location = 1) in vec2 textureCoordinate;

out vec2 fragmentTextureCoordinate;
out vec3 modelPos;

uniform mat4 vp;
uniform mat4 model;

void main(){
	fragmentTextureCoordinate = textureCoordinate;
    modelPos = vertexPosition_modelSpace;
    gl_Position = vp * model * vec4(vertexPosition_modelSpace, 1.0);
}
