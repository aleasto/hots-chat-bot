#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "talent.data"
#define BUF_SIZE 24

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
