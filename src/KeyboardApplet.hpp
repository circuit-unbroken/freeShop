#ifndef FREESHOP_KEYBOARDAPPLET_HPP
#define FREESHOP_KEYBOARDAPPLET_HPP

#include <3ds.h>
#include <cpp3ds/System/String.hpp>

namespace FreeShop {

class KeyboardApplet {
public:
	enum InputType {
		Text,
		Number,
		URL,
		TitleID,
	};

	KeyboardApplet(InputType type = Text);
	~KeyboardApplet();
	operator SwkbdState*();

	void addDictWord(const std::string &typedWord, const std::string &resultWord);

	cpp3ds::String getInput();
	SwkbdButton getButton() { return m_button; }

	std::basic_string<cpp3ds::Uint8> error;

private:
	InputType m_type;

	SwkbdState        m_swkbdState;
	SwkbdStatusData   m_swkbdStatus;
	SwkbdLearningData *m_swkbdLearning;
	SwkbdButton       m_button;
	std::vector<SwkbdDictWord> m_dictWords;
};

} // namespace FreeShop

#endif //FREESHOP_KEYBOARDAPPLET_HPP
