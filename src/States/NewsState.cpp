#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cmath>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include "NewsState.hpp"
#include "../AssetManager.hpp"
#include "SleepState.hpp"
#include "BrowseState.hpp"

namespace FreeShop {

NewsState::NewsState(StateStack &stack, Context &context, StateCallback callback)
: State(stack, context, callback)
, m_isClosing(false)
, m_finishedFadeIn(false)
, m_skippedFirstUpdate(false)
{
	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color(255, 255, 255, 0));

	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/button-radius.9.png");
	m_background.setTexture(&texture);
	m_background.setSize(cpp3ds::Vector2f(280.f, 200.f));
	m_background.setPosition(20.f, 0.f);
	m_background.setColor(cpp3ds::Color(200, 200, 200, 0));

	m_message.setCharacterSize(12);
	m_message.setFillColor(cpp3ds::Color::Transparent);
	m_message.setPosition(35.f, 40.f);

	m_font.loadFromFile("fonts/grandhotel.ttf");

	m_title.setFont(m_font);
	m_title.setString(_("FreeShop %s", FREESHOP_VERSION));
	m_title.setCharacterSize(40);
	m_title.setFillColor(cpp3ds::Color(100, 100, 100, 0));
	m_title.setOutlineColor(cpp3ds::Color(200, 200, 200, 0));
	m_title.setOutlineThickness(3.f);
	m_title.setPosition(15.f, -50.f);

	m_buttonOkBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	m_buttonOkBackground.setFillColor(cpp3ds::Color(100, 200, 100, 0));
	m_buttonOkBackground.setPosition(165.f, 167.f);

	m_buttonOkText.setString(_("\uE000 Close"));
	m_buttonOkText.setCharacterSize(14);
	m_buttonOkText.setFillColor(cpp3ds::Color(255, 255, 255, 0));
	m_buttonOkText.setPosition(215.f, 179.f);
	m_buttonOkText.useSystemFont();
	m_buttonOkText.setOrigin(std::round((m_buttonOkText.getLocalBounds().left + m_buttonOkText.getLocalBounds().width) / 2),
							 std::round((m_buttonOkText.getLocalBounds().top + m_buttonOkText.getLocalBounds().height) / 2));

	cpp3ds::FileInputStream file;
	if (file.open(FREESHOP_DIR "/news.txt"))
	{
		std::string text;
		text.resize(file.getSize());
		file.read(&text[0], text.size());
		m_message.setString(text);
	}

	m_scrollbar.setSize(cpp3ds::Vector2u(2, 196));
	m_scrollbar.setScrollAreaSize(cpp3ds::Vector2u(320, 120));
	m_scrollbar.setDragRect(cpp3ds::IntRect(0, 0, 320, 240));
	m_scrollbar.setColor(cpp3ds::Color(0, 0, 0, 40));
	m_scrollbar.setPosition(292.f, 22.f);
	m_scrollbar.setAutoHide(false);
	m_scrollbar.attachObject(this);

	m_scrollSize = cpp3ds::Vector2f(320.f, m_message.getLocalBounds().top + m_message.getLocalBounds().height);

#define TWEEN_IN(obj, posY) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.6f).target(255.f).delay(0.5f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.6f).target(255.f).delay(0.5f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.6f).target(posY).delay(0.5f).start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.6f).target(150.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.6f).target(220.f).delay(0.5f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.POSITION_Y, 0.6f).target(20.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
			m_finishedFadeIn = true;
		}).delay(0.5f).start(m_tweenManager);
	TWEEN_IN(m_title, 5.f);
	TWEEN_IN(m_message, 60.f);
	TWEEN_IN(m_buttonOkBackground, 187.f);
	TWEEN_IN(m_buttonOkText, 199.f);
}

void NewsState::renderTopScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);
}

void NewsState::renderBottomScreen(cpp3ds::Window &window)
{
	cpp3ds::UintRect scissor(0, 30 + m_background.getPosition().y, 320, 130);
	window.draw(m_overlay);
	window.draw(m_background);
	window.draw(m_message, scissor);
	window.draw(m_title);
	window.draw(m_buttonOkBackground);
	window.draw(m_buttonOkText);
	if (m_finishedFadeIn && !m_isClosing)
		window.draw(m_scrollbar);
}

bool NewsState::update(float delta)
{
	if (m_message.getString().isEmpty())
		requestStackPop();

	if (!m_skippedFirstUpdate)
	{
		m_skippedFirstUpdate = true;
		return true;
	}

	m_scrollbar.update(delta);
	m_tweenManager.update(delta);
	return true;
}

bool NewsState::processEvent(const cpp3ds::Event &event)
{
	BrowseState::clockDownloadInactivity.restart();
	SleepState::clock.restart();
	if (m_isClosing)
		return false;

	bool close = false;
	m_scrollbar.processEvent(event);

	if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y))
			close = true;
	}
	else if (event.type == cpp3ds::Event::KeyPressed)
	{
		if (event.key.code == cpp3ds::Keyboard::A)
			close = true;
	}

#define TWEEN_OUT(obj) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.3f).target(0.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.3f).target(0.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.3f).targetRelative(20.f).start(m_tweenManager);

	if (close)
	{
		// Highlight selected button
		m_buttonOkBackground.setFillColor(cpp3ds::Color(120, 90, 90, m_buttonOkBackground.getFillColor().a));

		m_isClosing = true;
		m_tweenManager.killAll();

		TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.3f).target(0.f).start(m_tweenManager);
		TweenEngine::Tween::to(m_background, m_background.POSITION_Y, 0.3f).target(40.f).start(m_tweenManager);
		TWEEN_OUT(m_title);
		TWEEN_OUT(m_message);
		TWEEN_OUT(m_buttonOkBackground);
		TWEEN_OUT(m_buttonOkText);
		TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.3f).target(0.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
				requestStackPop();
			})
			.delay(0.2f).start(m_tweenManager);
	}
	return false;
}

void NewsState::setScroll(float position)
{
	m_scrollPos = position;
	m_message.setPosition(35.f, std::round(60.f + position));
}

float NewsState::getScroll()
{
	return m_scrollPos;
}

const cpp3ds::Vector2f &NewsState::getScrollSize()
{
	return m_scrollSize;
}


} // namespace FreeShop
