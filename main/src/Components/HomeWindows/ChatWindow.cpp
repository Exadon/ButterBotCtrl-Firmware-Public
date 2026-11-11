#include "ChatWindow.h"

#include <Phrases.h>

ChatWindow::ChatWindow(lv_obj_t* parent, const ChatData* params) : HomeWindow(parent, TITLE){
	sentenceLabel = lv_label_create(innerContent);

	// Sentence label
	lv_label_set_long_mode(sentenceLabel, LV_LABEL_LONG_WRAP);
	lv_obj_add_style(sentenceLabel, labelDefaultStyle, 0);
	lv_obj_set_size(sentenceLabel, 90, LV_SIZE_CONTENT);

	buildUI(*params);
}

void ChatWindow::onData(const BBData* data){
	buildUI(*static_cast<const ChatData*>(data));
}

void ChatWindow::buildUI(ChatData data){
	lv_label_set_text(sentenceLabel, Phrases::mapShown(Phrase::Chat, data.id).c_str());
	updateLayout();
}
