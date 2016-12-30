#ifndef FREESHOP_QRSCANNERSTATE_HPP
#define FREESHOP_QRSCANNERSTATE_HPP

extern "C" {
#include <quirc.h>
}
#ifdef _3DS
#include <3ds.h>
#include <cpp3ds/System/LinearAllocator.hpp>
#endif
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/System/Mutex.hpp>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/Graphics/RectangleShape.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <TweenEngine/TweenManager.h>
#include "State.hpp"
#include "../TweenObjects.hpp"
#include "../GUI/NinePatch.hpp"

namespace FreeShop {

class QrScannerState: public State {
public:
	QrScannerState(StateStack& stack, Context& context, StateCallback callback);
	~QrScannerState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	void scanQrCode();
	void startCamera();
	void close();

private:
	cpp3ds::Thread m_camThread;
	cpp3ds::Mutex m_camMutex;
	std::vector<cpp3ds::Uint8> m_camBuffer;
#ifdef _3DS
	std::vector<cpp3ds::Uint8, cpp3ds::LinearAllocator<cpp3ds::Uint8>> m_textureBuffer;
	std::vector<cpp3ds::Uint8, cpp3ds::LinearAllocator<cpp3ds::Uint8>> m_rawTextureBuffer;
#endif
	bool m_capturing;
	bool m_threadRunning;
	bool m_requestedClose;
	cpp3ds::Mutex m_mutexRequest;
	cpp3ds::Mutex m_mutexDraw;

	bool m_displayError;
	bool m_hideQrBorder;
	cpp3ds::Text m_textError;
	cpp3ds::Text m_textCloseError;

	util3ds::TweenRectangleShape m_cameraScreen;
	cpp3ds::Texture m_cameraTexture;
	util3ds::TweenNinePatch m_qrBorder;

	cpp3ds::Text m_closeCaption;
	cpp3ds::RectangleShape m_bottomBackground;

	TweenEngine::TweenManager m_tweenManager;

	struct quirc *m_qr;
};

} // namespace FreeShop

#endif //FREESHOP_QRSCANNERSTATE_HPP
