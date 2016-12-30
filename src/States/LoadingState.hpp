#ifndef FREESHOP_LOADINGSTATE_HPP
#define FREESHOP_LOADINGSTATE_HPP

#include "State.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/RectangleShape.hpp>
#include <cpp3ds/Graphics/Text.hpp>

namespace FreeShop {

class LoadingState : public State
{
public:
	LoadingState(StateStack& stack, Context& context, StateCallback callback);

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	cpp3ds::RectangleShape m_background;
	cpp3ds::Text m_icon;
	cpp3ds::Clock m_rotateClock;
};

} // namespace FreeShop

#endif // FREESHOP_LOADINGSTATE_HPP
