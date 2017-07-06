#include "ui/ui_context.h"
#include "ui/view.h"
#include "ui/viewgroup.h"
#include "ui/ui.h"
#include "ChatScreen.h"
#include "Core/Config.h"
#include "Core/Host.h"
#include "Core/System.h"
#include "Common/LogManager.h"
#include "Core/HLE/proAdhoc.h"
#include "i18n/i18n.h"
#include <ctype.h>
#include "util/text/utf8.h"


void ChatMenu::CreatePopupContents(UI::ViewGroup *parent) {
	using namespace UI;
	I18NCategory *n = GetI18NCategory("Networking");
	LinearLayout *outer = new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT,400));
	scroll_ = outer->Add(new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(1.0)));
	LinearLayout *bottom = outer->Add(new LinearLayout(ORIENT_HORIZONTAL, new LayoutParams(FILL_PARENT, WRAP_CONTENT)));
#if defined(_WIN32) || defined(USING_QT_UI)
	chatEdit_ = bottom->Add(new TextEdit("", n->T("Chat Here"), new LinearLayoutParams(1.0)));
#if defined(USING_WIN_UI)
	//freeze  the ui when using ctrl + C hotkey need workaround
	if (g_Config.bBypassOSKWithKeyboard && !g_Config.bFullScreen)
	{
		std::wstring titleText = ConvertUTF8ToWString(n->T("Chat"));
		std::wstring defaultText = ConvertUTF8ToWString(n->T("Chat Here"));
		std::wstring inputChars;
		if (System_InputBoxGetWString(titleText.c_str(), defaultText, inputChars)) {
			//chatEdit_->SetText(ConvertWStringToUTF8(inputChars));
			sendChat(ConvertWStringToUTF8(inputChars));
		}
	}
#endif
	chatEdit_->OnEnter.Handle(this, &ChatMenu::OnSubmit);
	bottom->Add(new Button(n->T("Send")))->OnClick.Handle(this, &ChatMenu::OnSubmit);
#elif defined(__ANDROID__)
	bottom->Add(new Button(n->T("Chat Here"),new LayoutParams(FILL_PARENT, WRAP_CONTENT)))->OnClick.Handle(this, &ChatMenu::OnSubmit);
#endif

	if (g_Config.bEnableQuickChat) {
		LinearLayout *quickChat = outer->Add(new LinearLayout(ORIENT_HORIZONTAL, new LayoutParams(FILL_PARENT, WRAP_CONTENT)));
		quickChat->Add(new Button(n->T("1"), new LinearLayoutParams(1.0)))->OnClick.Handle(this, &ChatMenu::OnQuickChat1);
		quickChat->Add(new Button(n->T("2"), new LinearLayoutParams(1.0)))->OnClick.Handle(this, &ChatMenu::OnQuickChat2);
		quickChat->Add(new Button(n->T("3"), new LinearLayoutParams(1.0)))->OnClick.Handle(this, &ChatMenu::OnQuickChat3);
		quickChat->Add(new Button(n->T("4"), new LinearLayoutParams(1.0)))->OnClick.Handle(this, &ChatMenu::OnQuickChat4);
		quickChat->Add(new Button(n->T("5"), new LinearLayoutParams(1.0)))->OnClick.Handle(this, &ChatMenu::OnQuickChat5);
	}
	chatVert_ = scroll_->Add(new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT)));
	chatVert_->SetSpacing(0);
	parent->Add(outer);
}

void ChatMenu::CreateViews() {
	using namespace UI;

	I18NCategory *n = GetI18NCategory("Networking");
	UIContext &dc = *screenManager()->getUIContext();

	AnchorLayout *anchor = new AnchorLayout(new LayoutParams(FILL_PARENT, FILL_PARENT));
	anchor->Overflow(false);
	root_ = anchor;

	float yres = screenManager()->getUIContext()->GetBounds().h;

	switch (g_Config.iChatScreenPosition) {
	// the chat screen size is still static 280,250 need a dynamic size based on device resolution 
	case 0:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, 280, NONE, NONE, 250, true));
		break;
	case 1:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, dc.GetBounds().centerX(), NONE, NONE, 250, true));
		break;
	case 2:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, NONE, NONE, 280, 250, true));
		break;
	case 3:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, 280, 250, NONE, NONE, true));
		break;
	case 4:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, dc.GetBounds().centerX(), 250, NONE, NONE, true));
		break;
	case 5:
		box_ = new LinearLayout(ORIENT_VERTICAL, new AnchorLayoutParams(PopupWidth(), FillVertical() ? yres - 30 : WRAP_CONTENT, NONE, 250, 280, NONE, true));
		break;
	}

	root_->Add(box_);
	box_->SetBG(UI::Drawable(0x66303030));
	box_->SetHasDropShadow(false);

	View *title = new PopupHeader(n->T("Chat"));
	box_->Add(title);

	CreatePopupContents(box_);
#if defined(_WIN32) || defined(USING_QT_UI)
	root_->SetDefaultFocusView(box_);
	box_->SubviewFocused(chatEdit_);
	root_->SetFocus();
#else
	root_->SetDefaultFocusView(box_);
#endif
	chatScreenVisible = true;
	newChat = 0;
	UI::EnableFocusMovement(true);
	UpdateChat();
}

void ChatMenu::dialogFinished(const Screen *dialog, DialogResult result) {
	UpdateUIState(UISTATE_INGAME);
}

UI::EventReturn ChatMenu::OnSubmit(UI::EventParams &e) {
#if defined(_WIN32) || defined(USING_QT_UI)
	std::string chat = chatEdit_->GetText();
	chatEdit_->SetText("");
	chatEdit_->SetFocus();
	sendChat(chat);
#elif defined(__ANDROID__)
	System_SendMessage("inputbox", "Chat:");
#endif
	return UI::EVENT_DONE;
}


UI::EventReturn ChatMenu::OnQuickChat1(UI::EventParams &e) {
	sendChat(g_Config.sQuickChat0);
	return UI::EVENT_DONE;
}

UI::EventReturn ChatMenu::OnQuickChat2(UI::EventParams &e) {
	sendChat(g_Config.sQuickChat1);
	return UI::EVENT_DONE;
}

UI::EventReturn ChatMenu::OnQuickChat3(UI::EventParams &e) {
	sendChat(g_Config.sQuickChat2);
	return UI::EVENT_DONE;
}

UI::EventReturn ChatMenu::OnQuickChat4(UI::EventParams &e) {
	sendChat(g_Config.sQuickChat3);
	return UI::EVENT_DONE;
}

UI::EventReturn ChatMenu::OnQuickChat5(UI::EventParams &e) {
	sendChat(g_Config.sQuickChat4);
	return UI::EVENT_DONE;
}

/*
	maximum chat length in one message from server is only 64 character
	need to split the chat to fit the static chat screen size
	if the chat screen size become dynamic from device resolution
	we need to change split function logic also.
*/
std::vector<std::string> Split(const std::string& str)
{
	std::vector<std::string> ret;
	int counter = 0;
	int firstSentenceEnd = 0;
	int secondSentenceEnd = 0;
	for (auto i = 0; i<str.length(); i++) {
		if (isspace(str[i])) {
			if (i < 35) {
				if(str[i-1]!=':')
					firstSentenceEnd = i+1;
			}
			else if (i > 35) {
				secondSentenceEnd = i;
			}
		}
	}

	if (firstSentenceEnd == 0) {
		firstSentenceEnd = 35;
	}
	
	if(secondSentenceEnd == 0){
		secondSentenceEnd = str.length();
	}

	ret.push_back(str.substr(0, firstSentenceEnd));
	ret.push_back(str.substr(firstSentenceEnd, secondSentenceEnd));
	return ret;
}

void ChatMenu::UpdateChat() {
	using namespace UI;
	if (chatVert_ != NULL) {
		chatVert_->Clear(); //read Access violation is proadhoc.cpp use NULL_->Clear() pointer?
		std::vector<std::string> chatLog = getChatLog();
		for (auto i : chatLog) {
			//split long text
			if (i.length() > 30) {
				std::vector<std::string> splitted = Split(i);
				for (auto j : splitted) {
					TextView *v = chatVert_->Add(new TextView(j, FLAG_DYNAMIC_ASCII, false));
					uint32_t color = 0xFFFFFF;
					v->SetTextColor(0xFF000000 | color);
				}
			}
			else {
				TextView *v = chatVert_->Add(new TextView(i, FLAG_DYNAMIC_ASCII, false));
				uint32_t color = 0xFFFFFF;
				v->SetTextColor(0xFF000000 | color);
			}
		}
		toBottom_ = true;
		updateChatScreen = false;
	}
}

bool ChatMenu::touch(const TouchInput &touch) {
	if (!box_ || (touch.flags & TOUCH_DOWN) == 0 || touch.id != 0) {
		return UIDialogScreen::touch(touch);
	}

	if (!box_->GetBounds().Contains(touch.x, touch.y)){
		screenManager()->finishDialog(this, DR_BACK);
	}

	return UIDialogScreen::touch(touch);
}

void ChatMenu::update() {
	PopupScreen::update();
	if (updateChatScreen) {
		UpdateChat();
	}
	else {
		if (scroll_ && toBottom_) {
			toBottom_ = false;
			scroll_->ScrollToBottom();
		}
	}
}

ChatMenu::~ChatMenu() {
	chatScreenVisible = false;
}
