#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <vector>

namespace Blender
{
	struct Collection;
	struct Object;
	struct Mesh;
	struct AttributeStorage;
}
class fbtBlend;

namespace EngineCore
{
	enum class AttrDomain : short;
	enum class AttrType : short;
	struct LoadedBlendMesh;

	class BlendLoader
	{
	public:
		BlendLoader();
		~BlendLoader();

		bool loadFromFile(std::string_view path);
		bool loadFromFileByCollection(std::string_view path);

		void* getBlenderAttributeData(const Blender::AttributeStorage& attribute_storage, AttrDomain domain, AttrType type);
		void extractVertices(const Blender::Mesh& mesh);
		void recurseCollectionObjs(const Blender::Collection* collection, std::vector<std::string>& collectionPath);
		void recurseCollection(const Blender::Collection* collection, std::vector<std::string>& collectionPath);

	protected:
		std::unique_ptr<fbtBlend> fp;
		std::vector<std::unique_ptr<LoadedBlendMesh>> loaded;

		
	};


	class BlendCollection
	{

	};

	class BlendObject
	{

	};
















}