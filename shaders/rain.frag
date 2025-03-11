#version 330 core

in vec3 fragColor;
out vec4 fColor;

void main() {
    fColor = vec4(fragColor, 0.8); 
}
