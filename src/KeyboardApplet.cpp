#include "KeyboardApplet.hpp"
#include "TitleKeys.hpp"

namespace {

SwkbdCallbackResult callbackUrl(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	cpp3ds::String strError;
	auto kb = reinterpret_cast<FreeShop::KeyboardApplet*>(user);

	if (FreeShop::TitleKeys::isValidUrl(text, &strError))
		return SWKBD_CALLBACK_OK;

	kb->error = strError.toUtf8();
	*ppMessage = reinterpret_cast<const char*>(kb->error.c_str());

	return SWKBD_CALLBACK_CONTINUE;
}

}


namespace FreeShop {

KeyboardApplet::KeyboardApplet(InputType type)
	: m_type(type)
	, m_button(SWKBD_BUTTON_NONE)
	, m_swkbdLearning(nullptr)
{
	switch (type)
	{
		case URL:
			swkbdInit(&m_swkbdState, SWKBD_TYPE_NORMAL, 2, -1);
			swkbdSetHintText(&m_swkbdState, "https://");
			swkbdSetValidation(&m_swkbdState, SWKBD_NOTBLANK_NOTEMPTY, 0, 0);
			swkbdSetFilterCallback(&m_swkbdState, callbackUrl, this);
			swkbdSetFeatures(&m_swkbdState, SWKBD_PREDICTIVE_INPUT);
			break;

		case Text:
			// Fall through
		default:
			swkbdInit(&m_swkbdState, SWKBD_TYPE_NORMAL, 2, -1);
			swkbdSetValidation(&m_swkbdState, SWKBD_ANYTHING, 0, 0);
	}
//	swkbdSetPasswordMode(&m_swkbdState, SWKBD_PASSWORD_HIDE_DELAY);
//	swkbdSetFeatures(&m_swkbdState, SWKBD_FIXED_WIDTH);
//	swkbdSetNumpadKeys(&m_swkbdState, L'ツ', L'益');
}

KeyboardApplet::~KeyboardApplet()
{
	delete m_swkbdLearning;
}

KeyboardApplet::operator SwkbdState*()
{
	return &m_swkbdState;
}

cpp3ds::String KeyboardApplet::getInput()
{
	if (!m_dictWords.empty())
		swkbdSetDictionary(&m_swkbdState, &m_dictWords[0], m_dictWords.size());

	char inputBuf[255];
	m_button = swkbdInputText(&m_swkbdState, inputBuf, sizeof(inputBuf));
	if (m_button == SWKBD_BUTTON_CONFIRM)
	{
		std::string str(inputBuf);
		return cpp3ds::String::fromUtf8(str.begin(), str.end());
	}
	else
		return cpp3ds::String();
}

void KeyboardApplet::addDictWord(const std::string &typedWord, const std::string &resultWord)
{
	SwkbdDictWord dictWord;
	swkbdSetDictWord(&dictWord, typedWord.c_str(), resultWord.c_str());
	m_dictWords.push_back(dictWord);
}


} // namespace FreeShop
