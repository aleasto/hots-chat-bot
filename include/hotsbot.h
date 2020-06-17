#pragma once

int hots_bot_listen(char* channel_name, int polling_rate);
void hots_bot_set_new_message_cb(void (*cb)(int sender_id, char* sender_name, char* message));
void hots_bot_send_message(char* str);
