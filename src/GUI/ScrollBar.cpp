#include <TweenEngine/Tween.h>
#include "ScrollBar.hpp"
#include "../AssetManager.hpp"

namespace FreeShop {

ScrollBar::ScrollBar()
: m_isTouching(false)
, m_autoHide(true)
, m_needsUpdate(true)
, m_isHidden(true)
, m_isScrolling(false)
, m_deceleration(0.85f)
, m_velocity(0.f)
, m_scrollPos(0.f)
, m_scrollPosMin(0.f)
, m_scrollPosMax(0.f)
, m_scrollTolerance(6.f)
, m_veloctyModifer(2000.f)
{
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/scrollbar.9.png");
	texture.setSmooth(true);
	m_scrollBar.setTexture(&texture);
	m_scrollBar.setContentSize(0.f, 100.f);
	m_scrollBar.setColor(cpp3ds::Color::Red);
}

ScrollBar::~ScrollBar()
{
	for (auto &obj : m_scrollObjects)
		obj->detachScrollbar();
}

void ScrollBar::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	ensureUpdateScrollBar();
	if (m_scrollBar.getSize().y < m_size.y)
	{
		states.transform *= getTransform();
		target.draw(m_scrollBar, states);
	}
}

void ScrollBar::attachObject(Scrollable *object)
{
	if (!object)
		return;
	if (std::find(m_scrollObjects.begin(), m_scrollObjects.end(), object) == m_scrollObjects.end())
	{
		object->attachScrollbar(this);
		m_scrollObjects.push_back(object);
		m_needsUpdate = true;
	}
}

void ScrollBar::detachObject(Scrollable *object)
{
	if (!object)
		return;
	for (auto it = m_scrollObjects.begin(); it != m_scrollObjects.end(); ++it)
		if (*it == object)
		{
			m_scrollObjects.erase(it);
			object->detachScrollbar();
			m_needsUpdate = true;
			break;
		}
}

bool ScrollBar::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::TouchMoved)
	{
		if (m_isTouching)
		{
			m_clockHide.restart();

			if (!m_isScrolling && std::abs(m_startTouchPos.y - event.touch.y) > m_scrollTolerance)
				m_isScrolling = true;

			float posDiff = m_lastTouchPos.y - event.touch.y;
			if (m_isScrolling)
				setScrollRelative(-posDiff);
			m_lastTouchPos.x = event.touch.x;
			m_lastTouchPos.y = event.touch.y;

			cpp3ds::Time timeDiff = m_clockVelocity.restart();
			float v = m_veloctyModifer * -posDiff / (1 + timeDiff.asMilliseconds());
			if ((v >= 0.f && m_velocity >= 0.f) || (v < 0.f && m_velocity < 0.f))
				m_velocity = 0.5f * v + 0.5f * m_velocity;
			else
				m_velocity = v;

			return m_isScrolling;
		}
	}
	else if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (!m_isTouching && m_dragRect.contains(event.touch.x, event.touch.y))
		{
			m_isTouching = true;
			m_isScrolling = false;
			m_startTouchPos = cpp3ds::Vector2i(event.touch.x, event.touch.y);
			m_lastTouchPos = m_startTouchPos;
			m_clockVelocity.restart();
			m_velocity = 0.f;
			return m_isScrolling;
		}
	}
	else if (m_isTouching && event.type == cpp3ds::Event::TouchEnded)
	{
		m_isTouching = false;
		return m_isScrolling;
	}
	return false;
}

void ScrollBar::update(float delta)
{
	if (m_autoHide && m_clockHide.getElapsedTime() > cpp3ds::seconds(3.f))
		hide();

	if (!m_autoHide && m_isHidden)
		show();

	if (m_isScrolling && !m_isTouching)
	{
		if (m_velocity > 0.5f || m_velocity < -0.5f)
		{
			m_velocity *= m_deceleration;
			setScrollRelative(m_velocity * delta);
		}
		else
			m_isScrolling = false;
	}

	if (m_isScrolling)
		for (auto &obj : m_scrollObjects)
			obj->setScroll(m_scrollPos);

	if (m_scrollPos > m_scrollPosMax)
		setScroll(m_scrollPosMax);

	m_tweenManager.update(delta);
}

void ScrollBar::setDragRect(const cpp3ds::IntRect &rect)
{
	m_dragRect = rect;
	m_needsUpdate = true;
}

void ScrollBar::setScrollAreaSize(const cpp3ds::Vector2u &size)
{
	m_scrollAreaSize = size;
	m_needsUpdate = true;
}

void ScrollBar::setSize(const cpp3ds::Vector2u &size)
{
	m_size = size;
	m_needsUpdate = true;
}

void ScrollBar::ensureUpdateScrollBar() const
{
	if (!m_needsUpdate)
		return;

	m_scrollSize = 0.f;
	m_scrollPosMin = m_scrollAreaSize.y;
	m_scrollPosMax = 0.f;
	for (auto &obj : m_scrollObjects)
	{
		float scrollVal = obj->getScroll();
		const cpp3ds::Vector2f &size = obj->getScrollSize();
		m_scrollSize += size.y;
	}
	m_scrollPosMin = m_scrollAreaSize.y - m_scrollSize;

	// If everything is showing, no scrolling to do
	if (m_scrollPosMin > 0)
		m_scrollPosMin = 0;

	float visibleRatio = m_scrollAreaSize.y / (m_scrollSize+1);
	if (visibleRatio < 0.05f)
		visibleRatio = 0.05f;
	m_scrollBar.setSize(0.f, m_size.y * visibleRatio);

	m_needsUpdate = false;
}

void ScrollBar::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case ALPHA: {
			cpp3ds::Color color = m_scrollBar.getColor();
			color.a = newValues[0];
			m_scrollBar.setColor(color);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int ScrollBar::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case ALPHA:
			returnValues[0] = m_scrollBar.getColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void ScrollBar::show()
{
	if (!m_isHidden || m_tweenManager.getRunningTweensCount() > 0)
		return;
	m_clockHide.restart();
	TweenEngine::Tween::to(*this, ALPHA, 0.5f)
		.target(m_color.a)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
			m_isHidden = false;
		})
		.start(m_tweenManager);
}

void ScrollBar::hide()
{
	if (m_isHidden || m_tweenManager.getRunningTweensCount() > 0)
		return;
	TweenEngine::Tween::to(*this, ALPHA, 0.8f)
		.target(0)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
			m_isHidden = true;
		})
		.start(m_tweenManager);
}

void ScrollBar::setColor(const cpp3ds::Color &color)
{
	cpp3ds::Color tmpColor = color;
	if (m_isHidden)
		tmpColor.a = 0;

	m_color = color;
	m_scrollBar.setColor(tmpColor);
}

void ScrollBar::setScroll(float scrollPos)
{
	ensureUpdateScrollBar();
	if (scrollPos > m_scrollPosMax)
		scrollPos = m_scrollPosMax;
	else if (scrollPos < m_scrollPosMin)
		scrollPos = m_scrollPosMin;

	m_scrollPos = scrollPos;
	if (m_autoHide && m_scrollPosMin != m_scrollPosMax)
		show();

	setPosition(m_position);
}

void ScrollBar::setScrollRelative(float scrollDelta)
{
	setScroll(m_scrollPos + scrollDelta);
}

void ScrollBar::setPosition(const cpp3ds::Vector2f &position)
{
	m_position = position;
	float visibleRatio = m_size.y / (m_scrollSize+1);
	Transformable::setPosition(position + cpp3ds::Vector2f(0.f, -m_scrollPos * visibleRatio));
}

void ScrollBar::setPosition(float x, float y)
{
	cpp3ds::Vector2f position(x, y);
	setPosition(position);
}

const cpp3ds::Vector2f &ScrollBar::getPosition() const
{
	return m_position;
}

void ScrollBar::markDirty()
{
	m_needsUpdate = true;
}


} // namespace FreeShop
