#version 450 core
#extension GL_ARB_separate_shader_objects : enable

in vec2 fragmentTextureCoordinate;

uniform sampler2D sampler;

out vec3 color;

void main(){
    color = texture(sampler, fragmentTextureCoordinate).rgb;
}
