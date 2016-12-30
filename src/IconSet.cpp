#include <cpp3ds/System/Err.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "AssetManager.hpp"
#include "IconSet.hpp"
#include <sstream>
#include <TweenEngine/Tween.h>


namespace FreeShop
{

IconSet::IconSet()
: m_selectedIndex(-1)
, m_padding(6.f)
{

}

IconSet::~IconSet()
{

}

void IconSet::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& icon : m_icons)
	{
		target.draw(icon, states);
	}
}

bool IconSet::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::TouchBegan)
	{
		cpp3ds::FloatRect bounds;
		cpp3ds::Vector2f point(event.touch.x, event.touch.y);
		point -= getPosition();

		int i = 0;
		for (auto& icon : m_icons)
		{
			bounds = icon.getGlobalBounds();
			bounds.top -= m_padding / 2.f;
			bounds.left -= m_padding / 2.f;
			bounds.width += m_padding;
			bounds.height += m_padding;

			if (bounds.contains(point))
			{
				setSelectedIndex(i);
			}
			i++;
		}
	}
	return true;
}

void IconSet::update(float delta)
{
	m_tweenManager.update(delta);
}

void IconSet::addIcon(const cpp3ds::String &string)
{
	util3ds::TweenText text;
	text.setString(string);
	text.setCharacterSize(20);
	text.setFillColor(cpp3ds::Color(100, 100, 100));
	text.setOutlineColor(cpp3ds::Color::White);
	text.setOutlineThickness(2.f);
	text.setScale(0.8f, 0.8f);
	text.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));

	cpp3ds::FloatRect textRect = text.getLocalBounds();
	text.setOrigin(textRect.left + textRect.width / 2.f, textRect.top + textRect.height / 2.f);

	m_icons.push_back(text);
	resize();
}

void IconSet::setSelectedIndex(int index)
{
	if (index == m_selectedIndex)
		return;
	if (index < 0 || index >= m_icons.size())
		index = -1;

	for (auto& icon : m_icons)
	{
		TweenEngine::Tween::to(icon, SCALE_XY, 0.3f)
			.target(0.8f, 0.8f)
			.start(m_tweenManager);
		TweenEngine::Tween::to(icon, util3ds::TweenText::FILL_COLOR_RGB, 0.2f)
			.target(100.f, 100.f, 100.f)
			.start(m_tweenManager);
		TweenEngine::Tween::to(icon, util3ds::TweenText::OUTLINE_COLOR_RGB, 0.2f)
			.target(255.f, 255.f, 255.f)
			.start(m_tweenManager);
	}

	TweenEngine::Tween::to(m_icons[index], SCALE_XY, 0.3f)
		.target(1.f, 1.f)
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_icons[index], util3ds::TweenText::FILL_COLOR_RGB, 0.2f)
		.target(0.f, 0.f, 0.f)
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_icons[index], util3ds::TweenText::OUTLINE_COLOR_RGB, 0.3f)
		.target(200.f, 200.f, 200.f)
		.start(m_tweenManager);

	m_selectedIndex = index;
}

int IconSet::getSelectedIndex() const
{
	return m_selectedIndex;
}

void IconSet::resize()
{
	float posX = 0;
	for (auto& icon : m_icons)
	{
		icon.setPosition(posX, 0);
		posX += icon.getLocalBounds().width + m_padding;
	}
}

} // namespace FreeShop
