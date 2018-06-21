#version 450 core
#extension GL_ARB_separate_shader_objects : enable

in vec2 fragmentTextureCoordinate;
in vec3 modelPos;

uniform sampler2D sampler;

out vec3 color;

vec3 black = vec3(0.0,0.0,0.0);
vec3 white = vec3(1.0,1.0,1.0);

void main(){
    vec3 pos =  modelPos * 1.0;
    float total = floor(pos.x) + floor(pos.y) + floor(pos.z);
    bool isEven = mod(total, 2.0) == 0.0;
    color = isEven ? black : white;
}
