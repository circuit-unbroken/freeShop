#ifndef FREESHOP_APPINFO_HPP
#define FREESHOP_APPINFO_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include "AppItem.hpp"

namespace FreeShop {

class AppInfo : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	static const int ALPHA = 11;

	AppInfo();
	~AppInfo();

	void drawTop(cpp3ds::Window &window);

	bool processEvent(const cpp3ds::Event& event);
	void update(float delta);

	void loadApp(std::shared_ptr<AppItem> appItem);
	const std::shared_ptr<AppItem> getAppItem() const;

	void setCurrentScreenshot(int screenshotIndex);

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);

	void updateInfo();

private:
	void setDescription(const rapidjson::Value &jsonDescription);
	void setScreenshots(const rapidjson::Value &jsonScreenshots);
	void addScreenshot(int index, const rapidjson::Value &jsonScreenshot);

	cpp3ds::Sprite m_icon;
	cpp3ds::Text m_textTitle;
	cpp3ds::Text m_textDescription;
	cpp3ds::Text m_textTitleId;
	float m_descriptionVelocity;
	cpp3ds::RectangleShape m_fadeTextRect;

	bool m_isDemoInstalled;
	cpp3ds::Text m_textDemo;
	cpp3ds::Text m_textIconDemo;

	util3ds::TweenText m_textDownload;
	util3ds::TweenText m_textDelete;
	util3ds::TweenText m_textExecute;

	std::vector<std::unique_ptr<cpp3ds::Texture>> m_screenshotTopTextures;
	std::vector<std::unique_ptr<cpp3ds::Texture>> m_screenshotBottomTextures;
	std::vector<std::unique_ptr<cpp3ds::Sprite>> m_screenshotTops;
	std::vector<std::unique_ptr<cpp3ds::Sprite>> m_screenshotBottoms;

	int m_currentScreenshot;
	util3ds::TweenSprite m_screenshotTopTop;
	util3ds::TweenSprite m_screenshotTopBottom;
	util3ds::TweenSprite m_screenshotBottom;
	util3ds::TweenText m_arrowLeft;
	util3ds::TweenText m_arrowRight;
	util3ds::TweenText m_close;

	cpp3ds::RectangleShape m_screenshotsBackground;
	cpp3ds::Text m_textScreenshotsEmpty;

	util3ds::TweenText m_textNothingSelected;
	cpp3ds::RectangleShape m_fadeRect;

	std::shared_ptr<AppItem> m_appItem;

	TweenEngine::TweenManager m_tweenManager;
};

} // namepace FreeShop


#endif // FREESHOP_APPINFO_HPP
