#include "PPUI.h"
#include <cstdio>
#include "ConfigManager.h"

volatile u32 kDown;
volatile u32 kHeld;
volatile u32 kUp;

volatile u32 last_kDown;
volatile u32 last_kHeld;
volatile u32 last_kUp;

static circlePosition cPos;
static circlePosition cStick;
static touchPosition kTouch;
static touchPosition last_kTouch;
static touchPosition first_kTouchDown;
static touchPosition last_kTouchDown;
volatile u64 holdTime = 0;

static u32 sleepModeState = 0;

static bool mTmpLockTouch = false;
static PopupCallback *mDialogBox;
static std::vector<PopupCallback> mPopupList;
static std::string mTemplateInputString = "";
static ServerConfig* mTmpServerConfig = nullptr;

static const char* UI_INPUT_VALUE[] = { "1", "2", "3", 
										"4", "5", "6", 
										"7", "8", "9", 
										".", "0", ":" };

static const char* UI_KEYBOARD_VALUE[] = { 
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "'",
	"z", "x", "c", "v", "b", "n", "m", ",", ".", "?",
};

u32 PPUI::getKeyDown()
{
	return kDown;
}

u32 PPUI::getKeyHold()
{
	return kHeld;
}

u32 PPUI::getKeyUp()
{
	return kUp;
}

circlePosition PPUI::getLeftCircle()
{
	return cPos;
}

circlePosition PPUI::getRightCircle()
{
	return cStick;
}

u32 PPUI::getSleepModeState()
{
	return sleepModeState;
}

void PPUI::UpdateInput()
{
	//----------------------------------------
	// store old input
	last_kDown = kDown;
	last_kHeld = kHeld;
	last_kUp = kUp;
	last_kTouch = kTouch;
	//----------------------------------------
	// scan new input
	kDown = hidKeysDown();
	kHeld = hidKeysHeld();
	kUp = hidKeysUp();
	cPos = circlePosition();
	hidCircleRead(&cPos);
	cStick = circlePosition();
	irrstCstickRead(&cStick);
	kTouch = touchPosition();
	hidTouchRead(&kTouch);

	if(kDown & KEY_TOUCH)
	{
		first_kTouchDown = kTouch;
		last_kTouchDown = touchPosition();
	}
	if(last_kHeld & KEY_TOUCH && kUp & KEY_TOUCH)
	{
		last_kTouchDown = kTouch;
		first_kTouchDown = touchPosition();
		holdTime = 0;
	}
}

bool PPUI::TouchDownOnArea(float x, float y, float w, float h)
{
	if (kDown & KEY_TOUCH || kHeld & KEY_TOUCH)
	{
		if (kTouch.px >= (u16)x && kTouch.px <= (u16)(x + w) && kTouch.py >= (u16)y && kTouch.py <= (u16)(y + h))
		{
			return true;
		}
	}
	return false;
}

bool PPUI::TouchUpOnArea(float x, float y, float w, float h)
{
	if ((last_kDown & KEY_TOUCH || last_kHeld & KEY_TOUCH) && kUp & KEY_TOUCH)
	{
		if (last_kTouch.px >= (u16)x && last_kTouch.px <= (u16)(x + w) && last_kTouch.py >= (u16)y && last_kTouch.py <= (u16)(y + h))
		{
			return true;
		}
	}
	return false;
}

bool PPUI::TouchDown()
{
	return kDown & KEY_TOUCH;
}

bool PPUI::TouchMove()
{
	return kHeld & KEY_TOUCH;
}

bool PPUI::TouchUp()
{
	return last_kHeld & KEY_TOUCH && kUp & KEY_TOUCH;
}

///////////////////////////////////////////////////////////////////////////
// TEXT
///////////////////////////////////////////////////////////////////////////

int PPUI::DrawIdleTopScreen(PPSessionManager* sessionManager)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 400, 240, rgb(26, 188, 156));
	LabelBox(0, 0, 400, 240, "PinBox", rgb(26, 188, 156), rgb(255, 255, 255));
}

int PPUI::DrawNumberInputScreen(const char* label, ResultCallback cancel, ResultCallback ok)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 80, rgb(26, 188, 156));

	// Screen title
	LabelBox(0, 5, 320, 30, label, rgb(26, 188, 156), rgb(255, 255, 255));

	// Input display
	LabelBox(20, 40, 280, 30, mTemplateInputString.c_str(), rgb(236, 240, 241), rgb(44, 62, 80));

	// Number pad
	for(int c = 0; c < 3; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			if(FlatButton(10 + c * 35, 90 + r * 35, 30, 30, UI_INPUT_VALUE[c + r * 3]))
			{
				char v = *UI_INPUT_VALUE[c + r * 3];
				mTemplateInputString.push_back(v);
			}
		}
	}

	// Delete button
	if (FlatDarkButton(120, 90, 40, 30, "DEL"))
	{
		if (mTemplateInputString.size() > 0)
		{
			mTemplateInputString.erase(mTemplateInputString.end() - 1);
		}
	}

	// Clear button
	if (FlatDarkButton(120, 125, 40, 30, "CLR"))
	{
		mTemplateInputString = "";
	}

	// Cancel button
	if (FlatColorButton(200, 200, 50, 30, "Cancel", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (cancel != nullptr) cancel(nullptr, nullptr);
	}

	// OK button
	if (FlatColorButton(260, 200, 50, 30, "OK", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (ok != nullptr) ok(nullptr, nullptr);
	}

	return 0;
}

int PPUI::DrawBtmServerSelectScreen(PPSessionManager* sessionManager)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 35, rgb(26, 188, 156));

	// Screen title
	switch (sessionManager->GetManagerState()) {
	case -1: LabelBox(0, 5, 255, 25, "Status: No Wifi Connection", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 0: LabelBox(0, 5, 255, 25, "Status: Ready to Connect", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 1: LabelBox(0, 5, 255, 25, "Status: Connecting...", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 2: LabelBox(0, 5, 255, 25, "Status: Connected", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	}

	// Add new Server
	if (FlatColorButton(260, 5, 50, 25, "Add", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		AddPopup([=]()
		{
			if(mTmpServerConfig == nullptr)
			{
				mTmpServerConfig = new ServerConfig();
				mTmpServerConfig->name = "My Local PC";
				mTmpServerConfig->ip = "127.0.0.1";
				mTmpServerConfig->port = "1234";
			}
			
			return DrawBtmAddNewServerProfileScreen(sessionManager,
				[=](void* a, void* b)
				{
					delete mTmpServerConfig;
					mTmpServerConfig = nullptr;
					// on cancel
				},
				[=](void* a, void* b)
				{
					// add to list
					ConfigManager::Get()->servers.push_back(ServerConfig(*mTmpServerConfig));
					ConfigManager::Get()->Save(); //TODO: error here when save

					delete mTmpServerConfig;
					mTmpServerConfig = nullptr;
					// on ok
				}
			);
		});
	}

	// List servers
	if(ConfigManager::Get()->servers.size() > 0)
	{
		//TODO: scroll box
		float boxHeight = 40;
		for(int i = 0; i < ConfigManager::Get()->servers.size(); ++i)
		{
			float sx = 40 + boxHeight * i + 5 * i;
			// Draw BG
			PPGraphics::Get()->DrawRectangle(0, sx, 320, boxHeight, rgb(223, 228, 234));

			// Server name
			LabelBoxLeft(70, sx + 2, 150, 20, ConfigManager::Get()->servers[i].name.c_str(), transparent, rgb(47, 53, 66), 0.7);
			LabelBoxLeft(70, sx + 25, 150, 10, (ConfigManager::Get()->servers[i].ip + ":" + ConfigManager::Get()->servers[i].port).c_str(), transparent, rgb(116, 125, 140), 0.45);

			// Connect button
			if (FlatColorButton(5, sx + 2, 60, 32, "Connect", rgb(46, 213, 115), rgb(123, 237, 159), rgb(47, 53, 66)))
			{

			}

			// Remove button
			if (FlatColorButton(286, sx + 6, 26, 26, "X", rgb(255, 71, 87), rgb(255, 107, 129), rgb(47, 53, 66)))
			{
				DrawDialogMessage(sessionManager, "Warning", "Are you sure to remove this profile?", [=](void* a, void* b)
				{
					// on cancel
				}, [=](void* a, void* b)
				{
					// on ok
					ConfigManager::Get()->servers.erase(ConfigManager::Get()->servers.begin() + i);
					return 0;
				});
			}
		}

	}else
	{
		// Draw empty screen and tutorial
		LabelBoxAutoWrap(10, 45, 300, 185, "No server profile found.\nPlease add new one by Add button.", transparent, PPGraphics::Get()->PrimaryTextColor);
	}


	// Dialog box ( alway at bottom so it will draw on top )
	DrawDialogBox(sessionManager);
	return 0;
}

int PPUI::DrawBtmAddNewServerProfileScreen(PPSessionManager* sessionManager, ResultCallback cancel, ResultCallback ok)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 25, rgb(26, 188, 156));

	// Screen title
	LabelBox(0, 5, 320, 20, "Add New Server", rgb(26, 188, 156), rgb(255, 255, 255));

	// Input server name
	LabelBoxLeft(5, 30, 100, 30, "Server Name", transparent, rgb(44, 62, 80));
	if (LabelBox(105, 30, 210, 30, mTmpServerConfig->name.c_str(), PPGraphics::Get()->PrimaryColor, rgb(255, 255, 255)))
	{
		mTemplateInputString = std::string(mTmpServerConfig->name);
		DrawDialogKeyboard([=](void* a, void* b) {},
		[=](void* a, void* b)
		{
			// ok
			mTmpServerConfig->name.clear();
			mTmpServerConfig->name.append(mTemplateInputString);
			mTemplateInputString = "";
		});
	}

	// Input server ip
	LabelBoxLeft(5, 70, 100, 30, "Server IP", transparent, rgb(44, 62, 80));
	if (LabelBox(105, 70, 210, 30, mTmpServerConfig->ip.c_str(), PPGraphics::Get()->PrimaryColor, rgb(255, 255, 255)))
	{
		mTemplateInputString = std::string(mTmpServerConfig->ip);
		DrawDialogNumberInput([=](void* a, void* b) {},
			[=](void* a, void* b)
		{
			// ok
			mTmpServerConfig->ip.clear();
			mTmpServerConfig->ip.append(mTemplateInputString);
			mTemplateInputString = "";
		});
	}


	// Input server port
	LabelBoxLeft(5, 110, 100, 30, "Server Port", transparent, rgb(44, 62, 80));
	if (LabelBox(105, 110, 210, 30, mTmpServerConfig->port.c_str(), PPGraphics::Get()->PrimaryColor, rgb(255, 255, 255)))
	{
		mTemplateInputString = std::string(mTmpServerConfig->port);
		DrawDialogNumberInput([=](void* a, void* b) {},
			[=](void* a, void* b)
		{
			// ok
			mTmpServerConfig->port.clear();
			mTmpServerConfig->port.append(mTemplateInputString);
			mTemplateInputString = "";
		});
	}


	// Cancel button
	if (FlatColorButton(200, 200, 50, 30, "Cancel", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (cancel != nullptr) cancel(nullptr, nullptr);
	}

	// OK button
	if (FlatColorButton(260, 200, 50, 30, "OK", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (ok != nullptr) ok(nullptr, nullptr);
	}

	// Dialog box ( alway at bottom so it will draw on top )
	DrawDialogBox(sessionManager);

	return 0;
}


int PPUI::DrawBottomScreenUI(PPSessionManager* sessionManager)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 80, rgb(26, 188, 156));

	// Screen title
	switch (sessionManager->GetManagerState()) {
	case -1: LabelBox(0, 0, 320, 20, "Status: No Wifi Connection", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 0: LabelBox(0, 0, 320, 20, "Status: Ready to Connect", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 1: LabelBox(0, 0, 320, 20, "Status: Connecting...", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	case 2: LabelBox(0, 0, 320, 20, "Status: Connected", rgb(26, 188, 156), rgb(255, 255, 255)); break;
	}

	// IP Port
	LabelBox(20, 40, 230, 30, sessionManager->getIPAddress(), rgb(236, 240, 241), rgb(44, 62, 80));

	// Edit Button
	if (FlatColorButton(260, 40, 50, 30, "Edit", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		if (sessionManager->GetManagerState() == 2) return 0;

		mTemplateInputString = std::string(sessionManager->getIPAddress());
		AddPopup([=]()
		{
			return DrawNumberInputScreen("Enter your IP and port", 
				[=](void* a, void* b)
				{
					// cancel
					mTemplateInputString = "";
				},
				[=](void* a, void* b)
				{
					// ok
					sessionManager->setIPAddress(mTemplateInputString.c_str());
					//ConfigManager::Get()->_cfg_ip = strdup(mTemplateInputString.c_str());
					//ConfigManager::Get()->Save();
					mTemplateInputString = "";
				}
			);
		});
	}

	// Tab Button

	// Tab Content

	if (FlatColorButton(260, 90, 50, 30, "Start", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		if (sessionManager->GetManagerState() == 2) return 0;
		sessionManager->StartStreaming(sessionManager->getIPAddress());
	}

	// Sleep mode
	if (FlatColorButton(200, 90, 50, 30, "Sleep", rgb(39, 174, 96), rgb(46, 204, 113), rgb(255, 255, 255)))
	{
		if (sleepModeState == 1) sleepModeState = 0;
	}


	// Config mode
	if (FlatColorButton(10, 90, 120, 30, "> Adv. Config", rgb(39, 174, 96), rgb(46, 204, 113), rgb(255, 255, 255)))
	{
		AddPopup([=]()
		{
			return DrawStreamConfigUI(sessionManager,
				[=](void* a, void* b)
			{
				// cancel
			},
				[=](void* a, void* b)
			{
				// ok
				// save config
				ConfigManager::Get()->Save();
				// send new setting to server
				sessionManager->UpdateStreamSetting();
			}
			);
		});
	}


	// Exit Button
	if (FlatColorButton(260, 200, 50, 30, "Exit", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		//DrawDialogMessage(sessionManager, "Warning!!!", "There is some shit in here There is some shit in here There is some shit in here There is some shit in here There is some shit in here ");
		return -1;
	}


	InfoBox(sessionManager);


	// Dialog box ( alway at bottom so it will draw on top )
	DrawDialogBox(sessionManager);
	return 0;
}

int PPUI::DrawDialogKeyboard(ResultCallback cancelCallback, ResultCallback okCallback)
{
	if (mDialogBox) return 0;
	mDialogBox = new PopupCallback([=]()
	{
		// draw background
		PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, PPGraphics::Get()->TransBackgroundDark);

		// draw dialog box
		float boxY = 30;
		float boxHeight = 180;
		PPGraphics::Get()->DrawRectangle(13, boxY - 2, 294, boxHeight + 4, PPGraphics::Get()->PrimaryColor);
		PPGraphics::Get()->DrawRectangle(15, boxY, 290, boxHeight, rgb(247, 247, 247));

		// Input display
		LabelBox(22, boxY + 15, 276, 30, mTemplateInputString.c_str(), PPGraphics::Get()->PrimaryColor, rgb(247, 247, 247));

		// Keyboard Number
		int kW = 28;
		float startX = 22, startY = 60 + boxY;
		for (int c = 0; c < 10; c++)
		{
			for (int r = 0; r < 3; r++)
			{
				if (FlatButton(startX + c * kW, startY + r * kW, kW - 4, kW - 4, UI_KEYBOARD_VALUE[c + r * 10]))
				{
					char v = *UI_KEYBOARD_VALUE[c + r * 10];
					mTemplateInputString.push_back(v);
				}
			}
		}

		// Cancel button
		if (FlatColorButton(22, boxY + boxHeight - 30, 56, 25, "Cancel",
			PPGraphics::Get()->AccentColor, PPGraphics::Get()->AccentDarkColor, PPGraphics::Get()->AccentTextColor))
		{
			mTemplateInputString = "";
			cancelCallback(nullptr, nullptr);
			return -1;
		}

		// Delete Button
		if (FlatColorButton(192, boxY + boxHeight - 30, 56, 25, "Delete",
			PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryDarkColor, rgb(247, 247, 247)))
		{
			if (mTemplateInputString.size() > 0)
			{
				mTemplateInputString.erase(mTemplateInputString.end() - 1);
			}
		}

		// OK Button
		if (FlatColorButton(258, boxY + boxHeight - 30, 40, 25, "OK",
			PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryDarkColor, rgb(247, 247, 247)))
		{
			okCallback(nullptr, nullptr);
			return -1;
		}
		
		return 0;
	});
	return 0;
}

int PPUI::DrawDialogNumberInput(ResultCallback cancelCallback, ResultCallback okCallback)
{
	if (mDialogBox) return 0;
	mDialogBox = new PopupCallback([=]()
	{
		// draw background
		PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, PPGraphics::Get()->TransBackgroundDark);

		// draw dialog box
		float boxY = 30;
		float boxHeight = 180;
		PPGraphics::Get()->DrawRectangle(13, boxY - 2, 294, boxHeight + 4, PPGraphics::Get()->PrimaryColor);
		PPGraphics::Get()->DrawRectangle(15, boxY, 290, boxHeight, rgb(247, 247, 247));

		// Input display
		LabelBox(22, boxY + 15, 276, 30, mTemplateInputString.c_str(), PPGraphics::Get()->PrimaryColor, rgb(247, 247, 247));

		// Keyboard Number
		int kW = 28;
		float startX = 22, startY = 60 + boxY;
		for (int c = 0; c < 3; c++)
		{
			for (int r = 0; r < 4; r++)
			{
				if (FlatButton(startX + c * kW, startY + r * kW, kW - 4, kW - 4, UI_INPUT_VALUE[c + r * 3]))
				{
					char v = *UI_INPUT_VALUE[c + r * 3];
					mTemplateInputString.push_back(v);
				}
			}
		}

		// Cancel button
		if (FlatColorButton(238, boxY + boxHeight - 30, 56, 25, "Cancel",
			PPGraphics::Get()->AccentColor, PPGraphics::Get()->AccentDarkColor, PPGraphics::Get()->AccentTextColor))
		{
			mTemplateInputString = "";
			cancelCallback(nullptr, nullptr);
			return -1;
		}

		// Delete Button
		if (FlatColorButton(192, startY, 56, 25, "Delete",
			PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryDarkColor, rgb(247, 247, 247)))
		{
			if (mTemplateInputString.size() > 0)
			{
				mTemplateInputString.erase(mTemplateInputString.end() - 1);
			}
		}

		// OK Button
		if (FlatColorButton(258, startY, 40, 25, "OK",
			PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryDarkColor, rgb(247, 247, 247)))
		{
			okCallback(nullptr, nullptr);
			return -1;
		}

		return 0;
	});
	return 0;
}

int PPUI::DrawDialogMessage(PPSessionManager* sessionManager, const char* title, const char* body)
{
	if (mDialogBox) return 0;
	mDialogBox = new PopupCallback([=]()
	{
		ppVector3 bodySize = PPGraphics::Get()->GetTextSizeAutoWrap(body, 0.5, 0.5, 280);
		float popupHeight = bodySize.y + 40 + 36;
		if (popupHeight > 220) popupHeight = 220;

		float spaceY = (240.0f - popupHeight) / 2.0f;

		// draw background
		PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, PPGraphics::Get()->TransBackgroundDark);

		// draw dialog box
		PPGraphics::Get()->DrawRectangle(13, spaceY, 294, popupHeight + 4, PPGraphics::Get()->PrimaryColor);
		PPGraphics::Get()->DrawRectangle(15, spaceY + 2, 290, popupHeight, rgb(247, 247, 247));

		// draw title
		LabelBox(15, spaceY + 2, 290, 30, title, PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryTextColor, 0.8);
		LabelBoxAutoWrap(20, spaceY + 40, 280, bodySize.y, body, rgb(255, 255, 255), PPGraphics::Get()->PrimaryTextColor);

		// draw button close
		if(FlatColorButton(135, spaceY + 40 + bodySize.y + 4, 50, 30, "Close", 
			PPGraphics::Get()->AccentColor, PPGraphics::Get()->AccentDarkColor, PPGraphics::Get()->AccentTextColor))
		{
			return -1;
		}
		return 0;
	});
	return 0;
}

int PPUI::DrawDialogMessage(PPSessionManager* sessionManager, const char* title, const char* body,
	ResultCallback closeCallback)
{
	if (mDialogBox) return 0;
	mDialogBox = new PopupCallback([=]()
	{
		ppVector3 bodySize = PPGraphics::Get()->GetTextSizeAutoWrap(body, 0.5, 0.5, 280);
		float popupHeight = bodySize.y + 40 + 36;
		if (popupHeight > 220) popupHeight = 220;

		float spaceY = (240.0f - popupHeight) / 2.0f;

		// draw background
		PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, PPGraphics::Get()->TransBackgroundDark);

		// draw dialog box
		PPGraphics::Get()->DrawRectangle(13, spaceY, 294, popupHeight + 4, PPGraphics::Get()->PrimaryColor);
		PPGraphics::Get()->DrawRectangle(15, spaceY + 2, 290, popupHeight, rgb(247, 247, 247));

		// draw title
		LabelBox(15, spaceY + 2, 290, 30, title, PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryTextColor, 0.8);
		LabelBoxAutoWrap(20, spaceY + 40, 280, bodySize.y, body, rgb(255, 255, 255), PPGraphics::Get()->PrimaryTextColor);

		// draw button close
		if (FlatColorButton(135, spaceY + 40 + bodySize.y + 4, 50, 30, "Close",
			PPGraphics::Get()->AccentColor, PPGraphics::Get()->AccentDarkColor, PPGraphics::Get()->AccentTextColor))
		{
			if (closeCallback != nullptr) closeCallback(nullptr, nullptr);
			return -1;
		}
		return 0;
	});
	return 0;
}

int PPUI::DrawDialogMessage(PPSessionManager* sessionManager, const char* title, const char* body,
	ResultCallback cancelCallback, ResultCallback okCallback)
{
	if (mDialogBox) return 0;
	mDialogBox = new PopupCallback([=]()
	{
		ppVector3 bodySize = PPGraphics::Get()->GetTextSizeAutoWrap(body, 0.5, 0.5, 280);
		float popupHeight = bodySize.y + 40 + 36;
		if (popupHeight > 220) popupHeight = 220;

		float spaceY = (240.0f - popupHeight) / 2.0f;

		// draw background
		PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, PPGraphics::Get()->TransBackgroundDark);

		// draw dialog box
		PPGraphics::Get()->DrawRectangle(13, spaceY, 294, popupHeight + 4, PPGraphics::Get()->PrimaryColor);
		PPGraphics::Get()->DrawRectangle(15, spaceY + 2, 290, popupHeight, rgb(247, 247, 247));

		// draw title
		LabelBox(15, spaceY + 2, 290, 30, title, PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryTextColor, 0.8);
		LabelBoxAutoWrap(20, spaceY + 40, 280, bodySize.y, body, rgb(255, 255, 255), PPGraphics::Get()->PrimaryTextColor);

		// draw button close
		if (FlatColorButton(115, spaceY + 40 + bodySize.y + 4, 40, 30, "Close",
			PPGraphics::Get()->AccentColor, PPGraphics::Get()->AccentDarkColor, PPGraphics::Get()->AccentTextColor))
		{
			if (cancelCallback != nullptr) cancelCallback(nullptr, nullptr);
			return -1;
		}

		// draw button ok
		if (FlatColorButton(165, spaceY + 40 + bodySize.y + 4, 40, 30, "OK",
			PPGraphics::Get()->PrimaryColor, PPGraphics::Get()->PrimaryDarkColor, PPGraphics::Get()->PrimaryTextColor))
		{
			if (okCallback != nullptr) okCallback(nullptr, nullptr);
			return -1;
		}
		return 0;
	});
	return 0;
}

int PPUI::DrawStreamConfigUI(PPSessionManager* sessionManager, ResultCallback cancel, ResultCallback ok)
{
	PPGraphics::Get()->DrawRectangle(0, 0, 320, 240, rgb(236, 240, 241));
	LabelBox(0, 0, 320, 30, "Advance Config", rgb(26, 188, 156), rgb(255, 255, 255));


	
	//ConfigManager::Get()->_cfg_video_quality = Slide(5, 40, 300, 30, ConfigManager::Get()->_cfg_video_quality, 10, 100, "Quality");
	//ConfigManager::Get()->_cfg_video_scale = Slide(5, 70, 300, 30, ConfigManager::Get()->_cfg_video_scale, 10, 100, "Scale");
	//ConfigManager::Get()->_cfg_skip_frame = Slide(5, 100, 300, 30, ConfigManager::Get()->_cfg_skip_frame, 0, 60, "Skip Frame");
	//ConfigManager::Get()->_cfg_wait_for_received = ToggleBox(5, 130, 300, 30, ConfigManager::Get()->_cfg_wait_for_received, "Wait Received");


	// Cancel button
	if (FlatColorButton(200, 200, 50, 30, "Cancel", rgb(192, 57, 43), rgb(231, 76, 60), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (cancel != nullptr) cancel(nullptr, nullptr);
	}

	// OK button
	if (FlatColorButton(260, 200, 50, 30, "OK", rgb(41, 128, 185), rgb(52, 152, 219), rgb(255, 255, 255)))
	{
		ClosePopup();
		if (ok != nullptr) ok(nullptr, nullptr);
	}
}

int PPUI::DrawIdleBottomScreen(PPSessionManager* sessionManager)
{
	// touch screen to wake up
	if(TouchUpOnArea(0,0, 320, 240))
	{
		sleepModeState = 1;
	}
	// label
	LabelBox(0, 0, 320, 240, "Touch screen to wake up", rgb(0, 0, 0), rgb(125, 125, 125));

	InfoBox(sessionManager);

	return 0;
}

void PPUI::InfoBox(PPSessionManager* sessionManager)
{
	// render video FPS
	char videoFpsBuffer[100];
	snprintf(videoFpsBuffer, sizeof videoFpsBuffer, "FPS:%.1f|VPS:%.1f", sessionManager->GetFPS(), sessionManager->GetVideoFPS());
	LabelBoxLeft(5, 220, 100, 20, videoFpsBuffer, ppColor{ 0, 0, 0, 0 }, rgb(150, 150, 150), 0.4f);
	snprintf(videoFpsBuffer, sizeof videoFpsBuffer, "CPU:%.1f|GPU:%.1f|CMD:%.1f", C3D_GetProcessingTime()*6.0f, C3D_GetDrawingTime()*6.0f, C3D_GetCmdBufUsage()*100.0f);
	LabelBoxLeft(5, 210, 100, 20, videoFpsBuffer, ppColor{ 0, 0, 0, 0 }, rgb(150, 150, 150), 0.4f);
}

void PPUI::DrawDialogBox(PPSessionManager* sessionManager)
{
	if (mDialogBox != nullptr) {
		mTmpLockTouch = false;
		int ret = (*mDialogBox)();
		mTmpLockTouch = true;
		if(ret == -1)
		{
			mTmpLockTouch = false;
			delete mDialogBox;
			mDialogBox = nullptr;
		}
	}
}


///////////////////////////////////////////////////////////////////////////
// SLIDE
///////////////////////////////////////////////////////////////////////////

float PPUI::Slide(float x, float y, float w, float h, float val, float min, float max, float step, const char* label)
{
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, 0.5f, 0.5f);
	float labelY = (h - tSize.y) / 2.0f;
	float labelX = x + 5.f;
	float slideX = w / 100.f * 35.f;
	float marginY = 2;

	if (val < min) val = min;
	if (val > max) val = max;
	// draw label
	PPGraphics::Get()->DrawText(label, x + labelX, y + labelY, 0.5f, 0.5f, rgb(26, 26, 26), false);

	// draw bg
	float startX = x + slideX;
	float startY = y + marginY;
	w = w - slideX;
	h = h - 2 * marginY;
	PPGraphics::Get()->DrawRectangle(startX, startY, w, h, PPGraphics::Get()->PrimaryDarkColor);
	
	char valBuffer[50];
	snprintf(valBuffer, sizeof valBuffer, "%.1f", val, val);
	ppVector2 valSize = PPGraphics::Get()->GetTextSize(valBuffer, 0.5f, 0.5f);
	float valueX = (w - valSize.x) / 2.0f;
	float valueY = (h - valSize.y) / 2.0f;

	// draw value
	PPGraphics::Get()->DrawText(valBuffer, startX + valueX, startY + valueY, 0.5f, 0.5f, PPGraphics::Get()->AccentTextColor, false);
	float newValue = val;
	// draw plus and minus button
	if(RepeatButton(startX + 1, startY + 1, 30 - 2, h - 2, "<", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80)))
	{
		// minus
		newValue -= step;
	}

	if(RepeatButton(startX + w - 30, startY + 1, 30 - 1, h - 2, ">", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80)))
	{
		// plus
		newValue += step;
	}
	if (newValue < min) newValue = min;
	if (newValue > max) newValue = max;
	return newValue;
}

///////////////////////////////////////////////////////////////////////////
// CHECKBOX
///////////////////////////////////////////////////////////////////////////
bool PPUI::ToggleBox(float x, float y, float w, float h, bool value, const char* label)
{
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, 0.5f, 0.5f);
	float labelY = (h - tSize.y) / 2.0f;
	float labelX = x + 5.f;
	float boxSize = (w / 100.f * 35.f);
	float marginX = w - boxSize;
	float marginY = 2;

	// draw label
	PPGraphics::Get()->DrawText(label, x + labelX, y + labelY, 0.5f, 0.5f, rgb(26, 26, 26), false);

	// draw bg
	float startX = x + marginX;
	float startY = y + marginY;
	w = w - marginX;
	h = h - 2 * marginY;
	PPGraphics::Get()->DrawRectangle(startX, startY, w, h, PPGraphics::Get()->PrimaryDarkColor);

	bool result = value;
	if(value)
	{
		// on button
		FlatColorButton(startX + 1, startY + 1, (boxSize / 2) - 2, h - 2, "On", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80));
		// off button
		if (FlatColorButton(startX + 1 + (boxSize / 2), startY + 1, (boxSize / 2) - 2, h - 2, "Off", PPGraphics::Get()->PrimaryDarkColor, PPGraphics::Get()->PrimaryColor, rgb(236, 240, 241)))
		{
			if(!mTmpLockTouch) result = false;
		}
	}else
	{
		// on button
		if (FlatColorButton(startX + 1, startY + 1, (boxSize / 2) - 2, h - 2, "On", PPGraphics::Get()->PrimaryDarkColor, PPGraphics::Get()->PrimaryColor, rgb(236, 240, 241)))
		{
			if (!mTmpLockTouch) result = true;
		}
		// off button
		FlatColorButton(startX + 1 + (boxSize / 2), startY + 1, (boxSize / 2) - 2, h - 2, "Off", rgb(236, 240, 241), rgb(189, 195, 199), rgb(44, 62, 80));
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
// BUTTON
///////////////////////////////////////////////////////////////////////////

bool PPUI::FlatButton(float x, float y, float w, float h, const char* label)
{
	return FlatColorButton(x, y, w, h, label, rgb(26, 188, 156), rgb(46, 204, 113), rgb(236, 240, 241));
}

bool PPUI::FlatDarkButton(float x, float y, float w, float h, const char* label)
{
	return FlatColorButton(x, y, w, h, label, rgb(22, 160, 133), rgb(39, 174, 96), rgb(236, 240, 241));
}

bool PPUI::FlatColorButton(float x, float y, float w, float h, const char* label, ppColor colNormal, ppColor colActive, ppColor txtCol)
{
	float tScale = 0.5f;
	if (TouchDownOnArea(x, y, w, h) && !mTmpLockTouch)
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colActive);
		tScale = 0.6f;
	}
	else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colNormal);
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	PPGraphics::Get()->DrawText(label, x + (w - tSize.x) / 2.0f, y + (h - tSize.y) / 2.0f, tScale, tScale, txtCol, false);
	return TouchUpOnArea(x, y, w, h) && !mTmpLockTouch;
}

bool PPUI::RepeatButton(float x, float y, float w, float h, const char* label, ppColor colNormal, ppColor colActive, ppColor txtCol)
{
	bool isTouchDown = TouchDownOnArea(x, y, w, h);
	float tScale = 0.5f;
	u64 difTime = 0;
	if (isTouchDown && !mTmpLockTouch)
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colActive);
		tScale = 0.6f;
		
		if (holdTime == 0)
		{
			holdTime = osGetTime();
		}else
		{
			difTime = osGetTime() - holdTime;
		}
	}
	else
	{
		PPGraphics::Get()->DrawRectangle(x, y, w, h, colNormal);
	}
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, tScale, tScale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, tScale, tScale, txtCol, false);

	
	return (isTouchDown && difTime > 500 || TouchUpOnArea(x, y, w, h)) && !mTmpLockTouch;
}

///////////////////////////////////////////////////////////////////////////
// TEXT
///////////////////////////////////////////////////////////////////////////

/**
 * \brief Draw label box
 * \param x 
 * \param y 
 * \param w 
 * \param h 
 * \param defaultValue 
 * \param placeHolder 
 */
int PPUI::LabelBox(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor, float scale)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, scale, scale);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x + startX, y + startY, scale, scale, txtColor, false);


#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "w:%.02f|h:%.02f", tSize.x, tSize.y);
	PPGraphics::Get()->DrawText(buffer, x , y - 10, 0.8f * scale, 0.8f * scale, txtColor, false);
#endif

	return TouchUpOnArea(x, y, w, h) && !mTmpLockTouch;
}

int PPUI::LabelBoxAutoWrap(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor,
	float scale)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector3 tSize = PPGraphics::Get()->GetTextSizeAutoWrap(label, scale, scale, w);
	float startX = (w - tSize.x) / 2.0f;
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawTextAutoWrap(label, x + startX, y + startY, w, scale, scale, txtColor, false);

#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "w:%.02f|h:%.02f|l:%d", tSize.x, tSize.y, tSize.z);
	PPGraphics::Get()->DrawText(buffer, x, y - 10, 0.8f * scale, 0.8f * scale, txtColor, false);
#endif

	return TouchUpOnArea(x, y, w, h) && !mTmpLockTouch;
}

int PPUI::LabelBoxLeft(float x, float y, float w, float h, const char* label, ppColor bgColor, ppColor txtColor, float scale)
{
	PPGraphics::Get()->DrawRectangle(x, y, w, h, bgColor);
	ppVector2 tSize = PPGraphics::Get()->GetTextSize(label, scale, scale);
	float startY = (h - tSize.y) / 2.0f;
	PPGraphics::Get()->DrawText(label, x, y + startY, scale, scale, txtColor, false);

#ifdef UI_DEBUG
	char buffer[100];
	snprintf(buffer, sizeof buffer, "w:%.02f|h:%.02f", tSize.x, tSize.y);
	PPGraphics::Get()->DrawText(buffer, x, y - 10, 0.8f * scale, 0.8f * scale, txtColor, false);
#endif

	return TouchUpOnArea(x, y, w, h) && !mTmpLockTouch;
}


///////////////////////////////////////////////////////////////////////////
// POPUP
///////////////////////////////////////////////////////////////////////////

bool PPUI::HasPopup()
{
	return mPopupList.size() > 0;
}

PopupCallback PPUI::GetPopup()
{
	return mPopupList[mPopupList.size() - 1];
}

void PPUI::ClosePopup()
{
	mPopupList.erase(mPopupList.end() - 1);
}

void PPUI::AddPopup(PopupCallback callback)
{
	mPopupList.push_back(callback);
}