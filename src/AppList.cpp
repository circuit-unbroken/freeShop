#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <functional>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <TweenEngine/Tween.h>
#include <fstream>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include "AppList.hpp"
#include "Util.hpp"
#include "TitleKeys.hpp"
#include "AssetManager.hpp"


namespace FreeShop {

AppList::AppList(std::string jsonFilename)
: m_sortType(Name)
, m_selectedIndex(-1)
, m_collapsed(false)
, m_filterRegions(0)
, m_filterLanguages(0)
, m_sortAscending(true)
, m_targetPosX(0.f)
, m_indexDelta(0)
, m_startKeyRepeat(false)
, m_processedFirstKey(false)
{
	m_jsonFilename = jsonFilename;
	m_soundBlip.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/blip.ogg"));
}

AppList::~AppList()
{
	for (auto& texture : m_iconTextures)
		delete texture;
}

void AppList::refresh()
{
	m_appItems.clear();
	m_guiAppItems.clear();
#ifdef EMULATION
	bool isNew3DS = true;
#else
	bool isNew3DS = false;
	APT_CheckNew3DS(&isNew3DS);
#endif
	cpp3ds::Clock clock;
	cpp3ds::FileInputStream file;
	if (file.open(m_jsonFilename))
	{
		// Read file to string
		int size = file.getSize();
		std::string json;
		json.resize(size);
		file.read(&json[0], size);

		// Parse json string
		rapidjson::Document doc;
		doc.Parse(json.c_str());
		int i = 0;
		for (rapidjson::Value::ConstMemberIterator iter = doc.MemberBegin(); iter != doc.MemberEnd(); ++iter)
		{
			std::string id = iter->name.GetString();
			cpp3ds::Uint64 titleId = strtoull(id.c_str(), 0, 16);

			if (!isNew3DS && ((titleId >> 24) & 0xF) == 0xF)
				continue;

			if (TitleKeys::get(titleId))
			{
				auto appItem = std::make_shared<AppItem>();
				std::unique_ptr<GUI::AppItem> guiAppItem(new GUI::AppItem());

				appItem->loadFromJSON(iter->name.GetString(), iter->value);
				guiAppItem->setAppItem(appItem);
				// Move offscreen to avoid everything being drawn at once and crashing
				guiAppItem->setPosition(500.f, 100.f);

				m_appItems.emplace_back(std::move(appItem));
				m_guiAppItems.emplace_back(std::move(guiAppItem));
			}
		}
	}

	if (m_selectedIndex < 0 && getVisibleCount() > 0)
		m_selectedIndex = 0;

	setSelectedIndex(m_selectedIndex);
}

bool AppList::processEvent(const cpp3ds::Event &event)
{
	if (event.type == cpp3ds::Event::KeyPressed)
	{
		m_processedFirstKey = false;
		if (event.key.code & cpp3ds::Keyboard::Up) {
			m_indexDelta = -1;
		} else if (event.key.code & cpp3ds::Keyboard::Down) {
			m_indexDelta = 1;
		} else if (event.key.code & cpp3ds::Keyboard::Left) {
			m_indexDelta = -4;
		} else if (event.key.code & cpp3ds::Keyboard::Right) {
			m_indexDelta = 4;
		} else if (event.key.code & cpp3ds::Keyboard::L) {
			m_indexDelta = -8;
		} else if (event.key.code & cpp3ds::Keyboard::R) {
			m_indexDelta = 8;
		} else
			m_processedFirstKey = true;
	}
	else if (event.type == cpp3ds::Event::KeyReleased)
	{
		if (event.key.code & (cpp3ds::Keyboard::Up | cpp3ds::Keyboard::Down | cpp3ds::Keyboard::Left | cpp3ds::Keyboard::Right | cpp3ds::Keyboard::L | cpp3ds::Keyboard::R))
		{
			if (cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::Up)
					|| cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::Down)
					|| cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::Left)
					|| cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::Right)
					|| cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::L)
					|| cpp3ds::Keyboard::isKeyDown(cpp3ds::Keyboard::R))
				return false;
			m_indexDelta = 0;
			m_startKeyRepeat = false;
			m_processedFirstKey = false;
		}
	}
	return false;
}

void AppList::update(float delta)
{
	if (m_indexDelta != 0)
	{
		if (m_startKeyRepeat)
		{
			if (m_clockKeyRepeat.getElapsedTime() > cpp3ds::milliseconds(60))
				processKeyRepeat();
		}
		else if (!m_processedFirstKey)
		{
			m_processedFirstKey = true;
			processKeyRepeat();
		}
		else if (m_clockKeyRepeat.getElapsedTime() > cpp3ds::milliseconds(300))
			m_startKeyRepeat = true;
	}

	m_tweenManager.update(delta);
}

void AppList::processKeyRepeat()
{
	int index = getSelectedIndex();
	// Don't keep changing index on top/bottom boundaries
	if ((m_indexDelta != 1 || index % 4 != 3) && (m_indexDelta != -1 || index % 4 != 0))
	{
		int newIndex = index + m_indexDelta;
		if (newIndex < 0)
			newIndex = 0;
		if (newIndex >= getVisibleCount())
			newIndex = getVisibleCount() - 1;

		if (newIndex != index)
			m_soundBlip.play(1);

		setSelectedIndex(newIndex);
		m_clockKeyRepeat.restart();
	}
}

void AppList::setSortType(AppList::SortType sortType, bool ascending)
{
	m_sortType = sortType;
	m_sortAscending = ascending;
	sort();
}

void AppList::sort()
{
	setSelectedIndex(-1);
	m_tweenManager.killAll();
	std::sort(m_guiAppItems.begin(), m_guiAppItems.end(), [&](const std::unique_ptr<GUI::AppItem>& a, const std::unique_ptr<GUI::AppItem>& b)
	{
		if (a->isFilteredOut() != b->isFilteredOut())
			return !a->isFilteredOut();

		if (a->getMatchScore() != b->getMatchScore())
		{
			return a->getMatchScore() > b->getMatchScore();
		}
		else
		{
			if (m_sortAscending)
			{
				switch(m_sortType) {
					case Name: return a->getAppItem()->getNormalizedTitle() < b->getAppItem()->getNormalizedTitle();
					case Size: return a->getAppItem()->getFilesize() < b->getAppItem()->getFilesize();
					case VoteScore: return a->getAppItem()->getVoteScore() < b->getAppItem()->getVoteScore();
					case VoteCount: return a->getAppItem()->getVoteCount() < b->getAppItem()->getVoteCount();
					case ReleaseDate: return a->getAppItem()->getReleaseDate() < b->getAppItem()->getReleaseDate();
				}
			}
			else
			{
				switch(m_sortType) {
					case Name: return a->getAppItem()->getNormalizedTitle() > b->getAppItem()->getNormalizedTitle();
					case Size: return a->getAppItem()->getFilesize() > b->getAppItem()->getFilesize();
					case VoteScore: return a->getAppItem()->getVoteScore() > b->getAppItem()->getVoteScore();
					case VoteCount: return a->getAppItem()->getVoteCount() > b->getAppItem()->getVoteCount();
					case ReleaseDate: return a->getAppItem()->getReleaseDate() > b->getAppItem()->getReleaseDate();
				}
			}
		}
	});
	if (!m_guiAppItems.empty())
		setSelectedIndex(0);
	reposition();
}

void AppList::filter()
{
	// Region filter
	// Also resets the filter state when no region filter is set.
	if (m_filterRegions)
	{
		for (const auto& appItemGUI : m_guiAppItems)
			appItemGUI->setFilteredOut(!(appItemGUI->getAppItem()->getRegions() & m_filterRegions));
	}
	else
		for (const auto& appItemGUI : m_guiAppItems)
			appItemGUI->setFilteredOut(false);

	// Language filter
	if (m_filterLanguages != 0)
	{
		for (const auto& appItemGUI : m_guiAppItems)
			if (appItemGUI->isVisible())
				if (!(appItemGUI->getAppItem()->getLanguages() & m_filterLanguages))
					appItemGUI->setFilteredOut(true);
	}

	// Genre filter
	if (!m_filterGenres.empty())
	{
		for (const auto& appItemGUI : m_guiAppItems)
			if (appItemGUI->isVisible())
			{
				for (const auto& appGenre : appItemGUI->getAppItem()->getGenres())
					for (const auto& filterGenre : m_filterGenres)
						if (appGenre == filterGenre)
							goto matchedGenre;
				appItemGUI->setFilteredOut(true);
matchedGenre:;
			}
	}

	// Platform filter
	if (!m_filterPlatforms.empty())
	{
		for (const auto& appItemGUI : m_guiAppItems)
			if (appItemGUI->isVisible())
			{
				for (const auto& filterPlatform : m_filterPlatforms)
					if (appItemGUI->getAppItem()->getPlatform() == filterPlatform)
						goto matchedPlatform;
				appItemGUI->setFilteredOut(true);
matchedPlatform:;
			}
	}

	sort();
	setPosition(0.f, 0.f);
}

void AppList::reposition()
{
	bool segmentFound = false;
	float destY = 4.f;
	int i = 0;
	float newX = (m_selectedIndex ? : 0) / 4 * (m_collapsed ? 59.f : 200.f);

	for (auto& app : m_guiAppItems)
	{
		if (!app->isVisible() || app->isFilteredOut())
			continue;

		float destX = 3.f + (i/4) * (m_collapsed ? 59.f : 200.f);
		float itemPosX = app->getPosition().x + getPosition().x;
		if ((destX-newX < -200.f || destX-newX > 400.f) && (itemPosX < -200 || itemPosX > 400.f))
		{
			if (segmentFound == m_collapsed)
				TweenEngine::Tween::set(*app, GUI::AppItem::POSITION_XY)
					.target(destX, destY)
					.delay(0.3f)
					.start(m_tweenManager);
			else
				app->setPosition(destX, destY);
		}
		else
		{
			segmentFound = true;
			TweenEngine::Tween::to(*app, GUI::AppItem::POSITION_XY, 0.3f)
				.target(destX, destY)
				.start(m_tweenManager);
		}

		if (++i % 4 == 0)
			destY = 4.f;
		else
			destY += 59.f;
	}
}

void AppList::setSelectedIndex(int index)
{
	if (getVisibleCount() == 0)
		return;

	if (index >= 0)
	{
		if (index >= getVisibleCount())
			index = getVisibleCount() - 1;

		float extra = 1.0f; //std::abs(m_appList.getSelectedIndex() - index) == 8.f ? 2.f : 1.f;

		float pos = -200.f * extra * (index / 4);
		if (pos > m_targetPosX)
			m_targetPosX = pos;
		else if (pos <= m_targetPosX - 400.f)
			m_targetPosX = pos + 200.f * extra;

		TweenEngine::Tween::to(*this, AppList::POSITION_X, 0.3f)
			.target(m_targetPosX)
			.start(m_tweenManager);
	}

	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->deselect();
	m_selectedIndex = index;
	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->select();
}

int AppList::getSelectedIndex() const
{
	return m_selectedIndex;
}

GUI::AppItem *AppList::getSelected()
{
	if (m_selectedIndex < 0 || m_selectedIndex > m_guiAppItems.size()-1)
		return nullptr;
	return m_guiAppItems[m_selectedIndex].get();
}

void AppList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	for (auto& app : m_guiAppItems)
	{
		if (app->isVisible() && !app->isFilteredOut())
			target.draw(*app, states);
	}
}

size_t AppList::getCount() const
{
	return m_guiAppItems.size();
}

size_t AppList::getVisibleCount() const
{
	size_t count = 0;
	for (auto& item : m_guiAppItems)
		if (item->isVisible() && !item->isFilteredOut())
			count++;
		else break;

	return count;
}

void AppList::setCollapsed(bool collapsed)
{
	if (m_collapsed == collapsed)
		return;

	m_tweenManager.killAll();
	float newX = m_selectedIndex / 4 * (collapsed ? 59.f : 200.f);

	if (collapsed)
	{
		for (auto &app : m_guiAppItems)
		{
			float appPosX = app->getPosition().x + getPosition().x;
			if (appPosX >= 0.f && appPosX < 400.f)
				TweenEngine::Tween::to(*app, GUI::AppItem::INFO_ALPHA, 0.3f)
					.target(0)
					.setCallback(TweenEngine::TweenCallback::COMPLETE, [&](TweenEngine::BaseTween *source) {
						app->setInfoVisible(false);
					})
					.start(m_tweenManager);
			else {
				app->setInfoVisible(false);
				TweenEngine::Tween::set(*app, GUI::AppItem::INFO_ALPHA)
					.target(0)
					.start(m_tweenManager);
			}
		}

		TweenEngine::Tween::to(*this, AppList::POSITION_X, 0.3f)
			.target(-newX)
			.delay(0.3f)
			.setCallback(TweenEngine::TweenCallback::START, [=](TweenEngine::BaseTween *source) {
				m_collapsed = collapsed;
				reposition();
			})
			.start(m_tweenManager);
	}
	else
	{
		m_collapsed = collapsed;
		reposition();
		TweenEngine::Tween::to(*this, AppList::POSITION_X, 0.3f)
			.target(-newX)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [this, newX](TweenEngine::BaseTween *source) {
				for (auto &app : m_guiAppItems) {
					float appPosX = app->getPosition().x - newX;
					app->setInfoVisible(true);
					if (appPosX >= 0.f && appPosX < 400.f)
						TweenEngine::Tween::to(*app, GUI::AppItem::INFO_ALPHA, 0.3f)
							.target(255.f)
							.start(m_tweenManager);
					else
						TweenEngine::Tween::set(*app, GUI::AppItem::INFO_ALPHA)
							.target(255.f)
							.start(m_tweenManager);
				}

				GUI::AppItem *item = getSelected();
				if (item)
					TweenEngine::Tween::to(*item, GUI::AppItem::BACKGROUND_ALPHA, 0.3f)
						.target(255.f)
						.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween *source) {
							setSelectedIndex(m_selectedIndex);
						})
						.delay(0.3f)
						.start(m_tweenManager);
			})
			.start(m_tweenManager);
	}
}

bool AppList::isCollapsed() const
{
	return m_collapsed;
}

void AppList::filterBySearch(const std::string &searchTerm, std::vector<util3ds::RichText> &textMatches)
{
	m_tweenManager.killAll();
	for (auto& item : m_guiAppItems)
	{
		item->setMatchTerm(searchTerm);
		item->setVisible(item->getMatchScore() > -99);
	}

	if (m_selectedIndex >= 0)
		m_guiAppItems[m_selectedIndex]->deselect();
	sort();

	int i = 0;
	for (auto& textMatch : textMatches)
	{
		textMatch.clear();
		if (searchTerm.empty())
			continue;

		if (i < getVisibleCount())
		{
			auto item = m_guiAppItems[i].get();
			if (item->getMatchScore() > -99)
			{
				bool matching = false;
				const char *str = item->getAppItem()->getNormalizedTitle().c_str();
				const char *pattern = searchTerm.c_str();
				const char *strLastPos = str;

				auto title = item->getAppItem()->getTitle().toUtf8();
				auto titleCurPos = title.begin();
				auto titleLastPos = title.begin();

				textMatch << cpp3ds::Color(150,150,150);
				while (*str != '\0')  {
					if (tolower(*pattern) == tolower(*str)) {
						if (!matching) {
							matching = true;
							if (str != strLastPos) {
								textMatch << cpp3ds::String::fromUtf8(titleLastPos, titleCurPos);
								titleLastPos = titleCurPos;
								strLastPos = str;
							}
							textMatch << cpp3ds::Color::Black;
						}
						++pattern;
					} else {
						if (matching) {
							matching = false;
							if (str != strLastPos) {
								textMatch << cpp3ds::String::fromUtf8(titleLastPos, titleCurPos);
								titleLastPos = titleCurPos;
								strLastPos = str;
							}
							textMatch << cpp3ds::Color(150,150,150);
						}
					}
					++str;
					titleCurPos = cpp3ds::Utf8::next(titleCurPos, title.end());
				}
				if (str != strLastPos)
					textMatch << cpp3ds::String::fromUtf8(titleLastPos, title.end());
			}
			i++;
		}
	}

	if (m_guiAppItems.size() > 0 && textMatches.size() > 0)
		setSelectedIndex(0);
}

std::vector<std::unique_ptr<GUI::AppItem>> &AppList::getList()
{
	return m_guiAppItems;
}

AppList &AppList::getInstance()
{
	static AppList list(FREESHOP_DIR "/cache/data.json");
	return list;
}

void AppList::setFilterGenres(const std::vector<int> &genres)
{
	m_filterGenres = genres;
	filter();
}

void AppList::setFilterPlatforms(const std::vector<int> &platforms)
{
	m_filterPlatforms = platforms;
	filter();
}

void AppList::setFilterRegions(int regions)
{
	m_filterRegions = regions;
	filter();
}

void AppList::setFilterLanguages(int languages)
{
	m_filterLanguages = languages;
	filter();
}


} // namespace FreeShop