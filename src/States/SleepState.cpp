#include "SleepState.hpp"
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

bool SleepState::isSleeping = false;
cpp3ds::Clock SleepState::clock;

SleepState::SleepState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_sleepEnding(false)
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_TOP);
		gspLcdExit();
	}
#endif
	isSleeping = true;

	m_overlay.setSize(cpp3ds::Vector2f(320.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color::Transparent);

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.5f)
		.target(200.f)
		.start(m_tweenManager);
}

SleepState::~SleepState()
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_TOP);
		gspLcdExit();
	}
#endif
	isSleeping = false;
}

void SleepState::renderTopScreen(cpp3ds::Window& window)
{
}

void SleepState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_overlay);
}

bool SleepState::update(float delta)
{
	if (!isSleeping)
	{
		requestStackPop();
		clock.restart();
	}
	m_tweenManager.update(delta);
	return true;
}

bool SleepState::processEvent(const cpp3ds::Event& event)
{
	if (m_sleepEnding)
		return false;

	m_sleepEnding = true;

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.5f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [&](TweenEngine::BaseTween* source) {
			requestStackPop();
			clock.restart();
		})
		.start(m_tweenManager);

	return false;
}

} // namespace FreeShop
