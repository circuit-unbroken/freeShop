#ifndef FREESHOP_GUI_APPITEM_HPP
#define FREESHOP_GUI_APPITEM_HPP

#include <memory>
#include "../TweenObjects.hpp"
#include "../AppItem.hpp"
#include "NinePatch.hpp"

namespace FreeShop {

	namespace GUI {

		class AppItem : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
		public:
			static const int INFO_ALPHA = 11;
			static const int BACKGROUND_ALPHA = 12;

			AppItem();

			~AppItem();

			void setAppItem(std::shared_ptr<::FreeShop::AppItem> appItem);
			std::shared_ptr<::FreeShop::AppItem> getAppItem() const;

			void setSize(float width, float height);

			const cpp3ds::Vector2f &getSize() const;

			void setVisible(bool visible);
			bool isVisible() const;
			void setFilteredOut(bool filteredOut);
			bool isFilteredOut() const;

			void setInfoVisible(bool visible);
			bool isInfoVisible() const;

			void select();

			void deselect();

			void setMatchTerm(const std::string &string);
			int getMatchScore() const;

		protected:
			virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

			virtual void setValues(int tweenType, float *newValues);
			virtual int getValues(int tweenType, float *returnValues);

			void addRegionFlag(::FreeShop::AppItem::Region region);
			void setTitle(const cpp3ds::String &title);
			void setFilesize(cpp3ds::Uint64 filesize);

		private:
			std::shared_ptr<::FreeShop::AppItem> m_appItem;

			gui3ds::NinePatch m_background;

			cpp3ds::RectangleShape m_icon;
			cpp3ds::RectangleShape m_progressBar;

			cpp3ds::Text m_titleText;
			cpp3ds::Text m_filesizeText;

			cpp3ds::Vector2f m_size;

			std::vector<cpp3ds::Sprite> m_regionFlags;

			int m_matchScore;

			bool m_infoVisible;
			mutable bool m_visible;
			mutable bool m_filteredOut;
		};

	} // namespace GUI

} // namespace FreeSpace


#endif // FREESHOP_GUI_APPITEM_HPP
