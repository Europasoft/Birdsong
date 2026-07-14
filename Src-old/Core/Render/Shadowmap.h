#pragma once
#include "Core/Render/Attachment.h"

#include <memory>
#include <vector>

namespace EngineCore
{
	class EngineDevice;

	// contains the depth and color attachments for a specific light source,
	// which in turn manage the image resources (images may have multiple copies for multibuffering)
	class Shadowmap 
	{
	public:
		Shadowmap(EngineDevice& device, const AttachmentProperties& depthProps, const AttachmentProperties& colorProps)
			: depthAttachment{ std::make_unique<Attachment>(device, depthProps, false, true) },
			colorAttachment{ std::make_unique<Attachment>(device, colorProps, false, true) } {}

		const Attachment& getDepthAttachment() const { return *depthAttachment.get(); }
		const Attachment& getColorAttachment() const { return *colorAttachment.get(); }

	private:
		std::unique_ptr<Attachment> depthAttachment;
		std::unique_ptr<Attachment> colorAttachment;
	};
}
