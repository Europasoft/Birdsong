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
	}
}

namespace UI
{
	class RootElement;
	using UIInst = EngineCore::ShaderInstanceData::UIElementInstanceData;



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
		std::shared_ptr<EngineCore::Material> material;
		std::vector<std::unique_ptr<Element>> nested;
		Element* parent = nullptr;
		RootElement* root = nullptr;
		uint32_t instanceDataIndex = 0;

	public:
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

		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);

		bool isRoot() const;
		bool isNextToRoot() const;
		bool isLeaf() const;

	protected:
		friend class RootElement;

		template <typename T, typename... Args> requires std::derived_from<T, Element>
		static std::unique_ptr<T> create(Args&&... args)
		{
			return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
		}
		virtual void postAddElement(Element& e);
		virtual void preDrawRecursive();
		virtual void preDraw();
		virtual void drawRecursive(const EngineCore::FrameContext& f, EngineCore::Material*& m);
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m);
		Vec2 calculatePosition() const;
	};

	class RootElement : public Element
	{
	protected:
		RootElement(EngineCore::EngineDevice& device);

	public:
		~RootElement();
		static std::unique_ptr<RootElement> create(EngineCore::EngineDevice& device);

		void drawAll(const EngineCore::FrameContext& f);

	protected:
		friend Element;
		std::unique_ptr<EngineCore::InstanceBuffer<UIInst>> hierarchyInstanceBuffer;
		uint32_t numElements = 0;

	private:
		// these are hidden - not useful on root element
		void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override {};
		void drawRecursive(const EngineCore::FrameContext& f, EngineCore::Material*& m) override {};
		void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override {};
	};


	class VerticalBox : public Element
	{
	public:
		VerticalBox() = default;
	};

}