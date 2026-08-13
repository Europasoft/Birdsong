#pragma once
#include "core/types/CommonTypes.h"

#include <vector>
#include <memory>
#include <concepts>

namespace EngineCore
{
	class Material;
	class EngineDevice;
	struct FrameContext;
	struct DrawContext;
	template <typename T>
	class InstanceBuffer;
	namespace ShaderInstanceData
	{
		struct UIElementInstanceData;
		struct TextGlyphInstanceData;
	}
}

namespace UI
{
	class RootElement;
	using UIInst = EngineCore::ShaderInstanceData::UIElementInstanceData;
	using GlyphInst = EngineCore::ShaderInstanceData::TextGlyphInstanceData;

	struct PreDrawData
	{
		Vec2 size;
		Vec2 position;
		Vec2 pivot;
	};

	class Element
	{
	protected:
		Element();
		
	public:
		virtual ~Element();
		Element(const Element&) = delete;
		Element& operator=(const Element&) = delete;
		Element(Element&&) = default;
		Element& operator=(Element&&) = default;

		Vec2 position;
		Vec2 size;
		Vec2 pivotPoint;
		Vec backgroundColor;
		float backgroundOpacity = 1.f;
		Vec2 cornerRadiusTop;
		Vec2 cornerRadiusBottom;
		std::shared_ptr<EngineCore::Material> material;
		std::vector<std::unique_ptr<Element>> nested;
		Element* parent = nullptr;
		RootElement* root = nullptr;
		uint32_t instanceDataIndex = 0;

	public:
		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);
		bool isRoot() const;
		bool isNextToRoot() const;
		bool isLeaf() const;
		void setCornerRadius(float r);

		template <typename T, typename... Args> requires std::derived_from<T, Element>
		T& addElement(Args&&... args)
		{
			auto e = Element::create<T>(std::forward<Args>(args)...);
			e->parent = this;
			e->root = root;
			T& result = *e;
			nested.push_back(std::move(e));
			postAddElement(result);
			return result;
		}

	public:
		virtual void preDrawRecursive(const EngineCore::FrameContext& f, const PreDrawData& parentData);
		virtual void drawRecursive(const EngineCore::FrameContext& f, EngineCore::Material*& m);

	protected:
		template <typename T, typename... Args> requires std::derived_from<T, Element>
		static std::unique_ptr<T> create(Args&&... args)
		{
			return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
		}
		virtual void postAddElement(Element& e);

		virtual void preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData);

		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data);
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m);
	};

	class RootElement : public Element
	{
	protected:
		RootElement(EngineCore::EngineDevice& device);

	public:
		~RootElement();
		static std::unique_ptr<RootElement> create(EngineCore::EngineDevice& device);

		void drawAll(const EngineCore::FrameContext& f);

		EngineCore::InstanceBuffer<GlyphInst>& getTextGlyphInstanceBuffer() const;

	protected:
		friend Element;
		uint32_t numElements = 0;
		std::unique_ptr<EngineCore::InstanceBuffer<UIInst>> hierarchyInstanceBuffer;
		std::unique_ptr<EngineCore::InstanceBuffer<GlyphInst>> textGlyphInstanceBuffer;
	};

}