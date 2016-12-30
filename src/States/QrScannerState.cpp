#include "QrScannerState.hpp"
#include "../AssetManager.hpp"
#include <iostream>
#include <cpp3ds/System/Sleep.hpp>
#include <cpp3ds/System/Lock.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cmath>
#include <TweenEngine/Tween.h>

#define WIDTH 400
#define HEIGHT 240
#define EVENT_RECV 0
#define EVENT_BUFFER_ERROR 1
//#define EVENT_CANCEL 2

#define EVENT_COUNT 2

#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

namespace FreeShop {

QrScannerState::QrScannerState(StateStack &stack, Context &context, StateCallback callback)
	: State(stack, context, callback)
	, m_camThread(&QrScannerState::startCamera, this)
	, m_capturing(false)
	, m_threadRunning(false)
	, m_requestedClose(false)
	, m_displayError(false)
	, m_hideQrBorder(false)
{
	m_cameraScreen.setTexture(&m_cameraTexture);
	m_cameraScreen.setSize(cpp3ds::Vector2f(WIDTH, HEIGHT));
	m_cameraScreen.setTextureRect(cpp3ds::IntRect(0, 0, WIDTH, HEIGHT));

	cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/qr_selector.9.png");
	texture.setSmooth(false);
	m_qrBorder.setTexture(&texture);
	m_qrBorder.setContentSize(140.f, 140.f);
	m_qrBorder.setPosition(200.f, 120.f);
	m_qrBorder.setColor(cpp3ds::Color(255, 255, 255, 100));
	m_qrBorder.setOrigin(std::round(m_qrBorder.getSize().x * 0.5f), std::round(m_qrBorder.getSize().y * 0.5f));
	TweenEngine::Tween::to(m_qrBorder, m_qrBorder.SCALE_XY, 0.3f)
		.target(0.95f, 0.95f)
		.repeatYoyo(-1, 0.f)
		.start(m_tweenManager);

	m_bottomBackground.setSize(cpp3ds::Vector2f(320.f, 240.f));

	m_closeCaption.setString(_("\uE001 Cancel"));
	m_closeCaption.setFillColor(cpp3ds::Color(150, 150, 150));
	m_closeCaption.setCharacterSize(20);
	m_closeCaption.setPosition(160.f, 200.f);
	m_closeCaption.useSystemFont();
	m_closeCaption.setOrigin(m_closeCaption.getLocalBounds().width / 2,
	                         m_closeCaption.getLocalBounds().height / 2);

	m_textCloseError = m_closeCaption;
	m_textCloseError.setString(_("\uE000 Retry"));
	m_textCloseError.setPosition(160.f, 200.f);

	m_textError = m_closeCaption;
	m_textError.setPosition(160.f, 120.f);
	m_textError.setCharacterSize(16);

	m_qr = quirc_new();
	quirc_resize(m_qr, WIDTH, HEIGHT);

	m_camThread.setRelativePriority(4);
	m_camThread.launch();
}

QrScannerState::~QrScannerState()
{
	while (m_threadRunning && !m_capturing)
		cpp3ds::sleep(cpp3ds::milliseconds(10));
	m_displayError = false;
	m_capturing = false;
	m_camThread.wait();
	quirc_destroy(m_qr);
}

void QrScannerState::renderTopScreen(cpp3ds::Window& window)
{
	cpp3ds::Lock lock(m_mutexDraw);
#ifdef _3DS
	if (m_textureBuffer.empty())
		return;
	u32 dimTexture = GX_BUFFER_DIM(512, 256);
	GSPGPU_FlushDataCache(&m_rawTextureBuffer[0], m_rawTextureBuffer.size());
	GX_DisplayTransfer((u32*)&m_rawTextureBuffer[0], dimTexture, (u32*)&m_textureBuffer[0], dimTexture, TEXTURE_TRANSFER_FLAGS);
	gspWaitForPPF();
	GSPGPU_FlushDataCache(&m_textureBuffer[0], m_textureBuffer.size());
#endif
	window.draw(m_cameraScreen);

	if (!m_displayError && !m_hideQrBorder)
		window.draw(m_qrBorder);
}

void QrScannerState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_bottomBackground);
	if (m_displayError)
	{
		window.draw(m_textError);
		window.draw(m_textCloseError);
	}
	else
		window.draw(m_closeCaption);
}

bool QrScannerState::update(float delta)
{
	m_tweenManager.update(delta);
	return false;
}

bool QrScannerState::processEvent(const cpp3ds::Event& event)
{
	if (event.type == cpp3ds::Event::KeyPressed)
	{
		if (m_displayError)
		{
			if (event.key.code == cpp3ds::Keyboard::A)
				m_displayError = false;
		}
		else
		{
			if (event.key.code == cpp3ds::Keyboard::B)
				close();
		}
	}
	return false;
}

void QrScannerState::startCamera()
{
	cpp3ds::Thread qrThread(&QrScannerState::scanQrCode, this);
	qrThread.setRelativePriority(1);
	qrThread.setStackSize(64 * 1024);

	m_threadRunning = true;
#ifdef _3DS
	Handle events[EVENT_COUNT] = {0};
//	events[EVENT_CANCEL] = data->cancelEvent;
	std::vector<cpp3ds::Uint8> tmpCamBuffer;
	size_t qrCounter = 0;
	Result res = 0;

	int w = WIDTH, h = HEIGHT;
	u32 dimCam = GX_BUFFER_DIM(w, h);
	m_camBuffer.resize(w * h * sizeof(u16));
	tmpCamBuffer.resize(w * h * sizeof(u16));

	int npow2w = 512, npow2h = 256;
	m_textureBuffer.resize(npow2w * npow2h * sizeof(u16));
	m_rawTextureBuffer.resize(m_textureBuffer.size());
	m_cameraTexture.loadFromPreprocessedMemory(&m_textureBuffer[0], m_textureBuffer.size(), npow2w, npow2h, GPU_RGB565, false);

	if(R_SUCCEEDED(res = camInit())) {
		if(R_SUCCEEDED(res = CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A))
		   && R_SUCCEEDED(res = CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A))
		   && R_SUCCEEDED(res = CAMU_SetFrameRate(SELECT_OUT1, FRAME_RATE_30))
		   && R_SUCCEEDED(res = CAMU_SetNoiseFilter(SELECT_OUT1, true))
		   && R_SUCCEEDED(res = CAMU_SetAutoExposure(SELECT_OUT1, true))
		   && R_SUCCEEDED(res = CAMU_SetAutoWhiteBalance(SELECT_OUT1, true))
		   && R_SUCCEEDED(res = CAMU_Activate(SELECT_OUT1))) {
			u32 transferUnit = 0;

			if(R_SUCCEEDED(res = CAMU_GetBufferErrorInterruptEvent(&events[EVENT_BUFFER_ERROR], PORT_CAM1))
			   && R_SUCCEEDED(res = CAMU_SetTrimming(PORT_CAM1, false))
			   && R_SUCCEEDED(res = CAMU_GetMaxBytes(&transferUnit, w, h))
			   && R_SUCCEEDED(res = CAMU_SetTransferBytes(PORT_CAM1, transferUnit, w, h))
			   && R_SUCCEEDED(res = CAMU_ClearBuffer(PORT_CAM1))
			   && R_SUCCEEDED(res = CAMU_SetReceiving(&events[EVENT_RECV], &tmpCamBuffer[0], PORT_CAM1, tmpCamBuffer.size(), (s16) transferUnit))
			   && R_SUCCEEDED(res = CAMU_StartCapture(PORT_CAM1)))
			{
				m_capturing = true;
				qrThread.launch();
				while (m_capturing && R_SUCCEEDED(res))
				{
//					svcWaitSynchronization(task_get_pause_event(), U64_MAX);

					s32 index = 0;
					if(R_SUCCEEDED(res = svcWaitSynchronizationN(&index, events, EVENT_COUNT, false, U64_MAX))) {
						switch(index) {
//							case EVENT_CANCEL:
//								m_capturing = false;
//								break;
							case EVENT_RECV:
								svcCloseHandle(events[EVENT_RECV]);
								events[EVENT_RECV] = 0;
								{
									cpp3ds::Lock lock(m_camMutex);
									m_camBuffer = tmpCamBuffer;
								}
								{
									cpp3ds::Lock lock(m_mutexDraw);
									for (int i = 0; i < h; ++i)
										memcpy(&m_rawTextureBuffer[i * npow2w * 2], &tmpCamBuffer[i * w * 2], w * 2);
								}
								res = CAMU_SetReceiving(&events[EVENT_RECV], &tmpCamBuffer[0], PORT_CAM1, tmpCamBuffer.size(), (s16) transferUnit);
								break;
							case EVENT_BUFFER_ERROR:
								cpp3ds::err() << "CAM ERROR" << std::endl;
								svcCloseHandle(events[EVENT_RECV]);
								events[EVENT_RECV] = 0;

								if(R_SUCCEEDED(res = CAMU_ClearBuffer(PORT_CAM1))
								   && R_SUCCEEDED(res = CAMU_SetReceiving(&events[EVENT_RECV], &tmpCamBuffer[0], PORT_CAM1, tmpCamBuffer.size(), (s16) transferUnit))) {
									res = CAMU_StartCapture(PORT_CAM1);
								}

								break;
							default:
								break;
						}
					}
				}

				CAMU_StopCapture(PORT_CAM1);

				bool busy = false;
				while (R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy)
					cpp3ds::sleep(cpp3ds::milliseconds(10));

				CAMU_ClearBuffer(PORT_CAM1);
			}
			CAMU_Activate(SELECT_NONE);
		}
		camExit();
	}
#endif
	m_threadRunning = false;
	m_capturing = false;
	qrThread.wait();
}

void QrScannerState::scanQrCode()
{
	int w, h;
	std::vector<cpp3ds::Uint8> camBuffer;

	while (m_capturing && !m_requestedClose)
	{
		{
			cpp3ds::Lock lock(m_camMutex);
			camBuffer = m_camBuffer;
		}
		cpp3ds::Uint16 *pixels = reinterpret_cast<cpp3ds::Uint16*>(&camBuffer[0]);

		uint8_t *buf = quirc_begin(m_qr, &w, &h);
		for (int x = 0; x < w; x++)
			for (int y = 0; y < h; y++)
			{
				cpp3ds::Uint16 px = pixels[y * WIDTH + x];
				buf[y * w + x] = (uint8_t) (((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
			}
		quirc_end(m_qr);

		int count = quirc_count(m_qr);
		for (int i = 0; i < count; i++)
		{
			struct quirc_code code;
			struct quirc_data data;

			quirc_extract(m_qr, i, &code);
			if (!quirc_decode(&code, &data))
			{
				// Return QR's decoded text to callback, outputs error string when true
				cpp3ds::String text(reinterpret_cast<char*>(data.payload));

				m_hideQrBorder = true;
				TweenEngine::Tween::to(m_cameraScreen, m_cameraScreen.FILL_COLOR_ALPHA, 0.4f)
					.target(100.f)
					.start(m_tweenManager);

				if (runCallback(&text))
					close();
				else
				{
					m_textError.setString(text);
					m_textError.setOrigin(m_textError.getLocalBounds().width / 2,
										  m_textError.getLocalBounds().height / 2);
					m_displayError = true;
				}

				m_hideQrBorder = false;
			}
		}
		while (m_displayError)
			cpp3ds::sleep(cpp3ds::milliseconds(100));

		TweenEngine::Tween::to(m_cameraScreen, m_cameraScreen.FILL_COLOR_ALPHA, 0.4f)
			.target(255.f)
			.start(m_tweenManager);
	}
}

void QrScannerState::close()
{
	cpp3ds::Lock lock(m_mutexRequest);
	if (!m_requestedClose)
	{
		m_requestedClose = true;
		requestStackPop();
	}
}


} // namespace FreeShop
