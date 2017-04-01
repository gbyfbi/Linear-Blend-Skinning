/*
 * Notes for hacking:
 * libmmd is a header only library. Keeping mmd.hxx only included by a source
 * file rather a header file can reduce compiling time.
 */
#include "mmdadapter.h"
#include "mmd/mmd.hxx"
#include "bitmap.h"
#include <iostream>
#include <exception>
#include <unordered_map>

using std::endl;

namespace {
	glm::vec4 conv(const mmd::Vector4f& rhs)
	{
		glm::vec4 lhs;
		lhs[0] = rhs.v[0];
		lhs[1] = rhs.v[1];
		lhs[2] = rhs.v[2];
		lhs[3] = rhs.v[3];
		return lhs;
	}
	glm::vec4 conv(const mmd::Vector3f& rhs)
	{
		glm::vec4 lhs;
		lhs[0] = rhs.v[0];
		lhs[1] = rhs.v[1];
		lhs[2] = rhs.v[2];
		return lhs;
	}
	glm::vec2 conv(const mmd::Vector2f& rhs)
	{
		glm::vec2 lhs;
		lhs[0] = rhs.v[0];
		lhs[1] = rhs.v[1];
		return lhs;
	}
	glm::uvec3 conv(const mmd::Vector3D<std::uint32_t>& rhs)
	{
		glm::uvec3 lhs;
		lhs[0] = rhs.v[0];
		lhs[1] = rhs.v[1];
		lhs[2] = rhs.v[2];
		return lhs;
	}
};

class MMDAdapter {
	bool isBoneHasRoot0(int bone_id)
	{
		do {
			const auto& bone = model_.GetBone(bone_id);
			int parent = bone.GetParentIndex();
			if (parent < 0)
				break;
			bone_id = parent;
		} while (true);
		return bone_id == 0;
	}
public:
	MMDAdapter()
	{
	}

	~MMDAdapter()
	{
	}

	bool open(const std::string& fn)
	{
		try {
			mmd::FileReader file(fn);
			mmd::PmdReader reader(file);
			reader.ReadModel(model_);

			size_t useful_bone_id = 0;
			for (size_t i = 0; i < model_.GetBoneNum(); i++) {
				if (!isBoneHasRoot0(i)) {
					pmd_bone_to_useful_bone_[i] = -1;
					continue;
				}
				useful_bone_to_pmd_bone_[useful_bone_id] = i;
				pmd_bone_to_useful_bone_[i] = useful_bone_id;
				useful_bone_id++;
			}
		} catch (std::exception& e) {
			std::cerr << e.what() << endl;
			return false;
		}
		return true;
	}

	void getMesh(std::vector<glm::vec4>& V,
		     std::vector<glm::uvec3>& F,
		     std::vector<glm::vec4>& N,
		     std::vector<glm::vec2>& UV)
	{
		size_t nv = model_.GetVertexNum();
		V.resize(nv);
		N.resize(nv);
		UV.resize(nv);
		for (size_t i = 0; i < nv; i++) {
			const auto& v = model_.GetVertex(i);
			V[i] = conv(v.GetCoordinate());
			N[i] = conv(v.GetNormal());
			UV[i] = conv(v.GetUVCoordinate());
			N[i][3] = 0.0f;
			V[i][3] = 1.0f;
		}
		size_t nf = model_.GetTriangleNum();
		F.resize(nf);
		for (size_t i = 0; i < nf; i++) {
			const auto& f = model_.GetTriangle(i);
			F[i] = conv(f);
		}
	}

	void getMaterial(std::vector<Material>& vm)
	{
		std::map<std::string, std::shared_ptr<Image>> loaded_tex;
		vm.resize(model_.GetPartNum());
		for (size_t i = 0; i < vm.size(); i++) {
			const auto& part = model_.GetPart(i);
			const auto& material = part.GetMaterial();
			vm[i].diffuse = conv(material.GetDiffuseColor());
			vm[i].ambient = conv(material.GetAmbientColor());
			vm[i].specular = conv(material.GetSpecularColor());
			vm[i].shininess = material.GetShininess();
			vm[i].offset = part.GetBaseShift();
			vm[i].nfaces = part.GetTriangleNum();
			const mmd::Texture* tex = material.GetTexture();
			if (!tex)
				continue;
			std::string texfn = mmd::UTF16ToNativeString(tex->GetTexturePath());
			if (texfn.empty())
				continue;
			auto iter = loaded_tex.find(texfn);
			if (iter != loaded_tex.end()) {
				vm[i].texture = iter->second;
				continue;
			}
			auto image = std::make_shared<Image>();
			std::cerr << __func__ << " is trying to load texture " << texfn << std::endl;
			if (!readBMP(texfn.data(), *image))
				continue;
			std::cerr << __func__ << " successfully loaded texture " << texfn << std::endl;
#if 0
			std::cerr << "\t\ttesting RGB ";
			for (int i = 0; i < 64; i++)
				std::cerr << int(image->bytes[i]) << ' ';
			std::cerr << std::endl;
			//std::cerr << "\t\ttesting RGB " << int(image->bytes[0]) << ' ' << int(image->bytes[1]) << ' ' << int(image->bytes[2]) << ' ' << std::endl;
			//std::cerr << "\t\ttesting RGB " << int(image->bytes[256]) << ' ' << int(image->bytes[257]) << ' ' << int(image->bytes[258]) << ' ' << std::endl;
#endif
			loaded_tex[texfn] = image;
			vm[i].texture = image;
		}
	}

	bool getJoint(int useful_bone_id, glm::vec3& offset, int& parent)
	{
		if (useful_bone_id >= int(useful_bone_to_pmd_bone_.size()) || useful_bone_id < 0)
			return false;
		int id = useful_bone_to_pmd_bone_[useful_bone_id];
		const auto& bone = model_.GetBone(id);
		size_t mmd_parent = bone.GetParentIndex();
		if (mmd_parent == mmd::nil) {
			parent = -1;
			offset = glm::vec3(conv(bone.GetPosition()));
		} else {
			parent = pmd_bone_to_useful_bone_[int(mmd_parent)];
			const auto& parent_bone = model_.GetBone(mmd_parent);
			offset = glm::vec3(conv(bone.GetPosition() - parent_bone.GetPosition()));
		}
#if 0
		std::cerr << "Joint " << id << " type " << 
			bone.IsChildUseID() << ' ' << 
		bone.IsRotatable() << ' ' << 
		bone.IsMovable() << ' ' << 
		bone.IsVisible() << ' ' << 
		bone.IsControllable() << ' ' << 
		bone.IsHasIK() << ' ' << 
		bone.IsAppendRotate() << ' ' <<  // Remove these bones
		bone.IsAppendTranslate() << ' ' << 
		bone.IsRotAxisFixed() << ' ' << 
		bone.IsUseLocalAxis() << ' ' << 
		bone.IsPostPhysics() << ' ' << 
		bone.IsReceiveTransform() << ' ' << std::endl;
#endif
		return true;
	}

	void getJointWeights(std::vector<SparseTuple>& tup)
	{
		constexpr int SKINNING_BDEF1 = mmd::Model::SkinningOperator::SKINNING_BDEF1;
		constexpr int SKINNING_BDEF2 = mmd::Model::SkinningOperator::SKINNING_BDEF2;
		constexpr int SKINNING_BDEF4 = mmd::Model::SkinningOperator::SKINNING_BDEF4;
		constexpr int SKINNING_SDEF = mmd::Model::SkinningOperator::SKINNING_SDEF;
		size_t nv = model_.GetVertexNum();
		tup.clear();
		tup.reserve(nv * 2);
		for (size_t i = 0; i < nv; i++) {
			const auto& v = model_.GetVertex(i);
			int skt = v.GetSkinningOperator().GetSkinningType();
			
			switch (skt) {
				case SKINNING_BDEF1:
					{
						const auto& bdef1 = v.GetSkinningOperator().GetBDEF1();
						auto bid = pmd_bone_to_useful_bone_[bdef1.GetBoneID()];
						if (bid >= 0)
							tup.emplace_back(bid, i, 1.0f);
					}
					break;
				case SKINNING_BDEF2:
					{
						const auto& bdef2 = v.GetSkinningOperator().GetBDEF2();
						auto bid0 = pmd_bone_to_useful_bone_[bdef2.GetBoneID(0)];
						auto bid1 = pmd_bone_to_useful_bone_[bdef2.GetBoneID(1)];
						if (bid0 >= 0 && bid1 >= 0) {
							tup.emplace_back(bid0, i, bdef2.GetBoneWeight());
							tup.emplace_back(bid1, i, 1.0f - bdef2.GetBoneWeight());
						}
					}
					break;
				case SKINNING_BDEF4:
					{
						const auto& bdef4 = v.GetSkinningOperator().GetBDEF4();
						for (int i = 0 ; i < 4; i++) {
							auto bid = pmd_bone_to_useful_bone_[bdef4.GetBoneID(i)];
							if (bid < 0)
								continue;
							tup.emplace_back(bid, i, bdef4.GetBoneWeight(i));
						}
					}
					break;
				case SKINNING_SDEF:
					std::cerr << "Unexcepted SkinningOperator " << v.GetSkinningOperator().GetSkinningType() << std::endl;
					throw -1;
					break;
			}
			//std::cerr << bdef2.GetBoneID(0) << "\t" << bdef2.GetBoneID(1) << "\t" << bdef2.GetBoneWeight() << endl;
		}
	}
private:
	mmd::Model model_;
	std::unordered_map<int, int> useful_bone_to_pmd_bone_, pmd_bone_to_useful_bone_;
};

MMDReader::MMDReader()
	: d_(new MMDAdapter)
{
}

MMDReader::~MMDReader()
{
}

bool MMDReader::open(const std::string& fn)
{
	return d_->open(fn);
}

void MMDReader::getMesh(std::vector<glm::vec4>& V,
		std::vector<glm::uvec3>& F,
		std::vector<glm::vec4>& N,
		std::vector<glm::vec2>& UV)
{
	d_->getMesh(V, F, N, UV);
}

void MMDReader::getMaterial(std::vector<Material>& vm)
{
	d_->getMaterial(vm);
}

bool MMDReader::getJoint(int id, glm::vec3& offset, int& parent)
{
	return d_->getJoint(id, offset, parent);
}

void MMDReader::getJointWeights(std::vector<SparseTuple>& tup)
{
	d_->getJointWeights(tup);
}
