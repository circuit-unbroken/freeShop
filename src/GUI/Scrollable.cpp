#include "Scrollable.hpp"
#include "ScrollBar.hpp"

namespace FreeShop {

Scrollable::Scrollable()
: m_scrollBar(nullptr)
, m_scrollVal(0.f)
{

}

Scrollable::~Scrollable()
{
	if (m_scrollBar)
		m_scrollBar->detachObject(this);
}

void Scrollable::attachScrollbar(ScrollBar *scrollbar)
{
	m_scrollBar = scrollbar;
}

void Scrollable::detachScrollbar()
{
	m_scrollBar = nullptr;
}

void Scrollable::updateScrollSize()
{
	if (m_scrollBar)
		m_scrollBar->markDirty();
}


} // namespace FreeShop
