#include <sys/stat.h>
#include "FreeShop.hpp"
#include "States/TitleState.hpp"
#include "Notification.hpp"
#include "States/DialogState.hpp"
#include "States/LoadingState.hpp"
#include "States/SyncState.hpp"
#include "States/BrowseState.hpp"
#include "States/NewsState.hpp"
#include "TitleKeys.hpp"
#include "Util.hpp"
#include "States/SleepState.hpp"
#include "States/QrScannerState.hpp"


using namespace cpp3ds;
using namespace TweenEngine;

namespace FreeShop {

cpp3ds::Uint64 g_requestJump = 0;
bool g_requestShutdown = false;

FreeShop::FreeShop()
: Game(0x100000)
{
	m_stateStack = new StateStack(State::Context(m_text, m_data));
	m_stateStack->registerState<TitleState>(States::Title);
	m_stateStack->registerState<LoadingState>(States::Loading);
	m_stateStack->registerState<SyncState>(States::Sync);
	m_stateStack->registerState<BrowseState>(States::Browse);
	m_stateStack->registerState<SleepState>(States::Sleep);
	m_stateStack->registerState<QrScannerState>(States::QrScanner);
	m_stateStack->registerState<DialogState>(States::Dialog);
	m_stateStack->registerState<NewsState>(States::News);

#ifdef EMULATION
	m_stateStack->pushState(States::Browse);
#else
	m_stateStack->pushState(States::Loading);
	m_stateStack->pushState(States::Sync);
	m_stateStack->pushState(States::Title);
#endif

	textFPS.setFillColor(cpp3ds::Color::Red);
	textFPS.setCharacterSize(20);

	// Set up directory structure, if necessary
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR);
	if (!pathExists(path.c_str(), false))
		makeDirectory(path.c_str());

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp");
	if (pathExists(path.c_str(), false))
		removeDirectory(path.c_str());
	mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/cache");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/keys");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	Config::loadFromFile();

	// Load chosen language, correct auto-detect with separate Spanish/Portuguese
	std::string langCode = Config::get(Config::Language).GetString();
#ifdef _3DS
	u8 region;
	CFGU_SecureInfoGetRegion(&region);
	if (langCode == "auto")
	{
		if (cpp3ds::I18n::getLanguage() == cpp3ds::Language::Spanish)
			langCode = (region == CFG_REGION_USA) ? "es_US" : "es_ES";
		else if (cpp3ds::I18n::getLanguage() == cpp3ds::Language::Portuguese)
			langCode = (region == CFG_REGION_USA) ? "pt_BR" : "pt_PT";
	}
#endif
	if (langCode != "auto")
		cpp3ds::I18n::loadLanguageFile(_("lang/%s.lang", langCode.c_str()));

	// If override.lang exists, use it instead of chosen language
	std::string testLangFilename(FREESHOP_DIR "/override.lang");
	if (pathExists(testLangFilename.c_str()))
		cpp3ds::I18n::loadLanguageFile(testLangFilename);
}

FreeShop::~FreeShop()
{
	delete m_stateStack;
	Config::saveToFile();

#ifdef _3DS
	if (g_requestJump != 0)
	{
		Result res = 0;
		u8 hmac[0x20];
		memset(hmac, 0, sizeof(hmac));
		FS_MediaType mediaType = ((g_requestJump >> 32) == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
		if (R_SUCCEEDED(res = APT_PrepareToDoApplicationJump(0, g_requestJump, mediaType)))
			res = APT_DoApplicationJump(0, 0, hmac);
	}
	else if (g_requestShutdown)
	{
		ptmSysmInit();
		PTMSYSM_ShutdownAsync(0);
		ptmSysmExit();
	}
#endif
}

void FreeShop::update(float delta)
{
	// Need to update before checking if empty
	m_stateStack->update(delta);
	if (m_stateStack->isEmpty() || g_requestJump != 0 || g_requestShutdown)
		exit();

	Notification::update(delta);

#ifndef NDEBUG
	static int i;
	if (i++ % 10 == 0) {
		textFPS.setString(_("%.1f fps", 1.f / delta));
		textFPS.setPosition(395 - textFPS.getGlobalBounds().width, 2.f);
	}
#endif
}

void FreeShop::processEvent(Event& event)
{
	m_stateStack->processEvent(event);
}

void FreeShop::renderTopScreen(Window& window)
{
	window.clear(Color::White);
	m_stateStack->renderTopScreen(window);
	for (auto& notification : Notification::notifications)
		window.draw(*notification);

#ifndef NDEBUG
	window.draw(textFPS);
#endif
}

void FreeShop::renderBottomScreen(Window& window)
{
	window.clear(Color::White);
	m_stateStack->renderBottomScreen(window);
}


} // namespace FreeShop
