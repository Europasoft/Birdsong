#pragma once

#include <memory>
#include <vector>

namespace EngineCore
{
	class InputSystem;
	class InputAxis;
	class InputEvent;

	class KeyBinding
	{
	public:
		KeyBinding(uint32_t key, float influence = 1) : key(key), influence(influence)
		{};

	private:
		friend InputSystem;
		friend InputAxis;
		friend InputEvent;

		int32_t key = -2;
		float influence;
	};

	class InputAxisHandle
	{
	public:
		InputAxisHandle() : axis(nullptr) {};
	protected:
		friend InputSystem;
		InputAxisHandle(std::shared_ptr<InputAxis> axis) : axis(axis) {};

	public:
		float getValue() const noexcept;
		InputAxisHandle& addKeyBinding(KeyBinding keyBinding);
		InputAxisHandle& addKeyBinding(const std::vector<KeyBinding>& keyBindings);
		operator float() const noexcept
		{
			return getValue();
		}

	protected:
		std::shared_ptr<InputAxis> axis;
	};

	enum class EInputEvent : uint32_t { COMBO, ANY };

	class InputEventHandle : public InputAxisHandle
	{
	public:
		using InputAxisHandle::InputAxisHandle;
		float getValue() = delete;
		InputEventHandle& addKeyBinding(KeyBinding keyBinding);
		InputEventHandle& addKeyBinding(const std::vector<KeyBinding>& keyBindings);


		bool consume();
	};


}