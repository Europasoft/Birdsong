#pragma once

#include <memory>
#include <vector>

namespace EngineCore
{
	class InputSystem;
	class InputAxis;

	class KeyBinding
	{
	public:
		KeyBinding(uint32_t key, float influence = 1) : key(key), influence(influence)
		{};

	private:
		friend InputSystem;

		int32_t key = -2;
		float influence;
	};

	class InputAxisHandle
	{
	public:
		InputAxisHandle() : axis(nullptr)
		{};
	private:
		friend InputSystem;
		InputAxisHandle(std::shared_ptr<InputAxis> axis) : axis(axis)
		{};

	public:
		InputAxisHandle& addKeyBinding(KeyBinding keyBinding);
		InputAxisHandle& addKeyBinding(const std::vector<KeyBinding>& keyBindings);
		float getValue() const noexcept;
		operator float() const noexcept
		{
			return getValue();
		}

	private:
		std::shared_ptr<InputAxis> axis;
	};


}