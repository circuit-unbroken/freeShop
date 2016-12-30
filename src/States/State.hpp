#ifndef FREESHOP_STATE_HPP
#define FREESHOP_STATE_HPP

#include "StateIdentifiers.hpp"
#include <cpp3ds/System/Time.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <memory>
#include <vector>
#include <cpp3ds/Graphics/Color.hpp>
#include <cpp3ds/System/String.hpp>


namespace cpp3ds
{
	class Window;
	class TcpSocket;
}

namespace FreeShop {

class StateStack;
class Player;

typedef std::function<bool(void*)> StateCallback;

class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{
		Context(cpp3ds::String& text, std::vector<char*>& data);
		cpp3ds::String& text;
		std::vector<char*>& data;
	};

	State(StateStack& stack, Context& context, StateCallback callback);
	virtual ~State();

	virtual void renderTopScreen(cpp3ds::Window& window) = 0;
	virtual void renderBottomScreen(cpp3ds::Window& window) = 0;
	virtual bool update(float delta) = 0;
	virtual bool processEvent(const cpp3ds::Event& event) = 0;

	void requestStackPush(States::ID stateID, bool renderAlone = false, StateCallback callback = nullptr);
	void requestStackPop();
	void requestStackClear();
	void requestStackClearUnder();
	bool runCallback(void *data);

	Context getContext() const;

private:
	StateStack*  m_stack;
	Context      m_context;
	StateCallback m_callback;
};

} // namespace FreeShop

#endif // FREESHOP_STATE_HPP
