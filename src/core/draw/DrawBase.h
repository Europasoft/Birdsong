#pragma once

namespace EngineCore
{
	class EngineDevice;
	struct DrawContext;
	struct FrameContext;

	class DrawBase
	{
	public:
		DrawBase(EngineDevice& device, const DrawContext& d);
		~DrawBase();

		DrawBase(const DrawBase&) = delete;
		DrawBase& operator=(const DrawBase&) = delete;

		virtual void render(const FrameContext& f) = 0;

	protected:
		EngineDevice& device;
		const DrawContext& d;
	};

}