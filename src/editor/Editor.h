// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once

#include <memory>
#include <string_view>

namespace EngineCore
{
	class EngineDevice;
	struct DrawContext;
	struct FrameContext;
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
		Editor();
		~Editor();

		void initEditorUI(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);
		void renderUI(EngineCore::EngineDevice& device, const EngineCore::FrameContext& f, const EngineCore::DrawContext& d);

	protected:
		std::unique_ptr<EditorUI> ui;
		std::unique_ptr<AssetSystem::Collection> rootAssetCollection;
	};

}