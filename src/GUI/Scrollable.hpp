#ifndef FREESHOP_SCROLLABLE_HPP
#define FREESHOP_SCROLLABLE_HPP

#include <cpp3ds/System/Vector2.hpp>

namespace FreeShop {

class ScrollBar;

class Scrollable {
friend class ScrollBar;
public:
	Scrollable();
	~Scrollable();
	virtual void setScroll(float position) = 0;
	virtual float getScroll() = 0;
	virtual const cpp3ds::Vector2f &getScrollSize() = 0;

protected:
	void attachScrollbar(ScrollBar *scrollbar);
	void detachScrollbar();
	void updateScrollSize();

private:
	float m_scrollVal;
	ScrollBar *m_scrollBar;
};

} // namespace FreeShop

#endif // FREESHOP_SCROLLABLE_HPP
