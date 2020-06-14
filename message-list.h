#pragma once

struct el {
    struct el* next;
    int sender_id;
    char* sender_name;
    char* message;
};

typedef struct el message_entry_t;

message_entry_t* find_message(message_entry_t* head, int sender_id, char* sender_name, char* message);
void push_message(message_entry_t** head, int sender_id, char* sender_name, char* message);
void free_message_list(message_entry_t* head);