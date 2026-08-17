#include "core/asset/glTFLoader.h"
#include "core/asset/Collection.h"
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"

#include "deps/europasoft-json/Source/Parser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <string>
#include <string_view>
#include <map>
#include <limits>
#include <utility>

namespace AssetSystem
{
	using MeshBuilder = EngineCore::MeshBuilder;

	// glTF 2.0 binary https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
	namespace GLB
	{
		inline constexpr uint32_t MAGIC = 0x46546C67; // "glTF"

		enum class EChunkType : uint32_t
		{
			JSON = 0x4E4F534A,
			BIN = 0x004E4942
		};

		// binary header structures according to glTF 2.0 specification
#pragma pack(push, 1)
		struct Header
		{
			uint32_t magic;
			uint32_t version;
			uint32_t length; // total file size
		};

		struct ChunkHeader
		{
			uint32_t chunkLength;
			EChunkType chunkType;
		};
#pragma pack(pop)

		// internal descriptors to map binary data
		struct BufferView
		{
			size_t byteOffset = 0;
			size_t byteLength = 0;
			size_t byteStride = 0;
		};

		enum class EComponentType : uint32_t
		{
			NONE = 0,
			FLOAT = 5126,
			UNSIGNED_SHORT = 5123,
			UNSIGNED_INT = 5125,
			BYTE = 5120,
			UNSIGNED_BYTE =	5121,
			SHORT =	5122
		};

		namespace StructType
		{
			inline constexpr std::string_view SCALAR = "SCALAR";
			inline constexpr std::string_view VEC2 = "VEC2";
			inline constexpr std::string_view VEC3 = "VEC3";
			inline constexpr std::string_view VEC4 = "VEC4";
			inline constexpr std::string_view MAT2 = "MAT2";
			inline constexpr std::string_view MAT3 = "MAT3";
			inline constexpr std::string_view MAT4 = "MAT4";
		}

		struct Accessor
		{
			size_t bufferViewIndex = 0;
			size_t byteOffset = 0;
			EComponentType componentType = EComponentType::NONE;
			size_t count = 0;
			std::string_view structType; // for example "VEC3"
		};
	
		inline constexpr size_t NO_MESH = std::numeric_limits<size_t>::max();

		struct Node
		{
			std::string name;
			Transform transform;
			size_t meshIndex = NO_MESH;
			std::vector<size_t> childrenIndices;
		};

		struct Mesh
		{
			std::string name;
			std::shared_ptr<MeshBuilder> geometry;
		};
	}

	struct GLBParser
	{
		static std::vector<GLB::BufferView> parseBufferViews(std::map<std::string, JSON::ObjectPtr>& gltf);
		static std::vector<GLB::Accessor> parseAccessors(std::map<std::string, JSON::ObjectPtr>& gltf);
		static std::vector<std::unique_ptr<GLB::Mesh>> parseMeshes(
					const JSON::ObjectPtr& meshesJson, 
					const std::vector<GLB::Accessor>& accessors,
					const std::vector<GLB::BufferView>& bufferViews, 
					const std::vector<uint8_t>& bin);
		static std::vector<GLB::Node> getNodes(std::map<std::string, JSON::ObjectPtr>& gltf);
		static void buildHierarchyRecursive(
					size_t nodeIdx,
					Collection* currentCollection,
					Collection* rootCollection,
					const std::vector<GLB::Node>& nodes,
					const std::vector<std::unique_ptr<GLB::Mesh>>& meshes,
					const std::filesystem::path& sourcePath);
	};

	std::unique_ptr<Collection> glTF::load(const std::filesystem::path& path)
	{
		std::ifstream file(makePath(path), std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open GLB file: " + path.string());
		}

		// read GLB Header
		GLB::Header header;
		file.read(reinterpret_cast<char*>(&header), sizeof(GLB::Header));
		if (header.magic != GLB::MAGIC || header.version != 2)
		{
			throw std::runtime_error("Invalid GLB file or unsupported version");
		}

		std::string jsonString;
		std::vector<uint8_t> binBuffer;

		// read chunks
		while (file.tellg() < static_cast<std::streampos>(header.length))
		{
			GLB::ChunkHeader chunkHeader;
			file.read(reinterpret_cast<char*>(&chunkHeader), sizeof(GLB::ChunkHeader));

			if (chunkHeader.chunkType == GLB::EChunkType::JSON)
			{
				jsonString.resize(chunkHeader.chunkLength);
				file.read(&jsonString[0], chunkHeader.chunkLength);
			}
			else if (chunkHeader.chunkType == GLB::EChunkType::BIN)
			{
				binBuffer.resize(chunkHeader.chunkLength);
				file.read(reinterpret_cast<char*>(binBuffer.data()), chunkHeader.chunkLength);
			}
			else
			{
				// skip unknown chunk
				file.seekg(chunkHeader.chunkLength, std::ios::cur);
			}
		}

		// parse JSON chunk
		JSON::Object root;
		JSON::Result jsonResult = JSON::load(jsonString, root);
		if (jsonResult != JSON::Result::OK || root.size() == 0 || not root.isContainer())
		{
			throw std::runtime_error("Failed to parse internal GLTF JSON chunk");
		}

		std::map<std::string, JSON::ObjectPtr> gltf = root[0].map();

		// parse bufferviews and accessors
		std::vector<GLB::BufferView> bufferViews = GLBParser::parseBufferViews(gltf);
		std::vector<GLB::Accessor> accessors = GLBParser::parseAccessors(gltf);

		// extract meshes and geometry
		std::vector<std::unique_ptr<GLB::Mesh>> meshes;
		if (gltf.find("meshes") != gltf.end())
		{
			meshes = GLBParser::parseMeshes(gltf["meshes"], accessors, bufferViews, binBuffer);
		}

		// get list of nodes
		auto nodes = GLBParser::getNodes(gltf);

		// create the root collection for this file
		auto rootCollection = std::make_unique<Collection>(path.stem().string());
		rootCollection->importSourceInfo.originalFilePath = path;
		rootCollection->importSourceInfo.fileFormat = AssetFileFormat::GLTF_GLB;

		// identify root nodes (nodes that are not children of any other node)
		std::vector<bool> isChild(nodes.size(), false);
		for (const auto& node : nodes)
		{
			for (size_t childIdx : node.childrenIndices)
			{
				if (childIdx < isChild.size())
				{
					isChild[childIdx] = true;
				}
			}
		}

		// build the hierarchy under rootCollection
		for (size_t i = 0; i < nodes.size(); ++i)
		{
			if (!isChild[i])
			{
				GLBParser::buildHierarchyRecursive(i, rootCollection.get(), rootCollection.get(), nodes, meshes, path);
			}
		}

		return rootCollection;
	}

	std::vector<GLB::BufferView> GLBParser::parseBufferViews(std::map<std::string, JSON::ObjectPtr>& gltf)
	{
		std::vector<GLB::BufferView> views;
		if (gltf.find("bufferViews") == gltf.end()) return views;

		for (const auto& bvJson : *gltf["bufferViews"])
		{
			auto fields = bvJson->map();
			GLB::BufferView bv;
			if (fields.find("byteOffset") != fields.end())
				bv.byteOffset = fields["byteOffset"]->getValueInt();
			if (fields.find("byteLength") != fields.end())
				bv.byteLength = fields["byteLength"]->getValueInt();
			if (fields.find("byteStride") != fields.end())
				bv.byteStride = fields["byteStride"]->getValueInt();
			views.push_back(bv);
		}
		return views;
	}

	std::vector<GLB::Accessor> GLBParser::parseAccessors(std::map<std::string, JSON::ObjectPtr>& gltf)
	{
		std::vector<GLB::Accessor> accessors;
		if (gltf.find("accessors") == gltf.end()) return accessors;

		for (const auto& accJson : *gltf["accessors"])
		{
			auto fields = accJson->map();
			GLB::Accessor acc;
			if (fields.find("bufferView") != fields.end())
				acc.bufferViewIndex = fields["bufferView"]->getValueInt();
			if (fields.find("byteOffset") != fields.end())
				acc.byteOffset = fields["byteOffset"]->getValueInt();
			acc.componentType = static_cast<GLB::EComponentType>(static_cast<uint32_t>(fields["componentType"]->getValueInt()));
			acc.count = fields["count"]->getValueInt();
			acc.structType = fields["type"]->getValue();
			accessors.push_back(acc);
		}
		return accessors;
	}

	// unpacks binary data into vertices and indices
	std::vector<std::unique_ptr<GLB::Mesh>> GLBParser::parseMeshes(
		const JSON::ObjectPtr& meshesJson,
		const std::vector<GLB::Accessor>& accessors,
		const std::vector<GLB::BufferView>& bufferViews,
		const std::vector<uint8_t>& bin)
	{
		std::vector<std::unique_ptr<GLB::Mesh>> outMeshes;

		for (const auto& meshJson : *meshesJson)
		{
			auto meshFields = meshJson->map();

			// store geometry as shared pointer, so multiple mesh assets can instance the same data
			auto mesh = std::make_unique<GLB::Mesh>();
			mesh->geometry = std::make_shared<MeshBuilder>();
			MeshBuilder& mb = *mesh->geometry;

			if (meshFields.find("name") != meshFields.end())
			{
				mesh->name = meshFields["name"]->getValue();
			}

			for (const auto& primJson : *meshFields["primitives"])
			{
				auto primFields = primJson->map();
				auto attributes = primFields["attributes"]->map();

				// read positions
				size_t posAccIdx = attributes["POSITION"]->getValueInt();
				const GLB::Accessor& posAcc = accessors[posAccIdx];
				const GLB::BufferView& posView = bufferViews[posAcc.bufferViewIndex];
				const float* posPtr = reinterpret_cast<const float*>(&bin[posView.byteOffset + posAcc.byteOffset]);

				mb.vertices.resize(posAcc.count);
				for (size_t i = 0; i < posAcc.count; ++i)
				{
					mb.vertices[i].position = { posPtr[i * 3], posPtr[i * 3 + 1], posPtr[i * 3 + 2] };
				}

				// read normals
				if (attributes.find("NORMAL") != attributes.end())
				{
					size_t normAccIdx = attributes["NORMAL"]->getValueInt();
					const GLB::Accessor& normAcc = accessors[normAccIdx];
					const GLB::BufferView& normView = bufferViews[normAcc.bufferViewIndex];
					const float* normPtr = reinterpret_cast<const float*>(&bin[normView.byteOffset + normAcc.byteOffset]);
					for (size_t i = 0; i < normAcc.count; ++i)
					{
						mb.vertices[i].normal = { normPtr[i * 3], normPtr[i * 3 + 1], normPtr[i * 3 + 2] };
					}
				}

				// read UVs 
				if (attributes.find("TEXCOORD_0") != attributes.end())
				{
					size_t uvAccIdx = attributes["TEXCOORD_0"]->getValueInt();
					const GLB::Accessor& uvAcc = accessors[uvAccIdx];
					const GLB::BufferView& uvView = bufferViews[uvAcc.bufferViewIndex];
					const float* uvPtr = reinterpret_cast<const float*>(&bin[uvView.byteOffset + uvAcc.byteOffset]);
					for (size_t i = 0; i < uvAcc.count; ++i)
					{
						mb.vertices[i].uv = glm::vec2{ uvPtr[i * 2], uvPtr[i * 2 + 1] };
					}
				}

				// read vertex colors
				if (attributes.find("COLOR_0") != attributes.end())
				{
					size_t colAccIdx = attributes["COLOR_0"]->getValueInt();
					const GLB::Accessor& colAcc = accessors[colAccIdx];
					const GLB::BufferView& colView = bufferViews[colAcc.bufferViewIndex];
					const float* colPtr = reinterpret_cast<const float*>(&bin[colView.byteOffset + colAcc.byteOffset]);
					for (size_t i = 0; i < colAcc.count; ++i)
					{
						mb.vertices[i].color = glm::vec3{ colPtr[i * 4], colPtr[i * 4 + 1], colPtr[i * 4 + 2] };
					}
				}

				// read indices
				if (primFields.find("indices") != primFields.end())
				{
					size_t indAccIdx = primFields["indices"]->getValueInt();
					const GLB::Accessor& indAcc = accessors[indAccIdx];
					const GLB::BufferView& indView = bufferViews[indAcc.bufferViewIndex];
					const uint8_t* rawIndices = &bin[indView.byteOffset + indAcc.byteOffset];

					mb.indices.resize(indAcc.count);
					if (indAcc.componentType == GLB::EComponentType::UNSIGNED_SHORT)
					{
						const uint16_t* ptr = reinterpret_cast<const uint16_t*>(rawIndices);
						for (size_t i = 0; i < indAcc.count; ++i) mb.indices[i] = ptr[i];
					}
					else if (indAcc.componentType == GLB::EComponentType::UNSIGNED_INT)
					{
						const uint32_t* ptr = reinterpret_cast<const uint32_t*>(rawIndices);
						for (size_t i = 0; i < indAcc.count; ++i) mb.indices[i] = ptr[i];
					}
				}
			}
			outMeshes.push_back(std::move(mesh));
		}
		return outMeshes;
	}

	// builds a flat list of nodes
	std::vector<GLB::Node> GLBParser::getNodes(std::map<std::string, JSON::ObjectPtr>& gltf)
	{
		std::vector<GLB::Node> nodes;
		size_t nodeIndex = 0;
		for (const auto& nodeJson : *gltf["nodes"])
		{
			auto fields = nodeJson->map();
			auto node = GLB::Node();

			if (fields.find("name") != fields.end())
				node.name = fields["name"]->getValue();
			else
				node.name = "Node_" + std::to_string(nodeIndex);

			if (fields.find("mesh") != fields.end())
				node.meshIndex = fields["mesh"]->getValueInt();
			else
				node.meshIndex = GLB::NO_MESH;

			if (fields.find("translation") != fields.end())
			{
				auto t = fields["translation"]->vector();
				node.transform.translation = Vec{ std::stof(t[0]), std::stof(t[1]), std::stof(t[2]) };
			}
			if (fields.find("scale") != fields.end())
			{
				auto s = fields["scale"]->vector();
				node.transform.scale = Vec{ std::stof(s[0]), std::stof(s[1]), std::stof(s[2]) };
			}

			if (fields.find("children") != fields.end())
			{
				for (const auto& childIdxStr : fields["children"]->vector())
				{
					node.childrenIndices.push_back(std::stoull(childIdxStr));
				}
			}

			nodes.push_back(node);
			nodeIndex++;
		}
		return nodes;
	}

	void GLBParser::buildHierarchyRecursive(
		size_t nodeIdx,
		Collection* currentCollection,
		Collection* rootCollection,
		const std::vector<GLB::Node>& nodes,
		const std::vector<std::unique_ptr<GLB::Mesh>>& meshes,
		const std::filesystem::path& sourcePath)
	{
		const GLB::Node& glbNode = nodes[nodeIdx];

		// add nested collection
		if (glbNode.childrenIndices.size())
		{
			auto subCollection = std::make_unique<Collection>(glbNode.name);
			subCollection->parent = currentCollection;
			subCollection->importSourceInfo.importCollection = rootCollection;

			for (size_t childIdx : glbNode.childrenIndices)
			{
				buildHierarchyRecursive(childIdx, subCollection.get(), rootCollection, nodes, meshes, sourcePath);
			}

			currentCollection->nested.push_back(std::move(subCollection));
		}

		// add mesh asset
		if (glbNode.meshIndex != GLB::NO_MESH && glbNode.meshIndex < meshes.size())
		{
			auto meshAsset = std::make_unique<MeshAsset>(glbNode.name);
			meshAsset->geometry = meshes[glbNode.meshIndex]->geometry; // multiple assets may reference the same geometry data
			meshAsset->importSourceInfo.importCollection = rootCollection;
			currentCollection->assets.push_back(std::move(meshAsset));
		}
	}


}