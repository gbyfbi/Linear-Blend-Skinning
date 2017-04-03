#include "gui.h"
#include "config.h"
#include <jpegio.h>
#include "bone_geometry.h"
#include <iostream>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace {
	// Intersect a cylinder with radius 1/2, height 1, with base centered at
	// (0, 0, 0) and up direction (0, 1, 0).
	bool IntersectCylinder(const glm::vec3& origin, const glm::vec3& direction,
			float radius, float height, float* t)
	{
		//FIXME perform proper ray-cylinder collision detection
		float minDistance;
		//closest point to infinite line can be given by a derivative of the equation
		//for the distance to the bone based on the line's t value
		return true;
	}
}

using namespace glm;


bool isIntersected;
Ray r;
vector<uvec2>lines;
vector<vec4>verts;
bool canSwitchBones = true;


void GUI::setLinesNVerts(vector<uvec2> l, vector<vec4> v) {
	lines = l;
	verts = v;
}

bool GUI::checkCylinderIntersection(vec3 origin, vec3 d, float max, float& st) {
	//algorithm based on information from http://geomalgorithms.com/a07-_distance.html
	
	float kCylinderRadius = .3f;
	vec3 w = r.pos - origin;
	float divide = dot(r.d, r.d) * dot(d, d) - dot(r.d, d) * dot(r.d, d);
	float rt = (dot(r.d, d) * dot(d, w) - dot(d, d) * dot(r.d, w))/divide;
	st = (dot(r.d, r.d) * dot(d, w) - dot(r.d, d) * dot(r.d, w))/divide;
	if (st > 0 && st < max) {
		double dist = distance(origin + d * st, r.pos + r.d * rt);
		if (dist <= kCylinderRadius) {
			return true;
		}
	}
	return false;
}


bool GUI::checkBoneHover()
{
	float minST = 10000.0f;
	int idx = 0;
	isIntersected = false;
	for(uvec2 pair: lines)
	{
		vec4 s = verts.at(pair[0]);
		vec4 d = verts.at(pair[1]);

		vec3 source(s[0], s[1], s[2]);
		vec3 dest(d[0], d[1], d[2]);
		vec3 dir = normalize(dest - source);
		float length = glm::length(dest-source);

		float st = -1.0f;
		if(checkCylinderIntersection(source, dir, length, st))
		{
			isIntersected = true;

			if(st < minST)//find one closest to eye's z direction
			{
				setCurrentBone(idx);
				minST = st;
			}
		}
		++idx;
	}
	if(isIntersected == false)
		setCurrentBone(-1);

	return isIntersected;
}

GUI::GUI(GLFWwindow* window)
	:window_(window)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	float aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI()
{
}

void GUI::ScreenToWorld(double x, double y) {
	GLint viewport[4];
	GLfloat winX, winY, winZ;
	
	glGetIntegerv( GL_VIEWPORT, viewport);
	
	winX = (float) x;
	winY = (float) viewport[3] - (float) y;
	winZ = -1.0f;
	glReadPixels(x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	vec3 win(winX, winY, winZ);
	vec3 worldPos = unProject(win, view_matrix_ * model_matrix_, projection_matrix_, vec4(viewport[0], viewport[1], viewport[2], viewport[3]));

	r.d = normalize(vec3(worldPos[0] - eye_[0], worldPos[1] - eye_[1], worldPos[2] - eye_[2]));
	r.pos = eye_;
	
}


void GUI::assignMesh(Mesh* mesh)
{
	mesh_ = mesh;
	center_ = mesh_->getCenter();
}

void GUI::setBoneMovementMode()
{
	boneMovement = !boneMovement;
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
		//FIXME save out a screenshot using SaveJPEG
	}

	if (key == GLFW_KEY_M && action == GLFW_RELEASE) {
		setBoneMovementMode();
	}

	if (captureWASDUPDOWN(key, action))
		return ;
	if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
		float roll_speed;
		if (key == GLFW_KEY_RIGHT)
			roll_speed = -roll_speed_;
		else
			roll_speed = roll_speed_;
		// FIXME: actually roll the bone here
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode_ = !fps_mode_;
	} else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
		current_bone_--;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
		current_bone_++;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
		transparent_ = !transparent_;
	}
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, window_width_, window_height_);

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;
	bool drag_bone = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;
	
	// cout << "Ray pos == " << r.pos << ", d == " << r.d << endl;
	
	if (drag_camera) {

		glm::vec3 _axis = glm::normalize(
				orientation_ *
				glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
				);
		orientation_ =
			glm::mat3(glm::rotate(rotation_speed_, _axis) * glm::mat4(orientation_));
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
	} 

	else if (drag_bone && current_bone_ != -1) {
		// FIXME: Handle bone rotation

		// cout << "in mouse pos \n\n";

		vec3 drag_axis = unProject(mouse_direction, view_matrix_ * model_matrix_, projection_matrix_, vec4(viewport[0], viewport[1], viewport[2], viewport[3]));
		// vec4 mouse_direction4 = vec4(mouse_direction, 1.0f);
		// vec4 drag_axis4 = glm::inverse(view_matrix_) * mouse_direction4;
		// vec3 drag_axis(drag_axis4[0], drag_axis4[1], drag_axis4[2]);

		// vec4 ax(drag_axis, 1.0f);
		// ax = R * ax;
		// axis = vec3(ax[0], ax[1], ax[2]);

		// axis = cross(axis, look_);
		// angle = rotation_speed_;
		// R = glm::rotate(angle, axis) * R;

		axis = cross(drag_axis, look_);
		angle = rotation_speed_;
		R = glm::rotate(angle, axis);

		dragging = true;
		return ;
	}

	if(!drag_bone || current_bone_ == -1)
	{
		dragging = false;
		axis = vec3(0.0f, 0.0f, 0.0f);
		angle = 0.0f;

	}

	// FIXME: highlight bones that have been moused over
	// current_bone_ = -1;
}
void GUI::mouseButtonCallback(int button, int action, int mods)
{
	// cout << "got here\n";
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		canSwitchBones = false;
		// cout << "lock down bone\n\n";
	}

	else if(current_button_ == GLFW_MOUSE_BUTTON_LEFT && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		dragging = false;
		axis = vec3(0.0f, 0.0f, 0.0f);
		angle = 0.0f;
		canSwitchBones = true;
		// cout << "unlock bone\n\n";
	}

	current_button_ = button;
	drag_state_ = (action == GLFW_PRESS);
}

void GUI::updateMatrices()
{
	// Compute our view, and projection matrices.
	if (fps_mode_)
		center_ = eye_ + camera_distance_ * look_;
	else
		eye_ = center_ - camera_distance_ * look_;

	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = glm::vec4(eye_, 1.0f);

	aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_[0][0];
	ret.model= &model_matrix_[0][0];
	ret.view = &view_matrix_[0][0];
	return ret;
}

void GUI::setCurrentBone(int i)
{
	current_bone_ = i;
}

bool GUI::captureWASDUPDOWN(int key, int action)
{
	if (key == GLFW_KEY_W) {
		if (fps_mode_)
			eye_ += zoom_speed_ * look_;
		else
			camera_distance_ -= zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_)
			eye_ -= zoom_speed_ * look_;
		else
			camera_distance_ += zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_)
			eye_ -= pan_speed_ * tangent_;
		else
			center_ -= pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_)
			eye_ += pan_speed_ * tangent_;
		else
			center_ += pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			eye_ -= pan_speed_ * up_;
		else
			center_ -= pan_speed_ * up_;
		return true;
	} else if (key == GLFW_KEY_UP) {
		if (fps_mode_)
			eye_ += pan_speed_ * up_;
		else
			center_ += pan_speed_ * up_;
	return true;
	}
	return false;
}

bool GUI::IsIntersected() {
	return isIntersected;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);

	gui->mousePosCallback(mouse_x, mouse_y);

	gui->ScreenToWorld(mouse_x, mouse_y);

	if(canSwitchBones)
		isIntersected = gui->checkBoneHover();
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}
