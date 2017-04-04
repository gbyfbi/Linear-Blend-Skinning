#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <iostream>
#include <ostream>
#include <vector>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <mmdadapter.h>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <unordered_map>

using namespace glm;
using namespace std;

struct BoundingBox {
	BoundingBox()
		: min(glm::vec3(-std::numeric_limits<float>::max())),
		max(glm::vec3(std::numeric_limits<float>::max())) {}
	glm::vec3 min;
	glm::vec3 max;
};

struct Joint {
	// FIXME: Implement your Joint data structure.
	// Note: PMD represents weights on joints, but you need weights on
	//       bones to calculate the actual animation.
	
	int ID;
	int pID;
	vec3 offset;
	Joint() {};
};

struct Bone {
	Bone() {};
	~Bone() {};

	int ID;
	Joint* source;
	Joint* destination;
	float length;
	vec3 origin;
	vec3 t;
	vec3 n;
	vec3 b;
	mat4 T;
	mat4 R;
	mat4 S;


	Bone(Joint* js, Joint* jd, float l, vec3 o, vec3 t0, mat4 T0, mat4 R0) {
		source = js;
		destination = jd;
		length = l;
		origin = o;//same as offset of parent
		t = t0;
		ID = -1;
	//calculate n and b axes

		vec3 v(0.0, 0.0, 0.0);


		int minAxis = 0;//find axis with smallest value (in absolute value)
		float minVal = fabs(t[0]);
		for(int axis = 1; axis <=2; ++axis) {
			if(fabs(t[axis]) < minVal){
				minAxis = axis;
				minVal = fabs(t[axis]);
			}
		}


		v[minAxis] = 1.0f;
		n = normalize(cross(t,v));
		b = cross(t,n);

		T = T0;
		R = R0; 
		S = R0;
	}	
};

struct Skeleton {
	Skeleton() {};

	std::vector<Bone*> bones;

	//maps IDs to Joints
	std::unordered_map<int, Joint*> joints;

	//maps IDs to children IDs
	std::unordered_map<int, vector<int>> childJoints;

	
	//maps a joint's ID to the list of bones where that joint
	//is the source (can be more than one bone with the same source)
	std::unordered_map<int, vector<Bone*> > jointIDBoneMap;

	std::vector<glm::vec4> skel_vertices;
	std::vector<glm::uvec2> skel_lines;
	vector<float> change_color;

	//should be exclusive of limit:
	//think of limit as the number of bones
	//and the ID starts at 0 and goes to limit-1
	//just like in an array
	bool legalID(int ID, int limit)
	{
	    return (ID>=0 && ID<limit);
	}

	Joint* getJoint(int ID)
	{
		return joints.at(ID);
	}

	void calculateAxes(vec3 t, vec3 &n, vec3 &b)
	{
		vec3 v(0.0, 0.0, 0.0);

		//find axis with smallest value (in absolute value)
		int minAxis = 0;
		float minVal = fabs(t[0]);
		for(int axis = 1; axis <=2; ++axis) {
			if(fabs(t[axis]) < minVal){
				minAxis = axis;
				minVal = fabs(t[axis]);
			}
		}

		v[minAxis] = 1.0f;

		n = normalize(cross(t,v));

		b = normalize(cross(t,n));
	}

	vector<int> getChildJoints(Joint *j)
	{
		try{
			return childJoints.at(j->ID);	
		}
		catch(out_of_range & e){
			vector<int> badOutput;
			badOutput.push_back(-1);
			return badOutput;
		}
	}

	void setUpChildJoints()
	{
		for(int id = 0; id<joints.size(); ++id)
		{
			Joint *j = joints.at(id);
			if(legalID(j->pID, joints.size()))
			{
				Joint *p = joints.at(j->pID);
				try{
					vector<int> children;
					vector<Bone *> childBones = jointIDBoneMap.at(j->pID);
					for(Bone *b: childBones)
					{
						children.push_back(b->destination->ID);
					}
					childJoints.insert(make_pair(p->ID, children));
				}
				catch(out_of_range &e){}
			}
		}
	}

	void calculateBigT(mat4 &T, vec3 offset)
	{
        T = translate(offset);
	}

	void calculateR(mat4 &R, Joint* p, int i, vec3 t, float length)
	{
	    //if parent has parent (i.e. current joints grandparent)
	    //t of parent goes from grandparent offset to parent offset
	    //if no grandparent exists, then just goes from (0,0,0) to parent offset
	 
	 	vec3 grandParentOffset;   
	    if(p->ID == 0)//if parent is root
	    {
			grandParentOffset = vec3(0.0f, 0.0f, 0.0f);
		}


	    else if(legalID(p->pID, i))
	    {
	     	grandParentOffset = this->joints.at(p->pID)->offset;
	    	vec3 parent_t = glm::normalize(p->offset - grandParentOffset);
		    
		    //use parent's t to find parent's n
		    vec3 parent_n(0.0, 0.0, 0.0);
		    vec3 dummy(0.0, 0.0, 0.0);	
			calculateAxes(parent_t, parent_n, dummy);

			// cout << "i: " << i << "\n";
			// cout << "pID" << p->ID << "\n";
			// cout << "parent_t: "<< parent_t << "\n";
			// cout << "parent_n: "<< parent_n << "\n";

		    //then find angle between parent's t and child's t
		    //and find the 4 by 4 rotation matrix which will rotate
		    //and given vector from the parent to the child
		    //by the calculated angle around the parent's n axis
		    float angle = acos(dot(parent_t, t));

		    // cout << "angle: " << angle << "\n";

		    R = rotate(angle, parent_n);

		    // cout << "R: " << R << "\n\n";
	    }
	}

	mat4 calculateA(vec3 t, vec3 n, vec3 b)
	{
		return  { t[0], n[0], b[0], 1.0f,
		   		  t[1], n[1], b[1], 1.0f,
				  t[2], n[2], b[2], 1.0f,
				  0.0f, 0.0f, 0.0f, 1.0f };
	}

	void calculateR2(mat4 &R, vec3 t, vec3 n, vec3 b)
	{
		R = { b[0], b[1], b[2], 1.0f,
			  n[0], n[1], n[2], 1.0f,
			  t[0], t[1], t[2], 1.0f,
			  0.0f, 0.0f, 0.0, 1.0f };
	}


	void calculateR3(mat4 &R, Bone* j, Bone* p)
	{
		vec3 n = j->n, b = j->b;
		vec3 np = p->n, bp = p->b;
		vec3 t = j->t, tp = p->t;

		R = glm::transpose(calculateA(tp, np, bp)) * calculateA(t, n, b);
	}

	void initSkeleton()
	{
        //create a bone for each pair of joints
        //or each root joint

		//root joint case
			// initRoot();

        for (int i = 1; i < this->joints.size(); i++) {
                Joint* j = this->joints.at(i);

                //if the joint HAS a parent
                if (legalID(j->pID, joints.size())) {
                    
                    Joint* p = this->joints.at(j->pID);
                    float l = glm::length(j->offset);
                    vec3 t = glm::normalize(j->offset);

					mat4 R;
					vec3 n, bAxis;
					calculateAxes(t, n, bAxis);
					calculateR2(R, t, n, bAxis);

                    //find the translation matrix to get from the parent
                    //origin to the child origin
 					mat4 T;
 					calculateBigT(T, j->offset);                   

                    Bone* b = new Bone(p, j, l, p->offset, t, T, R);
                    b->ID = bones.size();
		    this->bones.push_back(b);

    				vector<Bone*> jointBones;
                    try{
                    	jointBones = jointIDBoneMap.at(j->pID);
	                    jointBones.push_back(b);
                    	jointIDBoneMap[j->pID] = jointBones;
                    }
                    catch(const out_of_range &e)
                    {
                    	jointBones = vector<Bone *>();
                    	jointBones.push_back(b);
                    	jointIDBoneMap.insert(make_pair(j->pID, jointBones));
                    }
                }
        }


        // for(Bone* b: bones)
        // {
        // 	Joint * src = b->source;
        // 	Joint * dest = b->destination;

        // 	try{
        // 		vector<Bone*> children = jointIDBoneMap.at(dest->ID);
        // 		for(Bone* c: children)
        // 		{
        // 			calculateR3(c->R, c, b);
        // 		}
        // 	}
        // 	catch(const out_of_range &e){}

        // }

        setUpChildJoints();


	}

	vector<Bone*> retJointBones(int ID) {
		try {
			vector<Bone*> goodOutput = jointIDBoneMap.at(ID);
			return goodOutput;
		} catch(out_of_range &e) {
			vector<Bone*> badOutput;
			return badOutput; 
		}
	}

	void printIDBoneMap()
	{
		cout << "\n--------------------------------\n";
		for(auto element: jointIDBoneMap)
		{
			int key = element.first;
			vector<Bone *> listBones = element.second;
			for(Bone *b: listBones)
			{
				cout << "key: " << key << ", source: " <<  b->source->ID << ", dest: " << b->destination->ID << "\n"; 
			}
		}
		cout << "--------------------------------\n\n";
	}


	//THE FOLLOWING 2 FUNCTIONS ASSUME THAT:
	//Only the root has no parent.
	//All other joints have parent joints
	//And all the joints can be accessed hierarchially
	//starting from the root.

	//THIS ASSUMPTION MAY BE INCORRECT

	//put data into vertices and lines for openGL to use
	void initVertsNLines()
	{
		skel_vertices.clear();
		skel_lines.clear();
		skel_vertices.reserve(joints.size());

		vec3 root3 = joints.at(0)->offset;
		vec4 root4(root3[0], root3[1], root3[2], 1.0f);


		mat4 T;
		mat4 R;
		calculateBigT(T, root3);

		vec3 t = normalize(root3);
		vec3 n, b;
		calculateAxes(t, n, b);
		calculateR2(R, t, n, b);

		vec4 rootStart(0.0f, 0.0f, 0.0f, 1.0f);
		vec4 rootEnd = T * R * rootStart;

		skel_vertices.push_back(rootStart);
		skel_vertices.push_back(rootEnd);
		skel_lines.push_back(uvec2(0, 1));

		mat4 M(1.0f);

		initVertsNLinesHelper(bones.at(0), M);
	}

	//helper for initVertsNLines
	void initVertsNLinesHelper(Bone* curBone, mat4 &M)
	{		
		vec4 boneStartPos = M * curBone->T * vec4(0.0f, 0.0f, 0.0f, 1.0f);
		vec4 boneEndPos = M * curBone->T * curBone->S * vec4(curBone->length, 0.0f, 0.0f, 1.0f);
		M = M * curBone->T * curBone->S;


		//record the bone's world position and the mapping from parent to child
		skel_vertices.push_back(boneStartPos);
		skel_vertices.push_back(boneEndPos);
		skel_lines.push_back(uvec2(skel_vertices.size()-1, skel_vertices.size()-2));

		//if the current bone has a child
		//recurse over the child
		//passing in the current bone's world position
		//as the parent world position

		int destID = curBone->destination->ID;

		try{
			vector<Bone *> children = jointIDBoneMap.at(destID); 
			// cout << "did not throw exception\n";
			for(int idx = 0; idx<children.size(); ++idx)
			{
				Bone* childBone = children.at(idx);				
				initVertsNLinesHelper(childBone, M); 			
			}
		}
		catch(const out_of_range &e){
			cout << "threw exception!\n\n";
		

		}
	}


	vec3 vec3Mat4Mult(vec3 v, mat4 m)
	{
		vec4 V(v, 1.0f);
		V = m * V;
		return vec3(V[0], V[1], V[2]);
	}


	vec3 getTFromLineID(int lineID)
	{
		uvec2 line = skel_lines.at(lineID);
		Joint *j = joints.at(line[1]);
		return normalize(j->offset);
	}

	void updateOffsets(int ID, mat4 R)
	{
		if(!legalID(ID, joints.size()))
		{
			// cout << "not legal ID: " << ID << "\n\n";
			return;
		}

		joints[ID]->offset = vec3Mat4Mult(joints[ID]->offset, R); 

		vector<int> childIDs = getChildJoints(joints.at(ID));
		if(childIDs[0] == -1)
			return;
	
		for(int cID: childIDs)
		{
			joints[cID]->offset = vec3Mat4Mult(joints[cID]->offset, R); 
		}
	}


	void initVertsNLinesIter()
	{
		skel_vertices.clear();
		skel_lines.clear();
		// skel_vertices = vector<vec4>(joints.size(), vec4(-1.0f, -1.0f, -1.0f, -1.0f));

		// cout << "joints size " << joints.size() << "\n\n";
		for(int x = 0; x<joints.size(); ++x)
			skel_vertices.push_back(vec4(-1.0f, -1.0f, -1.0f, -1.0f));


		vec3 root3 = joints.at(0)->offset;
		vec4 root4(root3[0], root3[1], root3[2], 1.0f);

		mat4 T;
		mat4 R;
		calculateBigT(T, root3);

		vec3 t = normalize(root3);
		vec3 n, b;
		calculateAxes(t, n, b);
		calculateR2(R, t, n, b);

		vec4 rootStart(0.0f, 0.0f, 0.0f, 1.0f);
		vec4 rootEnd = T * R * rootStart;

		skel_vertices[0] = rootEnd;

		int parentIdx;
		for(int i = 1; i<joints.size(); ++i) 
		{	
			Joint *j = joints.at(i);
			if(skel_vertices[i][0]==-1 && skel_vertices[i][1]==-1 && skel_vertices[i][2]==-1 && skel_vertices[i][3]==-1)
			{
				skel_vertices[i] = 	translate(j->offset) * getParentWP(j->pID);
			}
		}

		for(int i = 0; i<bones.size(); ++i)
		{
			Bone* bone = bones.at(i);
			skel_lines.push_back(uvec2(bone->source->ID, bone->destination->ID));
			change_color.push_back(0.0f);
		}
	}


	vec4 getParentWP(int pID)
	{
		if(pID==0)
			return skel_vertices[0];

		Joint *p = joints.at(pID);
		int gID = -1;
		gID = p->pID;
		if(!legalID(gID, joints.size()))
		{
			cout << "p is: " << p->ID << "\n";		
		}
		else
		{
			if(skel_vertices[pID][0]==-1 && skel_vertices[pID][1]==-1 && skel_vertices[pID][2]==-1 && skel_vertices[pID][3]==-1)
				return skel_vertices[pID];

			return translate(p->offset) * getParentWP(gID);
		}
	}
	// FIXME: create skeleton and bone data structures
};

struct Mesh {
	Mesh();
	~Mesh();
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec4> animated_vertices;
	std::vector<glm::uvec3> faces;
	std::vector<glm::vec4> vertex_normals;
	std::vector<glm::vec4> face_normals;
	std::vector<glm::vec2> uv_coordinates;
	std::vector<Material> materials;
	BoundingBox bounds;
	Skeleton skeleton;

	void loadpmd(const std::string& fn);
	void updateAnimation();
	int getNumberOfBones() 
	{ 
		return skeleton.bones.size();
	}
	glm::vec3 getCenter() const { return 0.5f * glm::vec3(bounds.min + bounds.max); }
private:
	void computeBounds();
	void computeNormals();
};

#endif


//cd ..; make; cd bin; ./skinning ../../assets/pmd/Neru_Akita.pmd
//cd ..; make; cd bin; valgrind ./skinning ../../assets/pmd/Neru_Akita.pmd
