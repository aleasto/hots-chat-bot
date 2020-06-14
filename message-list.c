#include <string.h>
#include <stdlib.h>
#include "message-list.h"

message_entry_t* find_message(message_entry_t* head, int sender_id, char* sender_name, char* message) {
    if (head == NULL)
        return NULL;

    for (message_entry_t* curr = head; curr != NULL; curr = curr->next) {
        if (curr->sender_id == sender_id && !strcmp(curr->sender_name, sender_name) && !strcmp(curr->message, message)) {
            return curr;
        }
    }
    return NULL;
}
void push_message(message_entry_t** head, int sender_id, char* sender_name, char* message) {
    message_entry_t* new = malloc(sizeof(message_entry_t));
    new->next = *head;
    new->sender_id = sender_id;
    new->sender_name = strdup(sender_name);
    new->message = strdup(message);
    *head = new;
}
void free_message_list(message_entry_t* head) {
    while (head) {
        message_entry_t* tbd = head;
        head = head->next;
        free(tbd->sender_name);
        free(tbd->message);
        free(tbd);
    }
}