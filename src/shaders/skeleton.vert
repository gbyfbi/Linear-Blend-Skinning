R"zzz(
#version 330 core
in vec4 vertex_position;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * vertex_position;
}
)zzz"
