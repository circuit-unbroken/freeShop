#include "AppItem.hpp"
#include <cpp3ds/System/Err.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "../AssetManager.hpp"
#include "../AppItem.hpp"
#include "../Util.hpp"
#include <sstream>


namespace {

#include "../fuzzysearch.inl"

}


namespace FreeShop
{
namespace GUI
{

AppItem::AppItem()
	: m_appItem(nullptr)
	, m_infoVisible(true)
	, m_filteredOut(false)
	, m_visible(true)
	, m_matchScore(-1)
{
	deselect();

	m_icon.setSize(cpp3ds::Vector2f(48.f, 48.f));
	m_icon.setPosition(4.f, 4.f);
	m_icon.setFillColor(cpp3ds::Color(180,180,180));
	m_icon.setOutlineColor(cpp3ds::Color(0, 0, 0, 50));

	m_progressBar.setFillColor(cpp3ds::Color(0, 0, 0, 50));

	m_titleText.setCharacterSize(12);
	m_titleText.setPosition(57.f, 3.f);
	m_titleText.setFillColor(cpp3ds::Color(50,50,50));
	m_titleText.useSystemFont();

	m_filesizeText.setCharacterSize(10);
	m_filesizeText.setPosition(56.f, 38.f);
	m_filesizeText.setFillColor(cpp3ds::Color(150,150,150));
	m_filesizeText.useSystemFont();

	setSize(194.f, 56.f);
}

AppItem::~AppItem()
{

}

void AppItem::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	cpp3ds::FloatRect rect(0, 0, 400, 240);
	cpp3ds::FloatRect rect2 = states.transform.transformRect(rect);
	if (rect.intersects(rect2)) {
		if (m_infoVisible) {
			target.draw(m_background, states);
			target.draw(m_icon, states);
			target.draw(m_titleText, states);
			target.draw(m_filesizeText, states);
			for (const auto& flag : m_regionFlags)
				target.draw(flag, states);
		} else {
			target.draw(m_icon, states);
		}
	}

}

void AppItem::setSize(float width, float height)
{
	m_size.x = width;
	m_size.y = height;
	m_background.setSize(width, height);
}

const cpp3ds::Vector2f& AppItem::getSize() const
{
	return m_size;
}

void AppItem::setTitle(const cpp3ds::String &title)
{
	cpp3ds::Text tmpText = m_titleText;
	auto s = title.toUtf8();

	int lineCount = 1;
	int startPos = 0;
	int lastSpace = 0;
	for (int i = 0; i < s.size(); ++i)
	{
		if (s[i] == ' ')
			lastSpace = i;
		tmpText.setString(cpp3ds::String::fromUtf8(s.begin() + startPos, s.begin() + i));
		if (tmpText.getLocalBounds().width > 126)
		{
			if (lineCount < 2 && lastSpace != 0)
			{
				s[lastSpace] = '\n';
				i = startPos = lastSpace + 1;
				lastSpace = 0;
				lineCount++;
			}
			else
			{
				s.erase(s.begin() + i, s.end());
				break;
			}
		}
	}

	m_titleText.setString(cpp3ds::String::fromUtf8(s.begin(), s.end()));
}

void AppItem::setAppItem(std::shared_ptr<::FreeShop::AppItem> appItem)
{
	m_appItem = appItem;
	if (appItem)
	{
		setTitle(appItem->getTitle());
		setFilesize(appItem->getFilesize());
		if (appItem->getIconIndex() >= 0)
		{
			cpp3ds::IntRect rect;
			m_icon.setTexture(appItem->getIcon(rect));
			m_icon.setTextureRect(rect);
			m_icon.setFillColor(cpp3ds::Color::White);
		}

		// Region flag sprites
		m_regionFlags.clear();
		for (int i = 0; i < 3; ++i)
		{
			auto region = static_cast<FreeShop::AppItem::Region>(1 << i);
			if (region & appItem->getRegions())
				addRegionFlag(region);
		}
		float posX = 55.f;
		for (auto& flag : m_regionFlags)
		{
			flag.setColor(cpp3ds::Color(255, 255, 255, 150));
			flag.setScale(0.7f, 0.7f);
			flag.setPosition(posX, 37.f);
			posX += 21.f;
		}
	}
}

std::shared_ptr<::FreeShop::AppItem> AppItem::getAppItem() const
{
	return m_appItem;
}

void AppItem::select()
{
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/itembg-selected.9.png");
	m_background.setTexture(&texture);
	m_background.setColor(cpp3ds::Color(255, 255, 255, 200));
	m_icon.setOutlineThickness(2.f);
}

void AppItem::deselect()
{
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/itembg.9.png");
	m_background.setTexture(&texture);
	m_background.setColor(cpp3ds::Color(255, 255, 255, 40));
	m_icon.setOutlineThickness(0.f);
}

bool AppItem::isVisible() const
{
	return m_visible;
}

void AppItem::setVisible(bool visible)
{
	m_visible = visible;
}

bool AppItem::isFilteredOut() const
{
	return m_filteredOut;
}

void AppItem::setFilteredOut(bool filteredOut)
{
	m_filteredOut = filteredOut;
}

void AppItem::setInfoVisible(bool visible)
{
	m_infoVisible = visible;
}

bool AppItem::isInfoVisible() const
{
	return m_infoVisible;
}

#define SET_ALPHA(getFunc, setFunc, maxValue) \
color = getFunc(); \
color.a = std::max(std::min(newValues[0] * maxValue / 255.f, 255.f), 0.f); \
setFunc(color);

void AppItem::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case INFO_ALPHA: {
			cpp3ds::Color color;
			SET_ALPHA(m_background.getColor, m_background.setColor, 40.f);
			SET_ALPHA(m_titleText.getFillColor, m_titleText.setFillColor, 255.f);
			SET_ALPHA(m_filesizeText.getFillColor, m_filesizeText.setFillColor, 255.f);
			for (auto& flag : m_regionFlags) {
				SET_ALPHA(flag.getColor, flag.setColor, 150.f);
			}
			break;
		}
		case BACKGROUND_ALPHA: {
			cpp3ds::Color color;
			SET_ALPHA(m_background.getColor, m_background.setColor, 255.f);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int AppItem::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case BACKGROUND_ALPHA:
			returnValues[0] = m_background.getColor().a;
			return 1;
		case INFO_ALPHA:
			returnValues[0] = m_titleText.getFillColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void AppItem::setMatchTerm(const std::string &string)
{
	if (string.empty() || !m_appItem)
		m_matchScore = 0;
	else if (!fuzzy_match(string.c_str(), m_appItem->getNormalizedTitle().c_str(), m_matchScore))
		m_matchScore = -99;
}

int AppItem::getMatchScore() const
{
	return m_matchScore;
}

void AppItem::setFilesize(cpp3ds::Uint64 filesize)
{
	if (filesize > 1024 * 1024 * 1024)
		m_filesizeText.setString(_("%.1f GB", static_cast<float>(filesize) / 1024.f / 1024.f / 1024.f));
	else if (filesize > 1024 * 1024)
		m_filesizeText.setString(_("%.1f MB", static_cast<float>(filesize) / 1024.f / 1024.f));
	else
		m_filesizeText.setString(_("%d KB", filesize / 1024));

	m_filesizeText.setPosition(189.f - m_filesizeText.getLocalBounds().width, 38.f);
}

void AppItem::addRegionFlag(FreeShop::AppItem::Region region)
{
	cpp3ds::Sprite flag;
	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/flags.png");
	texture.setSmooth(true);
	flag.setTexture(texture);

	if (region == FreeShop::AppItem::Europe)
		flag.setTextureRect(cpp3ds::IntRect(0, 0, 30, 20));
	if (region == FreeShop::AppItem::Japan)
		flag.setTextureRect(cpp3ds::IntRect(30, 0, 30, 20));
	else if (region == FreeShop::AppItem::USA)
		flag.setTextureRect(cpp3ds::IntRect(0, 20, 30, 20));

	m_regionFlags.push_back(flag);
}

} // namespace GUI
} // namespace FreeShop
