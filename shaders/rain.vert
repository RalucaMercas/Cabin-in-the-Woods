#version 330 core

layout(location = 0) in vec3 position; 
layout(location = 1) in vec3 velocity; 

uniform mat4 view;
uniform mat4 projection;
uniform float deltaTime; 

out vec3 fragColor;

void main() {
    vec3 newPos = position + velocity * deltaTime; 
    gl_Position = projection * view * vec4(newPos, 1.0); 
    fragColor = vec3(0.5, 0.5, 1.0);
    gl_PointSize = 4.0; 
}
