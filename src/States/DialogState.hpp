#ifndef FREESHOP_DIALOGSTATE_HPP
#define FREESHOP_DIALOGSTATE_HPP

#include <cpp3ds/Window/Window.hpp>
#include <TweenEngine/TweenManager.h>
#include "State.hpp"
#include "../TweenObjects.hpp"

namespace FreeShop {

class DialogState: public State {
public:
	enum EventType {
		GetText,
		Response,
	};
	struct Event {
		EventType type;
		void *data;
	};

	DialogState(StateStack& stack, Context& context, StateCallback callback);

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	bool m_isClosing;
	util3ds::TweenRectangleShape m_overlay;
	util3ds::TweenNinePatch m_background;
	util3ds::TweenText m_message;
	TweenEngine::TweenManager m_tweenManager;

	util3ds::TweenRectangleShape m_buttonOkBackground;
	util3ds::TweenRectangleShape m_buttonCancelBackground;
	util3ds::TweenText m_buttonOkText;
	util3ds::TweenText m_buttonCancelText;
};

} // namespace FreeShop

#endif // FREESHOP_DIALOGSTATE_HPP
