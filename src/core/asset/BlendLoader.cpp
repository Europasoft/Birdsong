#include "core/asset/BlendLoader.h"

#define FBTBLEND_IMPLEMENTATION
#include "thirdparty/fbtblend-header-only/502_lts/fbtBlend.h"

#include <string>
#include <vector>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <iostream>

namespace EngineCore
{
	using string = std::string;

	// blender DNA_object_types.h
	enum class ObjectType : short
	{
		OB_EMPTY = 0,
		OB_MESH = 1,
		OB_LAMP = 10,
		OB_CAMERA = 11,
		OB_ARMATURE = 25
	};

	// blender DNA_collection_types.h
	enum eCollection_Flag : uchar
	{
		/** Disable in viewports. */
		COLLECTION_HIDE_VIEWPORT = (1 << 0),
		/** Not selectable in viewport. */
		COLLECTION_HIDE_SELECT = (1 << 1),
		/** Disable in renders. */
		COLLECTION_HIDE_RENDER = (1 << 3),
		/** Runtime: object_cache is populated. */
		COLLECTION_HAS_OBJECT_CACHE = (1 << 4),
		/** Is master collection embedded in the scene. */
		COLLECTION_IS_MASTER = (1 << 5),
		/** for object_cache_instanced. */
		COLLECTION_HAS_OBJECT_CACHE_INSTANCED = (1 << 6),
	};

	BlendLoader::BlendLoader() = default;
	BlendLoader::~BlendLoader() = default;

	bool BlendLoader::loadFromFile(std::string_view path)
	{
		fbtBlend fp;
		const char* filePath = "";
		if (fp.parse(std::string(path).c_str()) != fbtFile::FS_OK) return false;

		fbtList& objects = fp.m_object;
		if (not objects.first) return false;

		for (fbtList::Link* link = objects.first; link; link = link->next)
		{
			Blender::Object* obj = reinterpret_cast<Blender::Object*>(link);
			//printf(".BLEND OBJECT: \"%s\"\n", getBlendObjectName(obj));

			if (obj->type == static_cast<short>(ObjectType::OB_MESH))
			{
				// mesh
				const Blender::Mesh* mesh = reinterpret_cast<Blender::Mesh*>(obj->data);
				//printf(".BLEND MESH: \"%s\"\n", getBlendMeshName(mesh));
			}
			else
			{
				// object type other than mesh
			}
		}

		return true;
	}

	// matching blender 5.2 DNA_customdata_types.h (only some values included here)
	enum eCustomDataType : int
	{
		CD_PROP_FLOAT3 = 48,
		CD_PROP_FLOAT2 = 49,
		CD_PROP_INT32 = 11,
		CD_PROP_COLOR = 47,
		CD_PROP_BYTE_COLOR = 17,
		// Used as temporary storage for some areas that support interpolating custom normals.
		// Using a separate type from generic 3D vectors is a simple way of keeping values normalized.
		CD_NORMAL = 8,
		CD_MDEFORMVERT = 2, /* Array of #MDeformVert. */
	};

	// matching blender 5.2 BKE_attribute_enums.hh
	enum class AttrType : short
	{
		Bool = 0,
		Int8 = 1,
		Int16_2D = 2,
		Int32 = 3,
		Int32_2D = 4,
		Float = 5,
		Float2 = 6,
		Float3 = 7,
		Float4x4 = 8,
		ColorByte = 9,
		ColorFloat = 10,
		Quaternion = 11,
		String = 12,
		Float4 = 13,
	};

	// matching blender 5.2 BKE_attribute_enums.hh
	enum class AttrDomain : short
	{
		/* Used to choose automatically based on other data. */
		Auto = -1,
		/* Mesh, Curve or Point Cloud Point. */
		Point = 0,
		/* Mesh Edge. */
		Edge = 1,
		/* Mesh Face. */
		Face = 2,
		/* Mesh Corner. */
		Corner = 3,
		/* A single curve in a larger curve data-block. */
		Curve = 4,
		/* Instance. */
		Instance = 5,
		/* A layer in a grease pencil data-block. */
		Layer = 6,
	};

	class fbtFileDummy : public fbtFile
	{
	public:
		using fbtFile::FileStartsWith; // get access to protected function
	};

	struct Vert
	{
		float x, y, z;
	};

	struct LoadedBlendMesh
	{
		std::vector<std::string> collectionPath;
		std::vector<Vert> verts;
	};

	std::unordered_set<const Blender::Collection*> gatherChildCollections(const fbtList& collections)
	{
		std::unordered_set<const Blender::Collection*> out;
		for (fbtList::Link* l = collections.first; l; l = l->next)
		{
			auto* collection = reinterpret_cast<Blender::Collection*>(l);
			for (void* c = collection->children.first; c; )
			{
				auto* child = static_cast<Blender::CollectionChild*>(c);
				c = child->next;
				if (child->collection)
				{
					out.insert(reinterpret_cast<Blender::Collection*>(child->collection));
				}
			}
		}
		return out;
	}

	bool BlendLoader::loadFromFileByCollection(std::string_view path)
	{
		fp = std::unique_ptr<fbtBlend>(new fbtBlend);

		assert(fbtFileDummy::FileStartsWith(string(path).c_str(), "BLENDER") && ".blend file is compressed, currently not supported");

		if (fp->parse(string(path).c_str()) != fbtFile::FS_OK) return false;

		const fbtList& collections = fp->m_group;
		if (!collections.first) return false;
		// TODO: if not collections exist, loop through objects directly

		const auto childCollections = gatherChildCollections(collections);
	
		// process root collections
		for (fbtList::Link* l = collections.first; l; l = l->next)
		{
			auto* collection = reinterpret_cast<Blender::Collection*>(l);

			// if it's a child elsewhere, skip it
			if (childCollections.find(collection) != childCollections.end()) continue;

			std::vector<string> currentCollectionPath = { string(collection->id.name) };
			recurseCollection(collection, currentCollectionPath);
		}

		return !loaded.empty();
	}

	void BlendLoader::recurseCollection(const Blender::Collection* collection, std::vector<string>& collectionPath)
	{
		// gather objects in this collection
		recurseCollectionObjs(collection, collectionPath);

		for (void* c = collection->children.first; c;)
		{
			auto* child = static_cast<Blender::CollectionChild*>(c);
			c = child->next;
			Blender::Collection* childCollection = reinterpret_cast<Blender::Collection*>(child->collection);
			collectionPath.push_back(string(childCollection->id.name));

			// do the same for further nested child collections
			recurseCollection(childCollection, collectionPath);
		}
		collectionPath.pop_back();
	}

	void BlendLoader::recurseCollectionObjs(const Blender::Collection* collection, std::vector<string>& collectionPath)
	{
		for (void* g = collection->gobject.first; g;)
		{
			auto* gobj = static_cast<Blender::CollectionObject*>(g);
			g = gobj->next;

			// object inside a collection
			Blender::Object* obj = gobj->ob;

			if (obj && obj->type == static_cast<short>(ObjectType::OB_MESH))
			{
				// mesh (see blender DNA_mesh_types.h)
				const Blender::Mesh* mesh = reinterpret_cast<Blender::Mesh*>(obj->data);
				//printf(".BLEND MESH: \"%s\"\n", mesh->id.name);
				extractVertices(*mesh);
			}
		}
	}

	void BlendLoader::extractVertices(const Blender::Mesh& mesh)
	{
		// "totvert" in .blend files is now called "verts_num" in official blender code (see blender DNA_mesh_types.h)
		if (mesh.totvert < 3) return;

		const void* attributeData = getBlenderAttributeData(mesh.attribute_storage, AttrDomain::Point, AttrType::Float3);
		const auto* attrArr = reinterpret_cast<const Blender::AttributeArray*>(attributeData);

		// INCORRECT, RESULTS IN GARBAGE VERTEX DATA
		const auto* vertices = static_cast<const float(*)[3]>(attrArr->data);


		/*uint8_t* raw_bytes = static_cast<uint8_t*>(attrArr->data);
		// Print the first 32 raw hex bytes of arr->data to see where the floats begin
		printf("Raw bytes at arr->data:\n");
		for (int i = 0; i < 32; i++)
		{
			printf("%02X ", raw_bytes[i]);
		}
		printf("\n");

		// 1. Cast data to a pointer table
		uintptr_t* ptr_table = static_cast<uintptr_t*>(attrArr->data);
		// Skip any initial header entries (looks like the real pointers start around index 2 or 3 in 64-bit words)
		// Let's test dereferencing the pointer found at offset 16 (index 2):
		float* vert0_pos = reinterpret_cast<float*>(ptr_table[2]);
		printf("Vert 0 XYZ: %f, %f, %f\n", vert0_pos[0], vert0_pos[1], vert0_pos[2]);
		*/

		//// Inspect the first 3 vertices
		for (int i = 0; i < min((int)mesh.totvert, 3); ++i)
		{
			printf("Vert %d: %f, %f, %f (Raw Hex: %08X %08X %08X)\n",
				i,
				vertices[i][0], vertices[i][1], vertices[i][2],
				*(uint32_t*)&vertices[i][0],
				*(uint32_t*)&vertices[i][1],
				*(uint32_t*)&vertices[i][2]
			);
		}

		// read vertex positions
		if (vertices)
		{
			loaded.push_back(std::make_unique<LoadedBlendMesh>());
			for (int i = 0; i < mesh.totvert; ++i)
			{
				loaded.back()->verts.push_back(
					Vert{
						.x = vertices[i][0],
						.y = vertices[i][1],
						.z = vertices[i][2]
					});
			}
		}
	}

	// just so we can call protected functions
	class fbtFileAccess : public fbtFile
	{
	public:
		using fbtFile::findPtr;
	};

	void* BlendLoader::getBlenderAttributeData(const Blender::AttributeStorage& attribute_storage, AttrDomain domain, AttrType type)
	{
		const auto attrNum = attribute_storage.dna_attributes_num;
		Blender::Attribute* attr = attribute_storage.dna_attributes;
		for (int i = 0; i < attrNum; ++i, ++attr)
		{
			// "data_type" is an AttrType enum value
			// "domain" is an AttrDomain enum value
			// "storage_type" is an AttrStorageType: 0 = array, 1 = single value (see blender BKE_attribute_enums.hh)
			if (attr->data_type == static_cast<short>(type) && attr->domain == static_cast<short>(domain))
			{
				assert(attr->storage_type == 0 && "expected array, but attribute storage_type was 1 (single value)");
				// do some trickery to attempt to get the actual vertex array
				//return reinterpret_cast<fbtFileAccess*>(static_cast<fbtFile*>(fp.get()))->findPtr((FBTsize)attr->data);
				return attr->data;
			}
		}
		return nullptr;
	}

}