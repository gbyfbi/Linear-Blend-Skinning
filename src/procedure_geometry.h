#ifndef PROCEDURE_GEOMETRY_H
#define PROCEDURE_GEOMETRY_H

#include <vector>
#include <glm/glm.hpp>

struct LineMesh {
	LineMesh(){}
	~LineMesh(){}

	


};


void create_floor(std::vector<glm::vec4>& floor_vertices, 
	std::vector<glm::uvec3>& floor_faces);

// FIXME: Add functions to generate the bone mesh.
void create_skeleton(std::vector<glm::vec4>& skel_vertices, 
	std::vector<glm::uvec2>& skel_faces);

#endif
