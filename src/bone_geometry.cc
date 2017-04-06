#include "config.h"
#include "bone_geometry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

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
        //std::map<int, vector<pair<int,int>>> boneWeights;

        int max_vec = 0;
        for (int i = 0; i < tup.size(); i++) {
                max_vec = std::max(max_vec, tup.at(i).vid); 
    
        }
        ++max_vec;
        cout << "num connections: " << tup.size() << endl;
        cout << "max vec_id: " << max_vec << endl;      

//        boneMatrix = new int[skeleton.bones.size()][max_vec];//each row corresponds to one bone (one source joint),
                                                        //and within each row, the elements correspond to different mesh vertices
                                                        //on which the bone has influence
     

        for(int r = 0; r<skeleton.bones.size(); ++r) {
                vector<float> row(max_vec);
                boneMatrix.push_back(row);
        }


        int idCounter = 0;   
        for (int i = 0; i < tup.size(); i++) {
            
                //get the list of bones that starts at source joint with ID jid
                int jointNum = tup[i].jid;
                vector<Bone*> boneChildren = skeleton.retJointBones(jointNum);

                for (int j = 0; j < boneChildren.size(); j++) {

                        Bone* b = boneChildren.at(j);
                        // if (b->ID != -1) {

                                float weight = tup[i].weight;

                                boneMatrix.at(b->ID).at(tup[i].vid) = weight;

                        // } else {
                        //         b->ID = idCounter;
                                
                        //         cout << "idCounter is: " << idCounter << endl;

                        //         idCounter++;

                        //         skeleton.BoneIDMap.insert(std::make_pair(b->ID, b));

                        //         boneMatrix[b->ID][tup[i].vid] = tup[i].weight;
                        // }
                }
        }




        // for(int i = 0; i < skeleton.bones.size(); ++i)
        // {
        //         Bone* b = skeleton.bones.at(i);
        //         cout << "bone ID is: " << b->ID << endl;



        //         // try{
        //         //         cout << "bone number: " << i <<  " is " << skeleton.BoneIDMap.at(i) << endl;                       
        //         // }
        //         // catch( out_of_range &e)
        //         // {
        //         //         cout << "bad bone ID";
        //         // }
        // }


}



void Mesh::updateAnimation()
{
        animated_vertices = vertices;
        // FIXME: blend the vertices to animated_vertices, rather than copy
        //        the data directly.

        for(int i = 0; i < vertices.size(); ++i)
        {
                vec4 v = vertices.at(i);
                vec4 newV(0.0);
                for(int j = 0; j < skeleton.bones.size(); ++j) {
                        Bone* b = skeleton.getBoneFromID(j);
                        
                        if(b == nullptr)
                        {
                                // cout << "bad bone ID\n\n";
                                continue;
                        }        

                        float weight = boneMatrix[j][i]; 
                        mat4 D = b->D;

                        // cout << "b->U: " << b->U << endl;

                        mat4 invU = inverse(b->U);
                        newV += weight * D * invU * v;
                }
                animated_vertices[i] = newV;
        }


        for(int i = 0; i<animated_vertices.size(); ++i)
        {
                cout << "vertex: " << animated_vertices.at(i) << endl; 
        }
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
