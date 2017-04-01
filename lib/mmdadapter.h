/*
 * For students:
 * This header include essential interfaces for loading PMD files 
 * You may hack its implementation but it's not recommended (a waste of time).
 */
#ifndef MMD_ADAPTER_H
#define MMD_ADAPTER_H

#include "material.h"
#include <image.h>
#include <string>
#include <glm/glm.hpp>

class MMDAdapter;

struct SparseTuple {
	/*
	 * jid: abbreviation for Joint ID
	 * vid: abbreviation for Vertex ID
	 */
	int jid, vid;
	float weight;

	SparseTuple(int _jid, int _vid, float _weight)
		: jid(_jid), vid(_vid), weight(_weight)
	{
	}
};

class MMDReader {
public:
	MMDReader();
	~MMDReader();

	/*
	 * Open a PMD model file.
	 * Input
	 *      fn: file name
	 * Return:
	 *      true: file opened successfully
	 *      false: file failed to open
	 * Note: We don't test your robustness for invalid input.
	 */
	bool open(const std::string& fn);
	/*
	 * Get mesh data from an opened model file
	 * Output:
	 *      V: list of vertices
	 *      F: list of faces
	 *      N: list of vertex normals
	 *      UV: texture UV coordinates for each vertex.
	 */
	void getMesh(std::vector<glm::vec4>& V,
		     std::vector<glm::uvec3>& F,
		     std::vector<glm::vec4>& N,
		     std::vector<glm::vec2>& UV);
	/*
	 * Get list of materials
	 * Check Material struct (in material.h) for details
	 */
	void getMaterial(std::vector<Material>&);
	/*
	 * Get a joint for given ID
	 * Input:
	 *      id: the Joint ID
	 * Output:
	 *      offset: the relative offset from its parent joint.
	 *              If there is no parent, this is an absolute position.
	 *      parent: the parent joint. -1 means there is no parent.
	 * Return:
	 *      true: the Joint ID is valid
	 *      false: the Joint ID in invalid.
	 * Note: The range of joint IDs are always starting from zero and
	 *       ending with some positive number. You may assume the joints
	 *       is a tree whose root is Joint 0, which is guaranteed by this
	 *       adapter.
	 *       
	 *       The bone organization in actual PMD files is a forest.
	 */
	bool getJoint(int id, glm::vec3& offset, int& parent);
	/*
	 * Get a list of tuples representing the vertex-joint weight.
	 * See SparseTuple for more details
	 */
	void getJointWeights(std::vector<SparseTuple>& tup);
private:
	std::unique_ptr<MMDAdapter> d_;
};

#endif
