#ifndef FREESHOP_SCROLLBAR_HPP
#define FREESHOP_SCROLLBAR_HPP

#include <vector>
#include <cpp3ds/Graphics/Rect.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/System/Clock.hpp>
#include "Scrollable.hpp"
#include "NinePatch.hpp"
#include "../TweenObjects.hpp"

namespace FreeShop {

class ScrollBar : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int ALPHA = 11; // for tweening

	ScrollBar();
	~ScrollBar();
	void attachObject(Scrollable *object);
	void detachObject(Scrollable *object);

	void setScroll(float scrollPos);
	void setScrollRelative(float scrollDelta);

	void setPosition(const cpp3ds::Vector2f& position);
	void setPosition(float x, float y);
	const cpp3ds::Vector2f& getPosition() const;

	void setDragRect(const cpp3ds::IntRect &rect);
	void setSize(const cpp3ds::Vector2u &size);
	void setScrollAreaSize(const cpp3ds::Vector2u &size);
	void setDeceleration(float rate) { m_deceleration = rate; }
	void setColor(const cpp3ds::Color &color);
	void setAutoHide(bool autoHide) { m_autoHide = autoHide; }

	void show();
	void hide();
	void markDirty();

	// Returns true when event is captured
	bool processEvent(const cpp3ds::Event &event);
	void update(float delta);

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);
	void ensureUpdateScrollBar() const;

private:
	bool m_autoHide;
	bool m_isHidden;
	bool m_isTouching;
	bool m_isScrolling;
	float m_scrollPos;
	mutable float m_scrollPosMin;
	mutable float m_scrollPosMax;
	mutable float m_scrollSize;
	float m_scrollTolerance; // Value to move before scrolling starts
	cpp3ds::Vector2i m_lastTouchPos;

	cpp3ds::Clock m_clockHide;
	cpp3ds::Color m_color;

	mutable bool m_needsUpdate;
	mutable std::vector<Scrollable*> m_scrollObjects;
	cpp3ds::IntRect m_dragRect;
	cpp3ds::Vector2u m_scrollAreaSize;
	cpp3ds::Vector2u m_size;
	cpp3ds::Vector2f m_position;

	gui3ds::NinePatch m_scrollBar;
	TweenEngine::TweenManager m_tweenManager;

	// Kinetic scrolling
	float m_deceleration;   // Rate at which the velocity slows to a stop (1.f to never end, 0.f for instant stop)
	float m_velocity;       // Store current velocity
	float m_veloctyModifer; // For fine tuning velocity
	cpp3ds::Clock m_clockVelocity;
	cpp3ds::Vector2i m_startTouchPos;
};

} // namespace FreeShop


#endif // FREESHOP_SCROLLBAR_HPP
