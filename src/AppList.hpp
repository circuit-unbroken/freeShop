#ifndef FREESHOP_APPLIST_HPP
#define FREESHOP_APPLIST_HPP

#include <vector>
#include <string>
#include <memory>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Audio/Sound.hpp>
#include "GUI/AppItem.hpp"
#include "RichText.hpp"


namespace FreeShop
{

class AppList : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	enum SortType {
		Name,
		Size,
		VoteScore,
		VoteCount,
		ReleaseDate,
	};

	static AppList &getInstance();

	~AppList();
	void refresh();
	void setSortType(SortType sortType, bool ascending);

	void setSelectedIndex(int index);
	int getSelectedIndex() const;

	size_t getCount() const;
	size_t getVisibleCount() const;

	GUI::AppItem* getSelected();
	std::vector<std::unique_ptr<GUI::AppItem>> &getList();

	void setCollapsed(bool collapsed);
	bool isCollapsed() const;

	void filterBySearch(const std::string& searchTerm, std::vector<util3ds::RichText> &textMatches);

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

	void setFilterGenres(const std::vector<int> &genres);
	void setFilterPlatforms(const std::vector<int> &genres);
	void setFilterLanguages(int languages);
	void setFilterRegions(int regions);

protected:
	AppList(std::string jsonFilename);
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	void sort();
	void filter();
	void reposition();
	void processKeyRepeat();

private:
	SortType m_sortType;
	bool m_sortAscending;

	float m_targetPosX;
	cpp3ds::Sound  m_soundBlip;
	cpp3ds::Clock m_clockKeyRepeat;
	int m_indexDelta;
	bool m_processedFirstKey;
	bool m_startKeyRepeat;

	std::vector<int> m_filterGenres;
	std::vector<int> m_filterPlatforms;
	int m_filterRegions;
	int m_filterLanguages;

	std::string m_jsonFilename;
	int m_selectedIndex;
	std::vector<std::shared_ptr<AppItem>> m_appItems;
	std::vector<std::unique_ptr<GUI::AppItem>> m_guiAppItems;
	std::vector<cpp3ds::Texture*> m_iconTextures;
	bool m_collapsed;
	TweenEngine::TweenManager m_tweenManager;
};

} // namespace FreeShop

#endif // FREESHOP_APPLIST_HPP
