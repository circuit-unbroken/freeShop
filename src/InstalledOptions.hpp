#ifndef FREESHOP_INSTALLEDOPTIONS_HPP
#define FREESHOP_INSTALLEDOPTIONS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "States/BrowseState.hpp"
#include "TitleKeys.hpp"

namespace FreeShop {

class InstalledItem;

class InstalledOptions : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int ALPHA = 11;

	InstalledOptions();

	void processTouchEvent(const cpp3ds::Event& event);

	void setInstalledItem(InstalledItem *installedItem);
	InstalledItem *getInstalledItem() const;

	void update();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);
private:
	cpp3ds::Text m_textGame;
	cpp3ds::Text m_textUpdates;
	cpp3ds::Text m_textDLC;

	cpp3ds::Text m_textIconGame;
	cpp3ds::Text m_textIconUpdates;
	cpp3ds::Text m_textIconDLC;

	InstalledItem *m_installedItem;
	bool m_updatesAvailable;
	bool m_dlcAvailable;
	bool m_updatesInstalled;
	bool m_dlcInstalled;

	TitleKeys::TitleType m_titleType;
#ifdef _3DS
	FS_MediaType m_mediaType;
#endif
};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDOPTIONS_HPP
