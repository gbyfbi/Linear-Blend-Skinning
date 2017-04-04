#include <GL/glew.h>
#include <dirent.h>

#include "bone_geometry.h"
#include "procedure_geometry.h"
#include "render_pass.h"
#include "config.h"
#include "gui.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>

using namespace glm;
using namespace std;

int window_width = 800, window_height = 600;
const std::string window_title = "Skinning";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* skel_vertex_shader = 
#include "shaders/skeleton.vert"
;

const char* skel_fragment_shader = 
#include "shaders/skeleton.frag"
;

const char* color_fragment_shader = 
#include "shaders/skel_color.frag"
;


double prev_mouse_x = 0, prev_mouse_y = 0;
vec4 mouse_direction;
int g_current_button;
bool g_mouse_pressed;

vector<uvec2> cylLines;
vector<vec4> cylVerts;
bool firstTime = false;



// FIXME: Add more shaders here.

void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y) {
	if (!g_mouse_pressed) return;
	double difX = mouse_x - prev_mouse_x;
	double difY = mouse_y - prev_mouse_y;
	prev_mouse_x = mouse_x;
	prev_mouse_y = mouse_y;
	
	mouse_direction[0] = difX;
	mouse_direction[1] = difY;
	
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

void clearColorPass(RenderDataInput &color_pass_input, RenderPass &color_pass, 
	ShaderUniform std_view, ShaderUniform std_proj)
{
	cylVerts.clear();
	cylLines.clear();

	cylVerts.push_back(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	cylVerts.push_back(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	cylLines.push_back(uvec2(0, 1));

	color_pass.updateVBO(0, cylVerts.data(), cylVerts.size());
	color_pass_input.assign_index(cylLines.data(), cylLines.size(), 2);
	color_pass = RenderPass(color_pass.getVAO(),
	color_pass_input,
	{ skel_vertex_shader, nullptr, color_fragment_shader},
	{ std_view, std_proj},
	{ "fragment_color" }
	);
}

void InitColorPass(int idx, Mesh mesh, RenderDataInput &color_pass_input, RenderPass &color_pass, 
	bool &firstTime, ShaderUniform std_view, ShaderUniform std_proj)
{
	uvec2 coloredLine = mesh.skeleton.skel_lines[idx];
	vector<vec4> coloredVerts;
	coloredVerts.push_back(mesh.skeleton.skel_vertices[coloredLine[0]]);
	coloredVerts.push_back(mesh.skeleton.skel_vertices[coloredLine[1]]);
	vec4 t4 = normalize(coloredVerts[1]-coloredVerts[0]);
	vec3 t(t4[0], t4[1], t4[2]);
	vec3 n, b;
	mesh.skeleton.calculateAxes(t, n, b);
	
	vec3 nH = 0.5f * n;
	vec3 bH = 0.5f * b;

	float len = glm::length(coloredVerts[1] - coloredVerts[0]);
	float radius = fmin(0.2f, 0.5*len);


	//handle initialization/updating of vertices
	cylVerts.clear();

	cylVerts.push_back(glm::translate(radius * n) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * n) * coloredVerts[1]);

	cylVerts.push_back(glm::translate(-1.0f * radius * n) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * n) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(radius * b) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * b) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(-1.0f * radius * b) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * b) * coloredVerts[1]);


	cylVerts.push_back(glm::translate(radius * (nH + bH)) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (nH + bH)) * coloredVerts[1]);

	cylVerts.push_back(glm::translate(-1.0f * radius * (nH + bH)) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH + bH)) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(radius * (nH - bH)) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (nH - bH)) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH - bH)) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH - bH)) * coloredVerts[1]);


	cylVerts.push_back(glm::translate(radius * (nH + (2.0f * bH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (nH + (2.0f * bH))) * coloredVerts[1]);

	cylVerts.push_back(glm::translate(-1.0f * radius * (nH + (2.0f * bH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH + (2.0f * bH))) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(radius * (nH - (2.0f * bH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (nH - (2.0f * bH))) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH - (2.0f * bH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (nH - (2.0f * bH))) * coloredVerts[1]);


	cylVerts.push_back(glm::translate(radius * (bH + (2.0f * nH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (bH + (2.0f * nH))) * coloredVerts[1]);

	cylVerts.push_back(glm::translate(-1.0f * radius * (bH + (2.0f * nH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (bH + (2.0f * nH))) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(radius * (bH - (2.0f * nH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(radius * (bH - (2.0f * nH))) * coloredVerts[1]);
	
	cylVerts.push_back(glm::translate(-1.0f * radius * (bH - (2.0f * nH))) * coloredVerts[0]);
	cylVerts.push_back(glm::translate(-1.0f * radius * (bH - (2.0f * nH))) * coloredVerts[1]);
	
	// for(int x = 0; x<cylVerts.size(); ++x)
	// {
	// 	cout << "cylVert number " << x << " : " << cylVerts[x] << "\n";
	// }
	// cout << "\n";

	if(firstTime)
	{
		color_pass_input.assign(0, "vertex_position", cylVerts.data(), 32, 4, GL_FLOAT);	
	}
	else
	{
		color_pass.updateVBO(0, cylVerts.data(), cylVerts.size());
	}

	//handle initialization/updating of lines/indices
	cylLines.clear();
	cylLines.push_back(uvec2(0,1));
	cylLines.push_back(uvec2(2,3));
	cylLines.push_back(uvec2(4,5));
	cylLines.push_back(uvec2(6,7));
	cylLines.push_back(uvec2(8,9));
	cylLines.push_back(uvec2(10,11));
	cylLines.push_back(uvec2(12,13));
	cylLines.push_back(uvec2(14,15));
	cylLines.push_back(uvec2(16,17));
	cylLines.push_back(uvec2(18,19));
	cylLines.push_back(uvec2(20,21));
	cylLines.push_back(uvec2(22,23));
	cylLines.push_back(uvec2(24,25));
	cylLines.push_back(uvec2(26,27));
	cylLines.push_back(uvec2(28,29));
	cylLines.push_back(uvec2(30,31));

	color_pass_input.assign_index(cylLines.data(), cylLines.size(), 2);

	if(firstTime)
	{
		color_pass = RenderPass(-1,
		color_pass_input,
		{ skel_vertex_shader, nullptr, color_fragment_shader},
		{ std_view, std_proj},
		{ "fragment_color" }
		);
		firstTime = false;
	}
	else
	{
		color_pass = RenderPass(color_pass.getVAO(),
		color_pass_input,
		{ skel_vertex_shader, nullptr, color_fragment_shader},
		{ std_view, std_proj},
		{ "fragment_color" }
		);
	}



}


void InitSkelGL(Mesh &mesh, RenderDataInput &skeleton_pass_input, RenderPass &skeleton_pass, bool &firstTime, 
	ShaderUniform std_view, ShaderUniform std_proj, bool rotation, mat4 R, int lineID)
{
	//if a joint is rotated,
	//update it and its children's offsets
	//and recalculate the world coords
	//of the vertices
	if(rotation)
	{
		uvec2 line = mesh.skeleton.skel_lines[lineID];
		mesh.skeleton.updateOffsets(line[1], R);	
		mesh.skeleton.initVertsNLinesIter();
	}	

	if(firstTime)
	{
		skeleton_pass_input.assign(0, "vertex_position", mesh.skeleton.skel_vertices.data(), mesh.skeleton.skel_vertices.size(), 4, GL_FLOAT);
		
	}
	else
	{
		skeleton_pass.updateVBO(0, mesh.skeleton.skel_vertices.data(), mesh.skeleton.skel_vertices.size());		
	}
	
	skeleton_pass_input.assign_index(mesh.skeleton.skel_lines.data(), mesh.skeleton.skel_lines.size(), 2);
	


	if(firstTime)
	{
		skeleton_pass = RenderPass(-1,
		skeleton_pass_input,
		{ skel_vertex_shader, nullptr, skel_fragment_shader},
		{ std_view, std_proj},
		{ "fragment_color" }
		);
		firstTime = false;
	}
	else
	{
		skeleton_pass = RenderPass(skeleton_pass.getVAO(),
		skeleton_pass_input,
		{ skel_vertex_shader, nullptr, skel_fragment_shader},
		{ std_view, std_proj},
		{ "fragment_color" }
		);
	}
}



int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Input model file is missing" << std::endl;
		std::cerr << "Usage: " << argv[0] << " <PMD file>" << std::endl;
		return -1;
	}
	GLFWwindow *window = init_glefw();
	GUI gui(window);

	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	create_floor(floor_vertices, floor_faces);

	// FIXME: add code to create bone and cylinder geometry

	Mesh mesh;
	mesh.loadpmd(argv[1]);
	mesh.skeleton.initVertsNLinesIter();

	for (int i = 0; i < mesh.skeleton.bones.size(); i++) {
		Bone* b = mesh.skeleton.bones.at(i);
	}
	
	std::cout << "Loaded object  with  " << mesh.vertices.size()
		<< " vertices and " << mesh.faces.size() << " faces.\n";

	glm::vec4 mesh_center = glm::vec4(0.0f);
	for (size_t i = 0; i < mesh.vertices.size(); ++i) {
		mesh_center += mesh.vertices[i];
	}
	mesh_center /= mesh.vertices.size();

	/*
	 * GUI object needs the mesh object for bone manipulation.
	 */
	gui.assignMesh(&mesh);

	glm::vec4 light_position = glm::vec4(0.0f, 100.0f, 0.0f, 1.0f);
	MatrixPointers mats; // Define MatrixPointers here for lambda to capture
	/*
	 * In the following we are going to define several lambda functions to bind Uniforms.
	 * 
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 */
	auto matrix_binder = [](int loc, const void* data) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat*)data);
	};
	auto vector_binder = [](int loc, const void* data) {
		glUniform4fv(loc, 1, (const GLfloat*)data);
	};
	auto vector3_binder = [](int loc, const void* data) {
		glUniform3fv(loc, 1, (const GLfloat*)data);
	};
	auto float_binder = [](int loc, const void* data) {
		glUniform1fv(loc, 1, (const GLfloat*)data);
	};
	/*
	 * These lambda functions below are used to retrieve data
	 */
	auto std_model_data = [&mats]() -> const void* {
		return mats.model;
	}; // This returns point to model matrix
	glm::mat4 floor_model_matrix = glm::mat4(1.0f);
	auto floor_model_data = [&floor_model_matrix]() -> const void* {
		return &floor_model_matrix[0][0];
	}; // This return model matrix for the floor.
	auto std_view_data = [&mats]() -> const void* {
		return mats.view;
	};
	auto std_camera_data  = [&gui]() -> const void* {
		return &gui.getCamera()[0];
	};
	auto std_proj_data = [&mats]() -> const void* {
		return mats.projection;
	};
	auto std_light_data = [&light_position]() -> const void* {
		return &light_position[0];
	};
	auto alpha_data  = [&gui]() -> const void* {
		static const float transparet = 0.5; // Alpha constant goes here
		static const float non_transparet = 1.0;
		if (gui.isTransparent())
			return &transparet;
		else
			return &non_transparet;
	};

	// FIXME: add more lambdas for data_source if you want to use RenderPass.
	//        Otherwise, do whatever you like here

	ShaderUniform std_model = { "model", matrix_binder, std_model_data };
	ShaderUniform floor_model = { "model", matrix_binder, floor_model_data };
	ShaderUniform std_view = { "view", matrix_binder, std_view_data };
	ShaderUniform std_camera = { "camera_position", vector3_binder, std_camera_data };
	ShaderUniform std_proj = { "projection", matrix_binder, std_proj_data };
	ShaderUniform std_light = { "light_position", vector_binder, std_light_data };
	ShaderUniform object_alpha = { "alpha", float_binder, alpha_data };

	// FIXME: define more ShaderUniforms for RenderPass if you want to use it.
	//        Otherwise, do whatever you like here

	std::vector<glm::vec2>& uv_coordinates = mesh.uv_coordinates;
	RenderDataInput object_pass_input;
	object_pass_input.assign(0, "vertex_position", nullptr, mesh.vertices.size(), 4, GL_FLOAT);
	object_pass_input.assign(1, "normal", mesh.vertex_normals.data(), mesh.vertex_normals.size(), 4, GL_FLOAT);
	object_pass_input.assign(2, "uv", uv_coordinates.data(), uv_coordinates.size(), 2, GL_FLOAT);
	object_pass_input.assign_index(mesh.faces.data(), mesh.faces.size(), 3);
	object_pass_input.useMaterials(mesh.materials);
	RenderPass object_pass(-1,
			object_pass_input,
			{
			  vertex_shader,
			  geometry_shader,
			  fragment_shader
			},
			{ std_model, std_view, std_proj,
			  std_light,
			  std_camera, object_alpha },
			{ "fragment_color" }
			);

	// FIXME: Create the RenderPass objects for bones here.
	//        Otherwise do whatever you like.

	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assign_index(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);

	RenderDataInput skeleton_pass_input;
	RenderPass skeleton_pass;

	bool skelFirstTime = true;
	mat4 R;
	InitSkelGL(mesh, skeleton_pass_input, skeleton_pass, skelFirstTime, std_view, std_proj, false, R, -1);



	bool firstTime = true;
	RenderDataInput color_pass_input;
	RenderPass color_pass;

	float aspect = 0.0f;
	std::cout << "center = " << mesh.getCenter() << "\n";

	bool draw_floor = true;
	bool draw_skeleton = true;
	bool draw_object = true;
	bool draw_cylinder = true;
	bool pr = true;
	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();

		int current_bone = gui.getCurrentBone();
#if 1
		draw_cylinder = (current_bone != -1 && gui.isTransparent());
#else
		draw_cylinder = true;
#endif
		// FIXME: Draw bones first.
		// Then draw floor.
		if(draw_skeleton) {

			bool bM = gui.canBonesMove();
			bool roll = gui.shouldRoll && gui.getCurrentBone()!=-1;
			bool drag = gui.dragging;

			if(bM)
			{

				if(roll)
				{
					float angle = gui.rollAngle;
					vec3 axis = mesh.skeleton.getTFromLineID(gui.getCurrentBone());

					mat4 R = glm::rotate(angle, axis);
					// mat4 R = gui.R;

					InitSkelGL(mesh, skeleton_pass_input, skeleton_pass, skelFirstTime, 
					std_view, std_proj, true, R, gui.getCurrentBone());

					skeleton_pass.setup();
					CHECK_GL_ERROR(glDrawElements(GL_LINES, mesh.skeleton.skel_lines.size() * 2, GL_UNSIGNED_INT, 0));

					InitColorPass(gui.getCurrentBone(), mesh, color_pass_input, color_pass, firstTime, std_view, std_proj);
					color_pass.setup();
					CHECK_GL_ERROR(glDrawElements(GL_LINES, 32, GL_UNSIGNED_INT, 0));

				}
				else if(drag) {
					float angle = gui.angle;
					vec3 axis = gui.axis;

					// cout << "angle: " << angle << endl;
					// cout << "axis: " << axis << endl;

					mat4 R = glm::rotate(angle, axis);
					// mat4 R = gui.R;

					InitSkelGL(mesh, skeleton_pass_input, skeleton_pass, skelFirstTime, 
					std_view, std_proj, true, R, gui.getCurrentBone());

					skeleton_pass.setup();
					CHECK_GL_ERROR(glDrawElements(GL_LINES, mesh.skeleton.skel_lines.size() * 2, GL_UNSIGNED_INT, 0));

					InitColorPass(gui.getCurrentBone(), mesh, color_pass_input, color_pass, firstTime, std_view, std_proj);
					color_pass.setup();
					CHECK_GL_ERROR(glDrawElements(GL_LINES, 32, GL_UNSIGNED_INT, 0));
				}
			}
			if((!bM)||(bM && !drag && !roll))
			{
				skeleton_pass.setup();
				CHECK_GL_ERROR(glDrawElements(GL_LINES, mesh.skeleton.skel_lines.size() * 2, GL_UNSIGNED_INT, 0));

				gui.setLinesNVerts(mesh.skeleton.skel_lines, mesh.skeleton.skel_vertices);
			
				if(gui.IsIntersected()) {
					InitColorPass(gui.getCurrentBone(), mesh, color_pass_input, color_pass, firstTime, std_view, std_proj);
					color_pass.setup();
					CHECK_GL_ERROR(glDrawElements(GL_LINES, 32, GL_UNSIGNED_INT, 0));
				}
				else
				{
					if(!firstTime)
					{
						clearColorPass(color_pass_input, color_pass, std_view, std_proj);
						color_pass.setup();
						CHECK_GL_ERROR(glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0));					
					}
				}			
			}
		}
		if (draw_floor) {
			floor_pass.setup();
			// Draw our triangles.
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
		}
		if (draw_object) {
			if (gui.isPoseDirty()) {
				mesh.updateAnimation();
				object_pass.updateVBO(0,
						      mesh.animated_vertices.data(),
						      mesh.animated_vertices.size());
#if 0
				// For debugging if you need it.
				for (int i = 0; i < 4; i++) {
					std::cerr << " Vertex " << i << " from " << mesh.vertices[i] << " to " << mesh.animated_vertices[i] << std::endl;
				}
#endif
				gui.clearPose();
			}
			object_pass.setup();
			int mid = 0;
			while (object_pass.renderWithMaterial(mid))
				mid++;
#if 0	
			// For debugging also
			if (mid == 0) // Fallback
				CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, mesh.faces.size() * 3, GL_UNSIGNED_INT, 0));
#endif
		}
		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
#if 0
	for (size_t i = 0; i < images.size(); ++i)
		delete [] images[i].bytes;
#endif
	exit(EXIT_SUCCESS);
}
