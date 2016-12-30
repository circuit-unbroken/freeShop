#ifndef FREESHOP_STATESTACK_HPP
#define FREESHOP_STATESTACK_HPP

#include "State.hpp"
#include "StateIdentifiers.hpp"
#include <cpp3ds/System/NonCopyable.hpp>
#include <cpp3ds/System/Time.hpp>
#include <vector>
#include <utility>
#include <functional>
#include <map>


namespace FreeShop {

class StateStack : private cpp3ds::NonCopyable
{
public:
	enum Action {
		Push,
		Pop,
		Clear,
		ClearUnder,
	};

public:
	explicit StateStack(State::Context context);

	template <typename T>
	void registerState(States::ID stateID);

	void update(float delta);
	void renderTopScreen(cpp3ds::Window& window);
	void renderBottomScreen(cpp3ds::Window& window);
	void processEvent(const cpp3ds::Event& event);

	void pushState(States::ID stateID, bool renderAlone = false, StateCallback callback = nullptr);
	void popState();
	void clearStates();
	void clearStatesUnder();

	bool isEmpty() const;


private:
	State::Ptr createState(States::ID stateID, StateCallback callback);
	void       applyPendingChanges();


private:
	struct PendingChange
	{
		explicit PendingChange(Action action, States::ID stateID = States::None, bool renderAlone = false, StateCallback = nullptr);

		Action     action;
		States::ID stateID;
		bool renderAlone;
		StateCallback callback;
	};

	struct StateStackItem
	{
		States::ID id;
		State::Ptr pointer;
		bool renderAlone;
		bool renderEnabled;
	};

	void updateRenderConfig();

private:
	std::vector<StateStackItem>    m_stack;
	std::vector<PendingChange> m_pendingList;

	State::Context m_context;
	std::map<States::ID, std::function<State::Ptr(StateCallback)>> m_factories;
};


template <typename T>
void StateStack::registerState(States::ID stateID)
{
	m_factories[stateID] = [this] (StateCallback callback)
	{
		return State::Ptr(new T(*this, m_context, callback));
	};
}

} // namespace FreeShop

#endif // FREESHOP_STATESTACK_HPP
