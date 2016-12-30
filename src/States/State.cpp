#include "State.hpp"
#include "StateStack.hpp"


namespace FreeShop {

State::Context::Context(cpp3ds::String& text, std::vector<char*>& data)
: text(text)
, data(data)
{
	//
}

State::State(StateStack& stack, Context& context, StateCallback callback)
: m_stack(&stack)
, m_context(context)
, m_callback(callback)
{
}

State::~State()
{
}

void State::requestStackPush(States::ID stateID, bool renderAlone, StateCallback callback)
{
	m_stack->pushState(stateID, renderAlone, callback);
}

void State::requestStackPop()
{
	m_stack->popState();
}

void State::requestStackClear()
{
	m_stack->clearStates();
}

void State::requestStackClearUnder()
{
	m_stack->clearStatesUnder();
}

bool State::runCallback(void *data)
{
	if (m_callback)
		return m_callback(data);
	return false;
}

State::Context State::getContext() const
{
	return m_context;
}

} // namespace FreeShop
