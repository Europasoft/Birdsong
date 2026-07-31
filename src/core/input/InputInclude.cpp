#include "core/input/InputInclude.h"
#include "core/input/Input.h"

namespace EngineCore
{
	InputAxisHandle& InputAxisHandle::addKeyBinding(KeyBinding keyBinding)
	{
		axis->keyBindings = { keyBinding };
		return *this;
	}

	InputAxisHandle& InputAxisHandle::addKeyBinding(const std::vector<KeyBinding>& keyBindings)
	{
		axis->keyBindings.insert(axis->keyBindings.end(), keyBindings.begin(), keyBindings.end());
		return *this;
	}

	float InputAxisHandle::getValue() const noexcept
	{
		return axis->value;
	}

	InputEventHandle& InputEventHandle::addKeyBinding(KeyBinding keyBinding)
	{
		InputAxisHandle::addKeyBinding(keyBinding);
		return *this;
	}

	InputEventHandle& InputEventHandle::addKeyBinding(const std::vector<KeyBinding>& keyBindings)
	{
		InputAxisHandle::addKeyBinding(keyBindings);
		return *this;
	}

	bool InputEventHandle::consume()
	{
		if (axis->value == 0) return false;
		axis->value = 0; // reset
		return true;
	}

}