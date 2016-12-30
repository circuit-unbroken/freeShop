#ifndef FREESHOP_NEWSSTATE_HPP
#define FREESHOP_NEWSSTATE_HPP

#include <cpp3ds/Window/Window.hpp>
#include <TweenEngine/TweenManager.h>
#include "State.hpp"
#include "../TweenObjects.hpp"
#include "../GUI/Scrollable.hpp"
#include "../GUI/ScrollBar.hpp"

namespace FreeShop {

class NewsState: public State, public Scrollable {
public:
	NewsState(StateStack& stack, Context& context, StateCallback callback);

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

	virtual void setScroll(float position);
	virtual float getScroll();
	virtual const cpp3ds::Vector2f &getScrollSize();

private:
	bool m_skippedFirstUpdate;
	bool m_isClosing;
	bool m_finishedFadeIn;
	util3ds::TweenRectangleShape m_overlay;
	util3ds::TweenNinePatch m_background;
	util3ds::TweenText m_title;
	util3ds::TweenText m_message;
	TweenEngine::TweenManager m_tweenManager;

	util3ds::TweenRectangleShape m_buttonOkBackground;
	util3ds::TweenText m_buttonOkText;
	cpp3ds::Font m_font;

	ScrollBar m_scrollbar;
	cpp3ds::Vector2f m_scrollSize;
	float m_scrollPos;
};

} // namespace FreeShop

#endif // FREESHOP_NEWSSTATE_HPP
