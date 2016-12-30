#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/Window/Window.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "AssetManager.hpp"
#include "AppInfo.hpp"
#include "Download.hpp"
#include "Util.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "InstalledList.hpp"
#include "FreeShop.hpp"
#include "States/DialogState.hpp"
#include <sstream>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <sys/stat.h>
#include <cmath>


namespace FreeShop
{

AppInfo::AppInfo()
: m_appItem(nullptr)
, m_currentScreenshot(0)
, m_descriptionVelocity(0.f)
, m_isDemoInstalled(false)
{
	m_textDownload.setFillColor(cpp3ds::Color::White);
	m_textDownload.setOutlineColor(cpp3ds::Color(0, 0, 0, 200));
	m_textDownload.setOutlineThickness(2.f);
	m_textDownload.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textDownload.setString("\uf019");
	m_textDownload.setCharacterSize(30);
	m_textDownload.setPosition(67.f, 93.f);
	m_textDelete = m_textDownload;
	m_textDelete.setString("\uf1f8");
	m_textDelete.setPosition(70.f, 90.f);
	m_textExecute = m_textDownload;
	m_textExecute.setString("\uf01d");
	m_textExecute.setPosition(5.f, 90.f);

	m_arrowLeft.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_arrowLeft.setCharacterSize(24);
	m_arrowLeft.setFillColor(cpp3ds::Color(255,255,255,150));
	m_arrowLeft.setOutlineColor(cpp3ds::Color(0,0,0,100));
	m_arrowLeft.setOutlineThickness(1.f);
	m_arrowLeft.setPosition(4.f, 100.f);
	m_arrowLeft.setString("\uf053");
	m_arrowRight = m_arrowLeft;
	m_arrowRight.setPosition(298.f, 100.f);
	m_arrowRight.setString("\uf054");
	m_close = m_arrowLeft;
	m_close.setCharacterSize(30);
	m_close.setPosition(285.f, 4.f);
	m_close.setString("\uf00d");

	m_screenshotsBackground.setFillColor(cpp3ds::Color(190, 190, 190, 255));
	m_screenshotsBackground.setSize(cpp3ds::Vector2f(320.f, 74.f));
	m_screenshotsBackground.setPosition(0.f, 166.f);

	m_textScreenshotsEmpty.setCharacterSize(12);
	m_textScreenshotsEmpty.setFillColor(cpp3ds::Color(255, 255, 255, 220));
	m_textScreenshotsEmpty.useSystemFont();
	m_textScreenshotsEmpty.setString(_("No Screenshots"));
	cpp3ds::FloatRect textRect = m_textScreenshotsEmpty.getLocalBounds();
	m_textScreenshotsEmpty.setOrigin(textRect.left + textRect.width / 2.f, textRect.top + textRect.height / 2.f);
	m_textScreenshotsEmpty.setPosition(160.f, 202.f);

	m_textNothingSelected.setString(_("No game selected"));
	m_textNothingSelected.setCharacterSize(16);
	m_textNothingSelected.setFillColor(cpp3ds::Color(0, 0, 0, 120));
	m_textNothingSelected.useSystemFont();
	m_textNothingSelected.setPosition(160.f, 105.f);
	m_textNothingSelected.setOrigin(m_textNothingSelected.getLocalBounds().width / 2, m_textNothingSelected.getLocalBounds().height / 2);
	TweenEngine::Tween::to(m_textNothingSelected, m_textNothingSelected.SCALE_XY, 4.f)
		.target(0.9f, 0.9f).repeatYoyo(-1, 0.f).start(m_tweenManager);

	m_textTitle.setCharacterSize(15);
	m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.setStyle(cpp3ds::Text::Bold);
	m_textTitle.setPosition(207.f, 28.f);
	m_textTitle.useSystemFont();

	m_textDescription.setCharacterSize(12);
	m_textDescription.setFillColor(cpp3ds::Color(100, 100, 100, 255));
	m_textDescription.useSystemFont();

	m_textTitleId.setCharacterSize(10);
	m_textTitleId.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textTitleId.setPosition(2.f, 127.f);

	m_textDemo.setString(_("Demo"));
	m_textDemo.setFillColor(cpp3ds::Color(100, 100, 100));
	m_textDemo.setCharacterSize(10);
	m_textDemo.setPosition(25.f, 140.f);
	m_textDemo.useSystemFont();

	m_textIconDemo.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textIconDemo.setFillColor(cpp3ds::Color(50, 100, 50));
	m_textIconDemo.setCharacterSize(18);
	m_textIconDemo.setPosition(5.f, 135.f);

	m_fadeTextRect.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fade.png"));
	m_fadeTextRect.setSize(cpp3ds::Vector2f(250.f, 8.f));
	m_fadeTextRect.setOrigin(m_fadeTextRect.getSize());
	m_fadeTextRect.setRotation(180.f);
	m_fadeTextRect.setPosition(102.f, 46.f);

	m_icon.setPosition(2.f, 30.f);
	m_icon.setScale(2.f, 2.f);

	m_fadeRect.setPosition(0.f, 30.f);
	m_fadeRect.setSize(cpp3ds::Vector2f(320.f, 210.f));
	m_fadeRect.setFillColor(cpp3ds::Color::White);
}

AppInfo::~AppInfo()
{

}

void AppInfo::drawTop(cpp3ds::Window &window)
{
	if (m_currentScreenshot)
		window.draw(m_screenshotTopTop);
}

void AppInfo::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	if (m_appItem)
	{
		target.draw(m_screenshotsBackground, states);
		if (m_screenshotTops.empty())
			target.draw(m_textScreenshotsEmpty, states);

		target.draw(m_icon, states);
		target.draw(m_textTitle, states);

		states.scissor = cpp3ds::UintRect(0, 46, 320, 120);
		target.draw(m_textDescription, states);
		states.scissor = cpp3ds::UintRect();
		target.draw(m_fadeTextRect, states);

		if (m_appItem->isInstalled())
		{
			target.draw(m_textExecute, states);
			target.draw(m_textDelete, states);
		}
		else if (!DownloadQueue::getInstance().isDownloading(m_appItem))
		{
			target.draw(m_textDownload, states);
		}

		if (!m_appItem->getDemos().empty() && !DownloadQueue::getInstance().isDownloading(m_appItem->getDemos()[0]))
		{
			target.draw(m_textDemo, states);
			target.draw(m_textIconDemo, states);
		}

		for (auto& screenshot : m_screenshotTops)
			target.draw(*screenshot, states);
		for (auto& screenshot : m_screenshotBottoms)
			target.draw(*screenshot, states);

		target.draw(m_fadeRect, states);
	}
	else
	{
		target.draw(m_textNothingSelected, states);
	}

	if (m_currentScreenshot)
	{
		target.draw(m_screenshotTopBottom);
		target.draw(m_screenshotBottom);
		if (m_currentScreenshot > 1)
			target.draw(m_arrowLeft);
		if (m_currentScreenshot < m_screenshotTops.size())
			target.draw(m_arrowRight);
		target.draw(m_close);
	}
}

void AppInfo::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case ALPHA: {
			cpp3ds::Color color = m_fadeRect.getFillColor();
			color.a = 255.f - std::max(std::min(newValues[0], 255.f), 0.f);
			m_fadeRect.setFillColor(color);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int AppInfo::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case ALPHA:
			returnValues[0] = 255.f - m_fadeRect.getFillColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void AppInfo::loadApp(std::shared_ptr<AppItem> appItem)
{
	if (m_appItem == appItem)
		return;

	m_appItem = appItem;

	if (appItem)
	{
		std::string countryCode = getCountryCode(appItem->getRegions());
		std::string jsonFilename = appItem->getJsonFilename();
		std::string urlTitleData = "https://samurai.ctr.shop.nintendo.net/samurai/ws/" + countryCode + "/title/" + appItem->getContentId() + "/?shop_id=1";
		std::string dir = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp/" + appItem->getTitleIdStr());
		if (!pathExists(dir.c_str(), false))
			mkdir(dir.c_str(), 0777);

		if (!appItem->isCached())
		{
			Download download(urlTitleData, jsonFilename);
			download.setField("Accept", "application/json");
			download.run();

			auto status = download.getLastResponse().getStatus();
			if (status != cpp3ds::Http::Response::Ok && status != cpp3ds::Http::Response::PartialContent)
			{
				urlTitleData = "https://samurai.ctr.shop.nintendo.net/samurai/ws/" + appItem->getUriRegion() + "/title/" + appItem->getContentId() + "/?shop_id=1";
				download.setUrl(urlTitleData);
				download.run();
			}
		}

		updateInfo();

		m_textTitle.setString("");
		m_textDescription.setString("");
		m_textTitleId.setString(appItem->getTitleIdStr());

		cpp3ds::IntRect textureRect;
		m_icon.setTexture(*appItem->getIcon(textureRect), true);
		m_icon.setTextureRect(textureRect);

		m_screenshotTops.clear();
		m_screenshotTopTextures.clear();
		m_screenshotBottoms.clear();
		m_screenshotBottomTextures.clear();

		rapidjson::Document doc;
		std::string json;
		cpp3ds::FileInputStream file;
		if (file.open(jsonFilename))
		{
			json.resize(file.getSize());
			file.read(&json[0], json.size());
			doc.Parse(json.c_str());
			if (!doc.HasParseError())
			{
				if (doc["title"].HasMember("screenshots"))
					setScreenshots(doc["title"]["screenshots"]["screenshot"]);

				if (doc["title"].HasMember("description"))
					setDescription(doc["title"]["description"]);
				m_textDescription.setPosition(102.f, 49.f);

				m_textTitle.setString(appItem->getTitle());
				m_textTitle.setOrigin(std::min(m_textTitle.getLocalBounds().width, 205.f) / 2.f, 0.f);

				m_textDownload.setFillColor(cpp3ds::Color::White);
			}
		}
	}
}

const std::shared_ptr<AppItem> AppInfo::getAppItem() const
{
	return m_appItem;
}

#define SCREENSHOT_BUTTON_FADEIN(object) \
	object.setFillColor(cpp3ds::Color(255,255,255,190)); \
	object.setOutlineColor(cpp3ds::Color(0,0,0,100)); \
	TweenEngine::Tween::from(object, util3ds::TweenText::FILL_COLOR_ALPHA, 4.f) \
		.target(0).start(m_tweenManager); \
	TweenEngine::Tween::from(object, util3ds::TweenText::OUTLINE_COLOR_ALPHA, 4.f) \
		.target(0).start(m_tweenManager);
#define SCREENSHOT_BUTTON_FADEOUT(object) \
	TweenEngine::Tween::to(object, util3ds::TweenText::FILL_COLOR_ALPHA, 0.5f) \
		.target(0).start(m_tweenManager); \
	TweenEngine::Tween::to(object, util3ds::TweenText::OUTLINE_COLOR_ALPHA, 0.5f) \
		.target(0).start(m_tweenManager);

void AppInfo::setCurrentScreenshot(int screenshotIndex)
{
	if (screenshotIndex != 0)
	{
		cpp3ds::Sprite* screenshotTop = m_screenshotTops[screenshotIndex-1].get();
		cpp3ds::Sprite* screenshotBottom = m_screenshotBottoms[screenshotIndex-1].get();

		m_screenshotTopTop.setTexture(*screenshotTop->getTexture(), true);
		m_screenshotTopBottom.setTexture(*screenshotTop->getTexture(), true);
		m_screenshotBottom.setTexture(*screenshotBottom->getTexture(), true);

		// If another screenshot not already showing
		if (m_currentScreenshot == 0)
		{
			SCREENSHOT_BUTTON_FADEIN(m_close);
			SCREENSHOT_BUTTON_FADEIN(m_arrowLeft);
			SCREENSHOT_BUTTON_FADEIN(m_arrowRight);

			m_screenshotTopTop.setPosition(screenshotTop->getPosition() + cpp3ds::Vector2f(0.f, 270.f));
			m_screenshotTopTop.setScale(screenshotTop->getScale());
			m_screenshotTopBottom.setPosition(screenshotTop->getPosition());
			m_screenshotTopBottom.setScale(screenshotTop->getScale());
			m_screenshotBottom.setPosition(screenshotBottom->getPosition());
			m_screenshotBottom.setScale(screenshotBottom->getScale());

			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.5f)
					.target(0.18f, 0.18f)
					.ease(TweenEngine::TweenEquations::easeOutElastic)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.5f)
					.targetRelative(-6.f, -6.f)
					.ease(TweenEngine::TweenEquations::easeOutElastic)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.5f)
					.target(0.18f, 0.18f)
					.ease(TweenEngine::TweenEquations::easeOutElastic)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.5f)
					.targetRelative(-6.f, 0.f)
					.ease(TweenEngine::TweenEquations::easeOutElastic)
					.start(m_tweenManager);

			TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f, 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(0.f, 0.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f, 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(-40.f, -240.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f, 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(0.f, 0.f)
					.delay(0.5f)
					.start(m_tweenManager);
		}
	}
	else if (m_currentScreenshot != 0)
	{
		// Close screenshot
		cpp3ds::Sprite* screenshotTop = m_screenshotTops[m_currentScreenshot-1].get();
		cpp3ds::Sprite* screenshotBottom = m_screenshotBottoms[m_currentScreenshot-1].get();

		SCREENSHOT_BUTTON_FADEOUT(m_close);
		SCREENSHOT_BUTTON_FADEOUT(m_arrowLeft);
		SCREENSHOT_BUTTON_FADEOUT(m_arrowRight);

		TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f, 0.15f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotTop->getPosition().x, screenshotTop->getPosition().y + 270.f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f, 0.15f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotTop->getPosition().x, screenshotTop->getPosition().y)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f, 0.15f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotBottom->getPosition().x, screenshotBottom->getPosition().y)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_currentScreenshot = screenshotIndex;
				})
				.start(m_tweenManager);
		return;
	}

	m_currentScreenshot = screenshotIndex;
}

bool AppInfo::processEvent(const cpp3ds::Event &event)
{
	if (!m_appItem)
		return true;

	if (m_currentScreenshot)
	{
		if (event.type == cpp3ds::Event::KeyPressed)
		{
			switch (event.key.code)
			{
				case cpp3ds::Keyboard::B:
					setCurrentScreenshot(0);
					break;
				case cpp3ds::Keyboard::DPadRight:
					if (m_currentScreenshot < m_screenshotTops.size())
						setCurrentScreenshot(m_currentScreenshot + 1);
					break;
				case cpp3ds::Keyboard::DPadLeft:
					if (m_currentScreenshot > 1)
						setCurrentScreenshot(m_currentScreenshot - 1);
					break;
				default:
					break;
			}
		}
		else if (event.type == cpp3ds::Event::TouchBegan)
		{
			if (m_close.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				setCurrentScreenshot(0);
			}
			else if (m_currentScreenshot > 1 && event.touch.x < 140)
			{
				setCurrentScreenshot(m_currentScreenshot - 1);
			}
			else if (m_currentScreenshot < m_screenshotTops.size() && event.touch.x > 180)
			{
				setCurrentScreenshot(m_currentScreenshot + 1);
			}
		}
		return false;
	}

	if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_textIconDemo.getGlobalBounds().contains(event.touch.x, event.touch.y))
		{
			if (!m_appItem->getDemos().empty())
			{
				cpp3ds::Uint64 demoTitleId = m_appItem->getDemos()[0];
				cpp3ds::String appTitle = m_appItem->getTitle();

				if (InstalledList::isInstalled(demoTitleId))
				{
					g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
						auto event = reinterpret_cast<DialogState::Event*>(data);
						if (event->type == DialogState::GetText)
						{
							auto str = reinterpret_cast<cpp3ds::String*>(event->data);
							*str = _("You are deleting the demo for\nthis title:\n\n%s", appTitle.toAnsiString().c_str());
							return true;
						}
						else if (event->type == DialogState::Response)
						{
							bool *accepted = reinterpret_cast<bool*>(event->data);
							if (*accepted)
							{
#ifdef _3DS
								AM_DeleteTitle(MEDIATYPE_SD, demoTitleId);
#endif
								Notification::spawn(_("Deleted demo: %s", appTitle.toAnsiString().c_str()));
								InstalledList::getInstance().refresh();
								updateInfo();
							}
							return true;
						}
						return false;
					});
				}
				else
				{
					DownloadQueue::getInstance().addDownload(m_appItem, demoTitleId, [this](bool succeeded){
						updateInfo();
					});
					Notification::spawn(_("Queued demo: %s", appTitle.toAnsiString().c_str()));
				}
			}
		}
		else if (m_appItem->isInstalled())
		{
			if (m_textExecute.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				g_requestJump = m_appItem->getTitleId();
			}
			else if (m_textDelete.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				cpp3ds::String appTitle = m_appItem->getTitle();
				g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
					auto event = reinterpret_cast<DialogState::Event*>(data);
					if (event->type == DialogState::GetText)
					{
						auto str = reinterpret_cast<cpp3ds::String*>(event->data);
						*str = _("You are deleting this game,\nincluding all save data:\n\n%s", appTitle.toAnsiString().c_str());
						return true;
					}
					else if (event->type == DialogState::Response)
					{
						bool *accepted = reinterpret_cast<bool*>(event->data);
						if (*accepted)
						{
#ifdef _3DS
							FS_MediaType mediaType = ((m_appItem->getTitleId() >> 32) == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
							AM_DeleteTitle(mediaType, m_appItem->getTitleId());
#endif
							m_appItem->setInstalled(false);
							Notification::spawn(_("Deleted: %s", appTitle.toAnsiString().c_str()));
							InstalledList::getInstance().refresh();
							updateInfo();
						}
						return true;
					}
					return false;
				});
			}
		}
		else if (m_textDownload.getGlobalBounds().contains(event.touch.x, event.touch.y))
		{
			if (!DownloadQueue::getInstance().isDownloading(m_appItem))
			{
				DownloadQueue::getInstance().addDownload(m_appItem);
				TweenEngine::Tween::to(m_textDownload, util3ds::TweenText::FILL_COLOR_RGB, 0.5f)
						.target(230, 20, 20)
						.start(m_tweenManager);

				Notification::spawn(_("Queued install: %s", m_appItem->getTitle().toAnsiString().c_str()));
			}
		}
	}

	for (int i = 0; i < m_screenshotTops.size(); ++i)
	{
		if (m_screenshotTops[i]->getGlobalBounds().contains(event.touch.x, event.touch.y))
			setCurrentScreenshot(i+1);
		else if (m_screenshotBottoms[i]->getGlobalBounds().contains(event.touch.x, event.touch.y))
			setCurrentScreenshot(i+1);
	}

	if (event.type == cpp3ds::Event::JoystickMoved)
	{
		m_descriptionVelocity = event.joystickMove.y * 2.f;
	}

	return true;
}

void AppInfo::update(float delta)
{
//	if (!m_downloads.isDownloading(m_currentApp) && !m_downloads.isInstalling(m_currentApp))
//	{
//		m_textDownload.setFillColor(cpp3ds::Color::White);
//	}

	m_textDescription.move(0.f, m_descriptionVelocity * delta);
	if (m_textDescription.getPosition().y < 49.f - m_textDescription.getLocalBounds().height + 110.f)
		m_textDescription.setPosition(102.f, 49.f - m_textDescription.getLocalBounds().height + 110.f);
	if (m_textDescription.getPosition().y > 49.f)
		m_textDescription.setPosition(102.f, 49.f);

	m_tweenManager.update(delta);
}

void AppInfo::setDescription(const rapidjson::Value &jsonDescription)
{
	// Decode description, strip newlines
	const char *dd = jsonDescription.GetString();
	cpp3ds::String description = cpp3ds::String::fromUtf8(dd, dd + jsonDescription.GetStringLength());
	description.replace("\n", " ");
	description.replace("<br>", "\n");
	description.replace("<BR>", "\n");
	description.replace("<br/>", "\n");

	// Calculate word-wrapping for description
	int startPos = 0;
	int lastSpace = 0;
	auto s = description.toUtf8();
	cpp3ds::Text tmpText = m_textDescription;
	for (int i = 0; i < s.size(); ++i)
	{
		if (s[i] == ' ')
			lastSpace = i;
		tmpText.setString(cpp3ds::String::fromUtf8(s.begin() + startPos, s.begin() + i));
		if (tmpText.getLocalBounds().width > 206)
		{
			if (lastSpace != 0)
			{
				s[lastSpace] = '\n';
				i = startPos = lastSpace + 1;
				lastSpace = 0;
			}
			else
			{
				s.insert(s.begin() + i, '\n');
				startPos = ++i;
			}
		}
	}

	m_textDescription.setString(cpp3ds::String::fromUtf8(s.begin(), s.end()));
}

void AppInfo::setScreenshots(const rapidjson::Value &jsonScreenshots)
{
	if (jsonScreenshots.IsArray())
		for (int i = 0; i < jsonScreenshots.Size(); ++i)
		{
			addScreenshot(i, jsonScreenshots[i]["image_url"][0]);
			addScreenshot(i, jsonScreenshots[i]["image_url"][1]);
		}

	float startX = std::round((320.f - 61.f * m_screenshotTops.size()) / 2.f);
	for (int i = 0; i < m_screenshotTops.size(); ++i)
	{
		m_screenshotTops[i]->setPosition(startX + i * 61.f, 167.f);
		m_screenshotBottoms[i]->setPosition(startX + 6.f + i * 61.f, 203.f);
	}
}

void AppInfo::addScreenshot(int index, const rapidjson::Value &jsonScreenshot)
{
	std::string url = jsonScreenshot["value"].GetString();
	std::string type = jsonScreenshot["type"].GetString();
	std::string filename = _(FREESHOP_DIR "/tmp/%s/screen%d%s.jpg", m_appItem->getTitleIdStr().c_str(), index, type.c_str());
	bool isUpper = type == "upper";

	if (!pathExists(filename.c_str()))
	{
		Download download(url, filename);
		download.run();
	}

	std::unique_ptr<cpp3ds::Texture> texture(new cpp3ds::Texture());
	std::unique_ptr<cpp3ds::Sprite> sprite(new cpp3ds::Sprite());

	texture->loadFromFile(filename);
	texture->setSmooth(true);
	sprite->setTexture(*texture, true);
	sprite->setScale(0.15f, 0.15f);
	sprite->setPosition(400.f, 0.f); // Keep offscreen

	if (isUpper) {
		m_screenshotTops.emplace_back(std::move(sprite));
		m_screenshotTopTextures.emplace_back(std::move(texture));
	} else {
		m_screenshotBottoms.emplace_back(std::move(sprite));
		m_screenshotBottomTextures.emplace_back(std::move(texture));
	}
}

void AppInfo::updateInfo()
{
	if (m_appItem->getDemos().size() > 0)
	{
		if (InstalledList::isInstalled(m_appItem->getDemos()[0])) {
			m_textIconDemo.setString(L"\uf1f8");
		} else {
			m_textIconDemo.setString(L"\uf019");
		}
	}
}


} // namespace FreeShop
