// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "editor/Editor.h"
#include "editor/EditorUI.h"
#include "core/draw/FrameContext.h"
#include "core/asset/Collection.h"
#include "core/asset/glTFLoader.h"
#include "core/engine/Engine.h"

#include <memory>
#include <string_view>

namespace Editor
{
	using namespace EngineCore;
	using namespace AssetSystem;

	Editor::Editor(EngineCore::EngineDevice& device)
	{
		rootAssetCollection = std::make_unique<AssetSystem::Collection>("Root");
		// test
		//rootAssetCollection->nested.push_back(AssetSystem::glTF::load("meshes/gltf-test.glb"));

		if (EngineCore::EngineApplication::enableEditor())
			initEditorUI(device);
	}

	Editor::~Editor() = default;

	void Editor::initEditorUI(EngineCore::EngineDevice& device)
	{
		ui = std::make_unique<EditorUI>(device);
	}

	void Editor::loadUIMaterials(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
		if (ui) ui->loadMaterials(device, d);
	}

	void Editor::renderUI(EngineCore::EngineDevice& device, const EngineCore::FrameContext& f, const EngineCore::DrawContext& d)
	{
		if (ui) ui->render(device, f, d);
	}

	const EngineCore::ViewportState& Editor::getViewportState() const
    {
		assert(ui);
		return ui->getViewportState();
    }

	bool Editor::hasUI() const
	{
		return ui.get();
	}


}