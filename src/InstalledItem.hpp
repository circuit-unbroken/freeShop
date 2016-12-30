#ifndef FREESHOP_INSTALLEDITEM_HPP
#define FREESHOP_INSTALLEDITEM_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "GUI/NinePatch.hpp"
#include "AppItem.hpp"

namespace FreeShop {

class InstalledItem : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int HEIGHT = 11; // for tweening

	InstalledItem(cpp3ds::Uint64 titleId);

	cpp3ds::Uint64 getTitleId() const;

	void setUpdateStatus(cpp3ds::Uint64 titleId, bool installed);
	void setDLCStatus(cpp3ds::Uint64 titleId, bool installed);
	bool getUpdateStatus(cpp3ds::Uint64 titleId) { m_updates[titleId]; }
	bool getDLCStatus(cpp3ds::Uint64 titleId) { m_dlc[titleId]; }
	bool hasUpdates() { return !m_updates.empty(); }
	bool hasDLC() { return !m_dlc.empty(); }

	std::shared_ptr<AppItem> getAppItem() const;

	void setHeight(float height);
	float getHeight() const;

	std::map<cpp3ds::Uint64, bool> &getUpdates() { return m_updates; };
	std::map<cpp3ds::Uint64, bool> &getDLC() { return m_dlc; };

	void processEvent(const cpp3ds::Event& event);

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;
	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);

private:
	float m_height;
	cpp3ds::Uint64 m_titleId;
	gui3ds::NinePatch m_background;

	std::map<cpp3ds::Uint64, bool> m_updates;
	std::map<cpp3ds::Uint64, bool> m_dlc;
	size_t m_updateInstallCount;
	size_t m_dlcInstallCount;

	cpp3ds::Text m_textTitle;
	util3ds::TweenText m_textWarningUpdate;
	util3ds::TweenText m_textWarningDLC;

	std::shared_ptr<AppItem> m_appItem;
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDITEM_HPP
