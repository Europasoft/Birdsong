#pragma once

namespace EngineCore
{

	// timing for the engine render loop
	class EngineClock
	{
		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;
		const double deltaMax = 0.2;

	public:
		EngineClock() : start{ Clock::now() } {};

		void measureFrameDelta(const uint32_t& currentframeIndex)
		{
			if (currentframeIndex != lastFrameIndex) /* new frame started? */
			{
				if (lastFrameIndex != 9999) /* update delta (except on very first frame) */
				{
					std::chrono::duration<double, std::milli> ms = Clock::now() - frameDeltaStart;
					frameDelta = std::min(deltaMax, (ms.count() / 1000.0));
				}
				// reset timer
				frameDeltaStart = Clock::now();
				lastFrameIndex = currentframeIndex;
			}
		}
		const double& getDelta() const { return frameDelta; }
		uint32_t getFps() const { return static_cast<uint32_t>(1 / frameDelta); }
		double getElapsed() const
		{
			std::chrono::duration<double, std::milli> ms = Clock::now() - start;
			return ms.count() / 1000.0;
		}

	private:
		TimePoint start;
		TimePoint frameDeltaStart;
		double frameDelta = 0.01;
		uint32_t lastFrameIndex = 9999;
	};
}

