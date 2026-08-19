// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/types/CommonTypes.h"

#include <memory>
#include <string_view>

namespace EngineCore
{
	class EngineDevice;
	struct DrawContext;
	struct FrameContext;
	struct ViewportState;
}

namespace AssetSystem
{
	class Collection;
}

namespace Editor
{
	class EditorUI;

	class Editor
	{
	public:
		Editor(EngineCore::EngineDevice& device);
		~Editor();

		void initEditorUI(EngineCore::EngineDevice& device);
		void loadUIMaterials(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);
		void renderUI(EngineCore::EngineDevice& device, const EngineCore::FrameContext& f, const EngineCore::DrawContext& d);
		const EngineCore::ViewportState& getViewportState() const;
		bool hasUI() const;

	protected:
		std::unique_ptr<EditorUI> ui;
		std::unique_ptr<AssetSystem::Collection> rootAssetCollection;
	};

}