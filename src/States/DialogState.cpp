#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cmath>
#include "DialogState.hpp"
#include "../AssetManager.hpp"
#include "SleepState.hpp"
#include "BrowseState.hpp"

namespace FreeShop {

DialogState::DialogState(StateStack &stack, Context &context, StateCallback callback)
: State(stack, context, callback)
, m_isClosing(false)
{
	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color(255, 255, 255, 0));

	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/button-radius.9.png");
	m_background.setTexture(&texture);
	m_background.setSize(cpp3ds::Vector2f(280.f, 200.f));
	m_background.setPosition(20.f, 0.f);
	m_background.setColor(cpp3ds::Color(200, 200, 200, 0));

	m_message.setCharacterSize(14);
	m_message.setFillColor(cpp3ds::Color::Transparent);
	m_message.useSystemFont();
	m_message.setPosition(160.f, 80.f);

	m_buttonOkBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	m_buttonOkBackground.setFillColor(cpp3ds::Color(100, 200, 100, 0));
	m_buttonOkBackground.setPosition(165.f, 160.f);

	m_buttonCancelBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	m_buttonCancelBackground.setFillColor(cpp3ds::Color(180, 180, 180, 0));
	m_buttonCancelBackground.setPosition(50.f, 160.f);

	m_buttonOkText.setString(_("\uE000 Ok"));
	m_buttonOkText.setCharacterSize(14);
	m_buttonOkText.setFillColor(cpp3ds::Color(255, 255, 255, 0));
	m_buttonOkText.setPosition(210.f, 172.f);
	m_buttonOkText.useSystemFont();
	m_buttonOkText.setOrigin(std::round((m_buttonOkText.getLocalBounds().left + m_buttonOkText.getLocalBounds().width) / 2),
							 std::round((m_buttonOkText.getLocalBounds().top + m_buttonOkText.getLocalBounds().height) / 2));

	m_buttonCancelText = m_buttonOkText;
	m_buttonCancelText.setString(_("\uE001 Cancel"));
	m_buttonCancelText.setPosition(95.f, 172.f);
	m_buttonCancelText.setOrigin(std::round((m_buttonCancelText.getLocalBounds().left + m_buttonCancelText.getLocalBounds().width) / 2),
	                             std::round((m_buttonCancelText.getLocalBounds().top + m_buttonCancelText.getLocalBounds().height) / 2));

	cpp3ds::String tmp;
	Event event = {GetText, &tmp};
	if (runCallback(&event)) {
		auto message = reinterpret_cast<cpp3ds::String*>(event.data);
		m_message.setString(*message);
	} else
		m_message.setString(_("Are you sure you want\nto continue?"));
	m_message.setOrigin(std::round(m_message.getLocalBounds().width / 2),
						std::round(m_message.getLocalBounds().height / 2));

#define TWEEN_IN(obj, posY) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.3f).target(255.f).delay(0.2f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.3f).target(posY).delay(0.2f).start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.3f).target(150.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.3f).target(220.f).delay(0.2f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.POSITION_Y, 0.3f).target(20.f).delay(0.2f).start(m_tweenManager);
	TWEEN_IN(m_message, 100.f);
	TWEEN_IN(m_buttonOkBackground, 180.f);
	TWEEN_IN(m_buttonCancelBackground, 180.f);
	TWEEN_IN(m_buttonOkText, 192.f);
	TWEEN_IN(m_buttonCancelText, 192.f);
}

void DialogState::renderTopScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);
}

void DialogState::renderBottomScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);
	window.draw(m_background);
	window.draw(m_message);
	window.draw(m_buttonOkBackground);
	window.draw(m_buttonOkText);
	window.draw(m_buttonCancelBackground);
	window.draw(m_buttonCancelText);
}

bool DialogState::update(float delta)
{
	m_tweenManager.update(delta);
	return false;
}

bool DialogState::processEvent(const cpp3ds::Event &event)
{
	BrowseState::clockDownloadInactivity.restart();
	SleepState::clock.restart();
	if (m_isClosing)
		return false;

	bool triggerResponse = false;
	bool accepted = false;

	if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y))
			triggerResponse = accepted = true;
		else if (m_buttonCancelBackground.getGlobalBounds().contains(event.touch.x, event.touch.y))
			triggerResponse = true;
	}
	else if (event.type == cpp3ds::Event::KeyPressed)
	{
		if (event.key.code == cpp3ds::Keyboard::A)
			triggerResponse = accepted = true;
		else if (event.key.code == cpp3ds::Keyboard::B)
			triggerResponse = true;
	}

#define TWEEN_OUT(obj, posY) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.3f).target(0.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.3f).target(posY).start(m_tweenManager);

	if (triggerResponse)
	{
		Event evt = {Response, &accepted};

		// Highlight selected button
		if (accepted)
			m_buttonOkBackground.setFillColor(cpp3ds::Color(120, 90, 90, m_buttonOkBackground.getFillColor().a));
		else
			m_buttonCancelBackground.setFillColor(cpp3ds::Color(120, 90, 90, m_buttonCancelBackground.getFillColor().a));

		if (runCallback(&evt))
		{
			m_isClosing = true;
			m_tweenManager.killAll();

			TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.3f).target(0.f).start(m_tweenManager);
			TweenEngine::Tween::to(m_background, m_background.POSITION_Y, 0.3f).target(40.f).start(m_tweenManager);
			TWEEN_OUT(m_message, 120.f);
			TWEEN_OUT(m_buttonOkBackground, 200.f);
			TWEEN_OUT(m_buttonCancelBackground, 200.f);
			TWEEN_OUT(m_buttonOkText, 210.f);
			TWEEN_OUT(m_buttonCancelText, 210.f);
			TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.3f).target(0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
					requestStackPop();
				})
				.delay(0.2f).start(m_tweenManager);
		}
	}
	return false;
}


} // namespace FreeShop
