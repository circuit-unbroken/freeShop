#ifndef FREESHOP_ICONSET_HPP
#define FREESHOP_ICONSET_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "AppItem.hpp"

namespace FreeShop {

class IconSet : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int ALPHA = 11;

	IconSet();
	~IconSet();

	bool processEvent(const cpp3ds::Event& event);
	void update(float delta);

	void addIcon(const cpp3ds::String &string);

	void setSelectedIndex(int index);
	int getSelectedIndex() const;

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;
	void resize();

private:
	int m_selectedIndex;
	float m_padding;
	std::vector<util3ds::TweenText> m_icons;

	TweenEngine::TweenManager m_tweenManager;
};

} // namepace FreeShop


#endif // FREESHOP_ICONSET_HPP
