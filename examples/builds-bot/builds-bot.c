#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hotsbot.h"
#include "string-additions.h"

#define BOT_PREFIX '?'
#define DATA_FILE "talent.data"
#define BUF_SIZE 24

void on_new_message(int sender_id, char* sender_name, char* message);
char* format_hero_name(char* hero_name);
char* search_talent_data(char* hero_name);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Specify a channel name\n");
        return EXIT_FAILURE;
    }

    char* channel_name = argv[1];
    int polling_rate = 2;
    hots_bot_set_new_message_cb(on_new_message);

    printf("HIGHTLIGHT GAME WINDOW NOW!\n");
    sleep(5);
    hots_bot_listen(channel_name, polling_rate);
    return 0;
}

void on_new_message(int sender_id, char* sender_name, char* message) {
    printf("[%d]%s: %s\n", sender_id, sender_name, message);

    if (message[0] != BOT_PREFIX)
        return;
    char* input_hero_name = &message[1];
    char* formatted_hero_name = format_hero_name(input_hero_name);

    char* talent_tree = search_talent_data(formatted_hero_name);
    char* out_buf = malloc(sizeof(char) * strlen(input_hero_name) + 128);
    out_buf[0] = '\0';
    if (talent_tree != NULL) {
        sprintf(out_buf, "[%s,%s]", talent_tree, formatted_hero_name);
        free(talent_tree);
    } else {
        sprintf(out_buf, "Could not find hero %s", input_hero_name);
    }
    hots_bot_send_message(out_buf);

    free(out_buf);
    free(formatted_hero_name);
}

// Caller must free the return
char* format_hero_name(char* hero_name) {
    char* replaced_apos = str_replace(hero_name, "&apos;", "");
    char* replaced_apos_dot = str_replace(replaced_apos, ".", "");
    char* replaced_apos_dot_dash = str_replace(replaced_apos_dot, "-", "");
    for (char* p = replaced_apos_dot_dash; *p; ++p) *p = tolower(*p);
    free(replaced_apos);
    free(replaced_apos_dot);
    return replaced_apos_dot_dash;
}

char* search_talent_data(char* hero_name) {
    // very slow but i just want something that works for now
    FILE* fp = fopen(DATA_FILE, "r");
    if (fp == NULL)
        goto nomatch;

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        char hero_name_buf[BUF_SIZE];
        char talent_tree_buf[BUF_SIZE];
        sscanf(line, "%[^:]: %s", hero_name_buf, talent_tree_buf);
        if (!strcmp(hero_name, hero_name_buf)) {
            return strdup(talent_tree_buf);
        }
    }
    free(line);

nomatch:
    return NULL;
}