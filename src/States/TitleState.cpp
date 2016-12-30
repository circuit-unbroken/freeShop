#include "TitleState.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Service.hpp>
#include "../Config.hpp"

using namespace TweenEngine;
using namespace util3ds;

namespace FreeShop {

TitleState::TitleState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
{
	m_textVersion.setString(FREESHOP_VERSION);
	m_textVersion.setCharacterSize(12);
	m_textVersion.setFillColor(cpp3ds::Color::White);
	m_textVersion.setOutlineColor(cpp3ds::Color(0, 0, 0, 100));
	m_textVersion.setOutlineThickness(1.f);
	m_textVersion.setPosition(318.f - m_textVersion.getLocalBounds().width, 222.f);

	m_textFree.setString("fre");
	m_textFree.setFillColor(cpp3ds::Color(255, 255, 255, 0));
	m_textFree.setCharacterSize(75);
	m_textFree.setPosition(23, 64);
	m_textFree.setStyle(cpp3ds::Text::Bold);

	m_textureEshop.loadFromFile("images/eshop.png");
	m_spriteEshop.setTexture(m_textureEshop, true);
	m_spriteEshop.setColor(cpp3ds::Color(0xf4, 0x6c, 0x26, 0x00));
	m_spriteEshop.setPosition(390.f - m_spriteEshop.getLocalBounds().width, 40.f);

	m_textureBag.loadFromFile("images/bag.png");
	m_spriteBag.setTexture(m_textureBag, true);
	m_spriteBag.setColor(cpp3ds::Color(0xf4, 0x6c, 0x26, 0x00));
	m_spriteBag.setPosition(50.f, 50.f);
	m_spriteBag.setScale(0.4f, 0.4f);

	// Tween the bag
	Tween::to(m_spriteBag, TweenSprite::COLOR_ALPHA, 0.5f)
			.target(255.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::POSITION_XY, 0.5f)
			.target(10.f, 30.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::SCALE_XY, 0.5f)
			.target(1.f, 1.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::COLOR_RGB, 1.f)
			.target(0, 0, 0)
			.delay(3.5f)
			.start(m_manager);

	// Tween the "fre" text
	Tween::to(m_textFree, TweenSprite::COLOR_ALPHA, 1.f)
			.target(255.f)
			.delay(3.5f)
			.start(m_manager);

	// Tween the logo after the bag
	Tween::to(m_spriteEshop, TweenSprite::POSITION_Y, 1.f)
			.targetRelative(30.f)
			.delay(1.5f)
			.start(m_manager);
	Tween::to(m_spriteEshop, TweenSprite::COLOR_ALPHA, 1.f)
			.target(255)
			.delay(1.5f)
			.start(m_manager);
	Tween::to(m_spriteEshop, TweenSprite::COLOR_RGB, 1.f)
			.target(0, 0, 0)
			.delay(3.5f)
			.start(m_manager);
}

void TitleState::renderTopScreen(cpp3ds::Window& window)
{
	window.draw(m_spriteEshop);
	window.draw(m_spriteBag);
	window.draw(m_textFree);
}

void TitleState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_textVersion);
}

bool TitleState::update(float delta)
{
	m_manager.update(delta);
	return true;
}

bool TitleState::processEvent(const cpp3ds::Event& event)
{
	return true;
}

} // namespace FreeShop
