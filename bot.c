#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "mem-stats.h"
#include "message-list.h"
#include "x-additions.h"
#include "string-additions.h"
#include "talent-data.h"

#define PID_LEN 12
#define MEM_FILENAME_LEN PID_LEN + 10
#define PROC_NAME "HeroesOfTheStor"
#define X_WINDOW_NAME "Heroes of the Storm"
#define MIN_ADDRESS 0x6000000
#define MAX_ADDRESS 0x40000000

#define CHANNEL_NAME "test bot"
#define EXCLUDE_MSG " Channel:" // `Joined Channel:` or Left Channel:`
#define BOT_PREFIX '?'

int search_memory(pid_t pid);
void search_at(int fd, void* start, size_t length, char* str, message_entry_t** new_list);
void parse_xml(char* str, message_entry_t** new_list);
void on_new_message(int sender_id, char* sender_name, char* message);
void send_build_message(char* hero, char* talent_tree);
void send_error_message(char* str);
void clear_chat();

message_entry_t* tracked_messages_g = NULL;
Display* x_display;

int main(int argc, char* argv[]) {
    pid_t pid;
    if (argc > 1) {
        pid = atoi(argv[1]);
    } else {
        char line[PID_LEN];
        FILE *cmd = popen("pidof "PROC_NAME, "r");
        fgets(line, PID_LEN, cmd);
        pid = strtoul(line, NULL, 10 /* base */);
        pclose(cmd);
        if (pid == 0) {
            fprintf(stderr, "No such process: " PROC_NAME "\n");
            return EXIT_FAILURE;
        }
    }
    x_display = XOpenDisplay(NULL);

    printf("HIGHTLIGHT GAME WINDOW NOW!\n");
    for (;;) {
        sleep(4);
        search_memory(pid);
    }
    free_message_list(tracked_messages_g);
}

int search_memory(pid_t pid) {
    address_range* list;
    list = mem_stats(pid);
    if (!list) {
        fprintf(stderr, "Cannot obtain memory usage of process %d: %s.\n", pid, strerror(errno));
        return EXIT_FAILURE;
    }

    address_range *curr, *first = NULL, *last = NULL;
    for (curr = list; curr != NULL; curr = curr->next) {
        if ((first == NULL || curr->start < first->start) && curr->start > (void*)MIN_ADDRESS) {
            first = curr;
        }
        if ((last == NULL || curr->start > last->start) && curr->start < (void*)MAX_ADDRESS) {
            last = curr;
        }
    }
    time_t ltime = time(NULL);
    char* time_string = asctime(localtime(&ltime));
    time_string[strlen(time_string) - 1] = '\0';
    printf("[%s] Searching %p:%p for chat logs\n", time_string, first->start, last->start + last->length);

    char mem_filename[MEM_FILENAME_LEN + 1];
    snprintf(mem_filename, MEM_FILENAME_LEN, "/proc/%ld/mem", (long)pid);

    ptrace(PTRACE_ATTACH, pid, NULL, NULL); // pause the process while we're reading its memory
    waitpid(pid, NULL, 0);
    int fd = open(mem_filename, O_RDWR);
    message_entry_t* temp_message_list = NULL;
    for (curr = last; curr != first; curr = curr->next)
        search_at(fd, curr->start, curr-> length, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\""CHANNEL_NAME"\">", &temp_message_list);
    close(fd);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    free_mem_stats(list);

    free_message_list(tracked_messages_g);
    tracked_messages_g = temp_message_list;

    return 0;
}

void search_at(int fd, void* start, size_t length, char* str, message_entry_t** new_list) {
    char* buf = malloc(sizeof(char) * length);
    lseek(fd, (size_t)start, SEEK_SET);
    read(fd, buf, length);

    int search_len = strlen(str);
    for (size_t i = 0; i < length; i++) {
        if (buf[i] == str[0]) {
            if (!memcmp(buf + i, str, search_len) && !strstr(buf + i, EXCLUDE_MSG)) {
                parse_xml(buf + i, new_list);
                i += strlen(buf + i) - 1;
            }
        }
    }
    free(buf);
}

void parse_xml(char* str, message_entry_t** new_list) {
    int sender_id;
    char* sender_name;
    char* message;
    if (sscanf(str, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\""CHANNEL_NAME"\">[%*d. "CHANNEL_NAME"]</a> <a name=\"PresenceId\" href=\"%d\">%m[^:]:</a> %m[^<]</s>",
        &sender_id, &sender_name, &message) < 3) {
        return;
    }
    if (!find_message(tracked_messages_g, sender_id, sender_name, message)) {
        clear_chat();   // try to free game memory
        if (message[0] == BOT_PREFIX) {
            on_new_message(sender_id, sender_name, message);
        }
    }
    push_message(new_list, sender_id, sender_name, message);
    free(sender_name);
    free(message);
}

void on_new_message(int sender_id, char* sender_name, char* message) {
    printf("[%d]%s: %s\n", sender_id, sender_name, message);

    char* parsed_name;
    sscanf(message, "?%ms", &parsed_name);
    char* replaced_apos = str_replace(parsed_name, "&apos;", "");
    char* replaced_apos_dot = str_replace(replaced_apos, ".", "");
    char* replaced_apos_dot_dash = str_replace(replaced_apos_dot, "-", "");
    free(replaced_apos);
    free(replaced_apos_dot);

    char* talent_tree = search_talent_data(replaced_apos_dot_dash);
    if (talent_tree != NULL) {
        send_build_message(replaced_apos_dot_dash, talent_tree);
        free(talent_tree);
    } else {
        char* error_buf = malloc(sizeof(char) * (strlen(parsed_name) + 128));
        sprintf(error_buf, "Could not find hero %s", parsed_name);
        send_error_message(error_buf);
        free(error_buf);
    }
    free(parsed_name);
    free(replaced_apos_dot_dash);
}

void send_build_message(char* hero_name, char* talent_tree) {
    XWriteSymbol(x_display, XK_bracketleft);
    XWriteString(x_display, talent_tree);
    XWriteSymbol(x_display, XK_comma);
    XWriteString(x_display, hero_name);
    XWriteSymbol(x_display, XK_bracketright);
    XWriteSymbol(x_display, XK_Return);
}

void send_error_message(char* str) {
    XWriteString(x_display, str);
    XWriteSymbol(x_display, XK_Return);
}

void clear_chat() {
    XWriteSymbol(x_display, XK_slash);
    XWriteString(x_display, "clear");
    XWriteSymbol(x_display, XK_Return);
}
