#version 450
#extension GL_ARB_separate_shader_objects : enable

/* copy and paste from tutorial */

layout(location = 0) in vec3 VertOutFragColour;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(VertOutFragColour, 1.0);
}
