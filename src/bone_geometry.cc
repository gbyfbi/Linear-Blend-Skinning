#include "config.h"
#include "bone_geometry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
        size_t count = std::min(v.size(), static_cast<size_t>(10));
        for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
        os << "size = " << v.size() << "\n";
        return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds)
{
        os << "min = " << bounds.min << " max = " << bounds.max;
        return os;
}



// FIXME: Implement bone animation.


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

//function to load pmd into a mesh object
void Mesh::loadpmd(const std::string& fn)
{
        //load the image of the character from the pmd
        MMDReader mr;
        mr.open(fn);
        mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
        computeBounds();
        mr.getMaterial(materials);


        //read the joints in
        int i = 0;
        int parent = 0;
        vec3 offs;
        while (mr.getJoint(i, offs, parent)) {
                
                
                Joint* j = new Joint();
                j->ID = i;
                j->pID = parent;

                if(j->ID == 0){
                        assert(parent == -1);
//                        std::cout << "\n\nfound root\n\n";
                }
                	

                j->offset = offs;
                this->skeleton.joints.insert(std::make_pair(i, j));
                i++;
        }

        this->skeleton.initSkeleton();

        // FIXME: load skeleton and blend weights from PMD file
        //        also initialize the skeleton as needed
}



void Mesh::updateAnimation()
{
        animated_vertices = vertices;
        // FIXME: blend the vertices to animated_vertices, rather than copy
        //        the data directly.
}


void Mesh::computeBounds()
{
        bounds.min = glm::vec3(std::numeric_limits<float>::max());
        bounds.max = glm::vec3(-std::numeric_limits<float>::max());
        for (const auto& vert : vertices) {
                bounds.min = glm::min(glm::vec3(vert), bounds.min);
                bounds.max = glm::max(glm::vec3(vert), bounds.max);
        }
}