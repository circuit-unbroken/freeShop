#include "BrowseState.hpp"
#include "SyncState.hpp"
#include "../Notification.hpp"
#include "../AssetManager.hpp"
#include "../Util.hpp"
#include "../Installer.hpp"
#include "SleepState.hpp"
#include "../InstalledList.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"
#include "../FreeShop.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <sstream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>

#define SECONDS_TO_SLEEP 60.f


namespace FreeShop {

BrowseState *g_browseState = nullptr;
cpp3ds::Clock BrowseState::clockDownloadInactivity;

BrowseState::BrowseState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_appListPositionX(0.f)
, m_threadInitialize(&BrowseState::initialize, this)
, m_threadLoadApp(&BrowseState::loadApp, this)
, m_threadMusic(&BrowseState::playMusic, this)
, m_threadBusy(false)
, m_activeDownloadCount(0)
, m_mode(Downloads)
, m_gwenRenderer(nullptr)
, m_gwenSkin(nullptr)
, m_settingsGUI(nullptr)
, m_isTransitioning(false)
{
	g_browseState = this;
#ifdef EMULATION
	g_syncComplete = true;
	initialize();
#else
	m_threadInitialize.setRelativePriority(1);
	m_threadInitialize.launch();
	m_threadMusic.setRelativePriority(-1);
	m_threadMusic.setAffinity(-1);
	m_threadMusic.launch();
#endif
}

BrowseState::~BrowseState()
{
	if (m_settingsGUI)
		delete m_settingsGUI;
	if (m_gwenSkin)
		delete m_gwenSkin;
	if (m_gwenRenderer)
		delete m_gwenRenderer;
}

void BrowseState::initialize()
{
	// Initialize AppList singleton first
	AppList::getInstance().refresh();
	InstalledList::getInstance().refresh();

	m_iconSet.addIcon(L"\uf290");
	m_iconSet.addIcon(L"\uf019");
	m_iconSet.addIcon(L"\uf11b");
	m_iconSet.addIcon(L"\uf013");
	m_iconSet.addIcon(L"\uf002");
	m_iconSet.setPosition(180.f, 15.f);

	m_textActiveDownloads.setCharacterSize(8);
	m_textActiveDownloads.setFillColor(cpp3ds::Color::Black);
	m_textActiveDownloads.setOutlineColor(cpp3ds::Color::White);
	m_textActiveDownloads.setOutlineThickness(1.f);
	m_textActiveDownloads.setPosition(218.f, 3.f);

	if (TitleKeys::getIds().empty())
		m_textListEmpty.setString(_("No title keys found.\nMake sure you have keys in\n%s\n\nManually copy keys to the directory\nor check settings to enter a URL\nfor downloading title keys.", FREESHOP_DIR "/keys/"));
	else
		m_textListEmpty.setString(_("No cache entries found\nfor your title keys.\n\nTry refreshing cache in settings.\nIf that doesn't work, then your\ntitles simply won't work with\nfreeShop currently."));
	m_textListEmpty.useSystemFont();
	m_textListEmpty.setCharacterSize(16);
	m_textListEmpty.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textListEmpty.setPosition(200.f, 120.f);
	m_textListEmpty.setOrigin(m_textListEmpty.getLocalBounds().width / 2, m_textListEmpty.getLocalBounds().height / 2);

	m_whiteScreen.setPosition(0.f, 30.f);
	m_whiteScreen.setSize(cpp3ds::Vector2f(320.f, 210.f));
	m_whiteScreen.setFillColor(cpp3ds::Color::White);

	m_keyboard.loadFromFile("kb/keyboard.xml");

	m_textMatches.resize(4);
	for (auto& text : m_textMatches)
	{
		text.setCharacterSize(13);
		text.useSystemFont();
	}

	m_musicIntro.openFromFile(FREESHOP_DIR "/shop-intro.ogg");
	m_musicLoop.openFromFile(FREESHOP_DIR "/shop-loop.ogg");
	m_musicLoop.setLoop(true);

	m_scrollbarInstalledList.setPosition(314.f, 30.f);
	m_scrollbarInstalledList.setDragRect(cpp3ds::IntRect(0, 30, 320, 210));
	m_scrollbarInstalledList.setScrollAreaSize(cpp3ds::Vector2u(320, 210));
	m_scrollbarInstalledList.setSize(cpp3ds::Vector2u(8, 210));
	m_scrollbarInstalledList.setColor(cpp3ds::Color(150, 150, 150, 150));
	m_scrollbarDownloadQueue = m_scrollbarInstalledList;

	m_scrollbarInstalledList.attachObject(&InstalledList::getInstance());
	m_scrollbarDownloadQueue.attachObject(&DownloadQueue::getInstance());

	setMode(App);

#ifdef _3DS
	while (!m_gwenRenderer)
		cpp3ds::sleep(cpp3ds::milliseconds(10));
	m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);
	m_gwenSkin->Init("images/DefaultSkin.png");
	m_gwenSkin->SetDefaultFont(L"", 11);
	m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif

	g_browserLoaded = true;

	SleepState::clock.restart();
	clockDownloadInactivity.restart();
	requestStackClearUnder();
}

void BrowseState::renderTopScreen(cpp3ds::Window& window)
{
	if (!g_syncComplete || !g_browserLoaded)
		return;

	if (AppList::getInstance().getList().size() == 0)
		window.draw(m_textListEmpty);
	else
		window.draw(AppList::getInstance());

	// Special draw method to draw top screenshot if selected
	m_appInfo.drawTop(window);
}

void BrowseState::renderBottomScreen(cpp3ds::Window& window)
{
	if (!m_gwenRenderer)
	{
		m_gwenRenderer = new Gwen::Renderer::cpp3dsRenderer(window);

#ifdef EMULATION
		m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);
		m_gwenSkin->Init("images/DefaultSkin.png");
		m_gwenSkin->SetDefaultFont(L"", 11);
		m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif
	}
	if (!g_syncComplete || !g_browserLoaded)
		return;

	if (m_mode == Search)
	{
		window.draw(m_keyboard);
		for (auto& textMatch : m_textMatches)
			window.draw(textMatch);
	}
	if (m_mode == Settings)
	{
		m_settingsGUI->render();
	}

	window.draw(m_iconSet);

	if (m_activeDownloadCount > 0)
		window.draw(m_textActiveDownloads);

	if (m_mode == App)
		window.draw(m_appInfo);
	if (m_mode == Downloads)
	{
		window.draw(DownloadQueue::getInstance());
		window.draw(m_scrollbarDownloadQueue);
	}
	if (m_mode == Installed)
	{
		window.draw(InstalledList::getInstance());
		window.draw(m_scrollbarInstalledList);
	}

	if (m_isTransitioning)
		window.draw(m_whiteScreen);
}

bool BrowseState::update(float delta)
{
	if (!g_syncComplete || !g_browserLoaded)
		return true;
	if (m_threadBusy)
	{
		clockDownloadInactivity.restart();
		SleepState::clock.restart();
	}

	// Show latest news if requested
	if (Config::get(Config::ShowNews).GetBool())
	{
		Config::set(Config::ShowNews, false);
		requestStackPush(States::News);
	}

	// Go into sleep state after inactivity
	if (!SleepState::isSleeping && Config::get(Config::SleepMode).GetBool() && SleepState::clock.getElapsedTime() > cpp3ds::seconds(SECONDS_TO_SLEEP))
	{
		requestStackPush(States::Sleep);
		return false;
	}

	// Power off after sufficient download inactivity
	if (m_activeDownloadCount == 0
		&& Config::get(Config::PowerOffAfterDownload).GetBool()
		&& clockDownloadInactivity.getElapsedTime() > cpp3ds::seconds(Config::get(Config::PowerOffTime).GetInt()))
	{
		g_requestShutdown = true;
		return false;
	}

	// If selected icon changed, change mode accordingly
	int iconIndex = m_iconSet.getSelectedIndex();
	if (m_mode != iconIndex && iconIndex >= 0)
		setMode(static_cast<Mode>(iconIndex));

	// Update the active mode
	if (m_mode == App)
	{
		m_appInfo.update(delta);
	}
	else if (m_mode == Downloads)
	{
		DownloadQueue::getInstance().update(delta);
		m_scrollbarDownloadQueue.update(delta);
	}
	else if (m_mode == Installed)
	{
		InstalledList::getInstance().update(delta);
		m_scrollbarInstalledList.update(delta);
	}
	else if (m_mode == Search)
	{
		m_keyboard.update(delta);
	}

	if (m_activeDownloadCount != DownloadQueue::getInstance().getActiveCount())
	{
		clockDownloadInactivity.restart();
		m_activeDownloadCount = DownloadQueue::getInstance().getActiveCount();
		m_textActiveDownloads.setString(_("%u", m_activeDownloadCount));
	}

	m_iconSet.update(delta);
	AppList::getInstance().update(delta);
	m_tweenManager.update(delta);
	return true;
}

bool BrowseState::processEvent(const cpp3ds::Event& event)
{
	SleepState::clock.restart();
	clockDownloadInactivity.restart();

	if (m_threadBusy || !g_syncComplete || !g_browserLoaded)
		return false;

	if (m_mode == App) {
		if (!m_appInfo.processEvent(event))
			return false;
	}
	else if (m_mode == Downloads) {
		if (!m_scrollbarDownloadQueue.processEvent(event))
			DownloadQueue::getInstance().processEvent(event);
	} else if (m_mode == Installed) {
		if (!m_scrollbarInstalledList.processEvent(event))
			InstalledList::getInstance().processEvent(event);
	} else if (m_mode == Settings) {
		m_settingsGUI->processEvent(event);
	}

	m_iconSet.processEvent(event);

	if (m_mode == Search)
	{
		if (!m_keyboard.processEvents(event))
			return true;

		cpp3ds::String currentInput;
		if (m_keyboard.popString(currentInput))
		{
			// Enter was pressed, so close keyboard
			m_iconSet.setSelectedIndex(App);
			m_lastKeyboardInput.clear();
		}
		else
		{
			currentInput = m_keyboard.getCurrentInput();
			if (m_lastKeyboardInput != currentInput)
			{
				m_lastKeyboardInput = currentInput;
				AppList::getInstance().filterBySearch(currentInput, m_textMatches);
				TweenEngine::Tween::to(AppList::getInstance(), AppList::POSITION_XY, 0.3f)
					.target(0.f, 0.f)
					.start(m_tweenManager);
			}
		}
	}
	else
	{
		// Events for all modes except Search
		AppList::getInstance().processEvent(event);
	}

	if (event.type == cpp3ds::Event::KeyPressed)
	{
		int index = AppList::getInstance().getSelectedIndex();

		switch (event.key.code)
		{
			case cpp3ds::Keyboard::Select:
				requestStackClear();
				return true;
			case cpp3ds::Keyboard::A:
			{
				// Don't load if game is already loaded
				if (m_appInfo.getAppItem() == AppList::getInstance().getSelected()->getAppItem())
					break;

				m_threadBusy = true;
				// Only fade out if a game is loaded already
				if (!m_appInfo.getAppItem())
					m_threadLoadApp.launch();
				else
					TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.3f)
						.target(0.f)
						.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
							m_threadLoadApp.launch();
						})
						.start(m_tweenManager);
				break;
			}
			case cpp3ds::Keyboard::B:
				AppList::getInstance().filterBySearch("", m_textMatches);
				break;
			case cpp3ds::Keyboard::X: {
				auto app = AppList::getInstance().getSelected()->getAppItem();
				if (app && !DownloadQueue::getInstance().isDownloading(app))
				{
					if (!app->isInstalled())
					{
						app->queueForInstall();
						Notification::spawn(_("Queued install: %s", app->getTitle().toAnsiString().c_str()));
					}
					else
						Notification::spawn(_("Already installed: %s", app->getTitle().toAnsiString().c_str()));
				}
				break;
			}
			default:
				break;
		}
	}

	return true;
}


void BrowseState::loadApp()
{
	auto item = AppList::getInstance().getSelected()->getAppItem();
	if (!item)
		return;

	// TODO: Don't show loading when game is cached?
	bool showLoading = g_browserLoaded && m_appInfo.getAppItem() != item;

	m_iconSet.setSelectedIndex(App);
	if (m_appInfo.getAppItem() != item)
	{
		setMode(App);

		if (showLoading)
			requestStackPush(States::Loading);

		m_appInfo.loadApp(item);
	}

	TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.5f)
		.target(255.f)
		.start(m_tweenManager);

	if (showLoading)
		requestStackPop();

	m_threadBusy = false;
}


void BrowseState::setMode(BrowseState::Mode mode)
{
	if (m_mode == mode || m_isTransitioning)
		return;

	// Transition / end current mode
	if (m_mode == Search)
	{
		float delay = 0.f;
		for (auto& text : m_textMatches)
		{
			TweenEngine::Tween::to(text, util3ds::RichText::POSITION_X, 0.2f)
				.target(-text.getLocalBounds().width)
				.delay(delay)
				.start(m_tweenManager);
			delay += 0.05f;
		}

		AppList::getInstance().setCollapsed(false);
	}
	else if (m_mode == Settings)
	{
		m_settingsGUI->saveToConfig();
	}

	// Transition / start new mode
	if (mode == Search)
	{
		float posY = 1.f;
		for (auto& text : m_textMatches)
		{
			text.clear();
			text.setPosition(5.f, posY);
			posY += 13.f;
		}
		AppList::getInstance().setCollapsed(true);

		m_lastKeyboardInput = "";
		m_keyboard.setCurrentInput(m_lastKeyboardInput);
	}

	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(255.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_mode = mode;
		})
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_isTransitioning = false;
		})
		.delay(0.4f)
		.start(m_tweenManager);

	m_isTransitioning = true;
}

void BrowseState::playMusic()
{
	while (!g_syncComplete || !g_browserLoaded)
		cpp3ds::sleep(cpp3ds::milliseconds(50));
	m_musicLoop.play();
	m_musicLoop.pause();
	m_musicIntro.play();
	cpp3ds::Clock clock;
	while (clock.getElapsedTime() < m_musicIntro.getDuration())
		cpp3ds::sleep(cpp3ds::milliseconds(5));
	m_musicLoop.play();
}

} // namespace FreeShop
