#include "config.h"
#include "bone_geometry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

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


vector<vector<float> > boneMatrix;
// FIXME: Implement bone animation.

std::unordered_map <int, vector<pair<int, float> > >jointTuples;


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


        //A SparseTuple has { int jid, vid; float weight; }
        //jid == joint_id, vid == vertex_id

        std::vector<SparseTuple> tup;
        mr.getJointWeights(tup);

        int max_vec = 0;
        for (int i = 0; i < tup.size(); i++) {
                max_vec = std::max(max_vec, tup.at(i).vid); 
    
        }
        ++max_vec;
        cout << "num connections: " << tup.size() << endl;
        cout << "max vec_id: " << max_vec << endl;      
     

        //for each joint
        //create a list (vector)
        //of vertex-weight pairs
        //which represent how much impact
        //the movements of this joint
        //have on those vertices
        for(int i = 0; i < tup.size(); ++i)
        {
                try {
                        vector<pair<int, float> > thisJoint = jointTuples.at(tup[i].jid);
                        thisJoint.push_back(make_pair(tup[i].vid, tup[i].weight));
                        jointTuples.at(tup[i].jid) = thisJoint;
                }
                catch(out_of_range &e)
                {
                        vector<pair<int, float> > thisJoint;
                        thisJoint.push_back(make_pair(tup[i].vid, tup[i].weight));
                        jointTuples.insert(make_pair(tup[i].jid, thisJoint));
                }
        }


        // for(int r = 0; r<skeleton.bones.size(); ++r) {
        //         vector<float> row(max_vec);
        //         boneMatrix.push_back(row);
        // }

        // int idCounter = 0;   
        // for (int i = 0; i < tup.size(); i++) {
            
        //         int jointNum = tup[i].jid;
        //         vector<Bone*> boneChildren = skeleton.retJointBones(jointNum);

        //         for (int j = 0; j < boneChildren.size(); j++) {

        //                 Bone* b = boneChildren.at(j);
                        
        //                 float weight = tup[i].weight;

        //                 boneMatrix.at(b->ID).at(tup[i].vid) = weight;

        //                 if(fabs(weight) > 0)
        //                 {
        //                         animIdxList.push_back(make_pair(b->ID, tup[i].vid));
        //                 }
        //         }
        // }
}

void Mesh::updateAnimation(int sourceJointIdx)
{
        //get the list of bones where this is the source joint
        vector<Bone *> bonesList = skeleton.retJointBones(sourceJointIdx);

        //get the list of vertex-weight pairs for this joint
        //holding info about the repective impacts the movements of this joint
        //have on the respective vertices
        vector<pair<int, float> > thisJoint = jointTuples.at(sourceJointIdx);

        for(pair<int, float> p: thisJoint)
        {
                vec4 newV(0.0f);
                int vid = p.first;

                float weight = p.second;
                vec4 v = vertices.at(vid);

                for(Bone* b: bonesList)
                {
                        newV += (weight * b->DinvU * v);
                }
                animated_vertices.at(vid) = newV;
        }
}


void Mesh::updateAnimation()
{
        for(int idx = 0; idx < animIdxList.size(); ++idx)
        {
                pair<int, int> IDXs = animIdxList.at(idx);
                animated_vertices.at(IDXs.second) = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        for(int idx = 0; idx < animIdxList.size(); ++idx)
        {
                pair<int, int> IDXs = animIdxList.at(idx);
         
                vec4 v = vertices.at(IDXs.second);
                Bone* b = skeleton.getBoneFromID(IDXs.first);

                float weight = boneMatrix.at(IDXs.first).at(IDXs.second);
                mat4 D = b->D;
                mat4 invU = inverse(b->U);

                animated_vertices.at(IDXs.second) += (weight * D * invU * v);
        }

        // for(int i = 0; i<animated_vertices.size(); ++i)
        // {
        //         cout << "vertex: " << animated_vertices.at(i) << endl; 
        // }
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
