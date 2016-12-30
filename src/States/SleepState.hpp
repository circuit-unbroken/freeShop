#ifndef FREESHOP_SLEEPSTATE_HPP
#define FREESHOP_SLEEPSTATE_HPP

#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <TweenEngine/TweenManager.h>
#include "State.hpp"
#include "../TweenObjects.hpp"

namespace FreeShop {

class SleepState : public State
{
public:
	SleepState(StateStack& stack, Context& context, StateCallback callback);
	~SleepState();

	static bool isSleeping;
	static cpp3ds::Clock clock;

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	util3ds::TweenRectangleShape m_overlay;
	TweenEngine::TweenManager m_tweenManager;
	bool m_sleepEnding;
};

} // namespace FreeShop

#endif // FREESHOP_SLEEPSTATE_HPP
