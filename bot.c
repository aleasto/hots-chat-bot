#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
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
#define MIN_ADDRESS 0x6000000
#define MAX_ADDRESS 0x40000000
#define BOT_PREFIX '?'

int search_memory(pid_t pid);
void* search_at(void* args);
void parse_xml(char* str, message_entry_t** new_list);
void on_new_message(int sender_id, char* sender_name, char* message);
void send_build_message(char* hero, char* talent_tree);
void send_error_message(char* str);
void clear_chat();

pthread_mutex_t list_lock;
pthread_mutex_t output_lock;
message_entry_t* tracked_messages_g = NULL;
Display* x_display;
char* channel_name_g;

struct search_arguments {
    char* filename;
    void* start;
    size_t len;
    char* pattern;
    message_entry_t** output_list;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Specify a channel name\n");
        return EXIT_FAILURE;
    }
    channel_name_g = argv[1];

    char line[PID_LEN];
    FILE *cmd = popen("pidof "PROC_NAME, "r");
    fgets(line, PID_LEN, cmd);
    pid_t pid = strtoul(line, NULL, 10 /* base */);
    pclose(cmd);
    if (pid == 0) {
        fprintf(stderr, "No such process: " PROC_NAME "\n");
        return EXIT_FAILURE;
    }
    x_display = XOpenDisplay(NULL);

    pthread_mutex_init(&list_lock, NULL);
    pthread_mutex_init(&output_lock, NULL);

    printf("HIGHTLIGHT GAME WINDOW NOW!\n");
    sleep(5);
    for (;;) {
        search_memory(pid);
        sleep(2);
    }
    free_message_list(tracked_messages_g);

    pthread_mutex_destroy(&list_lock);
    pthread_mutex_destroy(&output_lock);
}

int search_memory(pid_t pid) {
    char mem_filename[MEM_FILENAME_LEN + 1];
    snprintf(mem_filename, MEM_FILENAME_LEN, "/proc/%ld/mem", (long)pid);
    ptrace(PTRACE_ATTACH, pid, NULL, NULL); // pause the process while we're reading its memory
    waitpid(pid, NULL, 0);

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
    int regions_count = 0;
    for (curr = last; curr != first; curr = curr->next) {
        if (curr->perms & PERMS_READ) {
            regions_count++;
        }
    }

    time_t ltime = time(NULL);
    char* time_string = asctime(localtime(&ltime));
    time_string[strlen(time_string) - 1] = '\0';
    printf("[%s] Searching %d regions at %p:%p for chat logs\n", time_string, regions_count, first->start, last->start + last->length);

    char search_pattern[1024];
    sprintf(search_pattern, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\"%s\">", channel_name_g);

    pthread_t* tid = malloc(sizeof(pthread_t) * regions_count);   // one thread per region
    message_entry_t* temp_message_list = NULL;
    int thread_no = 0;
    for (curr = last; curr != first; curr = curr->next) {
        if (curr->perms & PERMS_READ) {
            struct search_arguments* args = malloc(sizeof(*args));  // thread must free me
            args->filename = mem_filename;
            args->start = curr->start;
            args->len = curr->length;
            args->pattern = search_pattern;
            args->output_list = &temp_message_list;
            pthread_create(&tid[thread_no], NULL, search_at, args);
            thread_no++;
        }
    }
    for (thread_no = 0; thread_no < regions_count; thread_no++) {
        pthread_join(tid[thread_no], NULL);
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    free(tid);
    free_mem_stats(list);

    free_message_list(tracked_messages_g);
    tracked_messages_g = temp_message_list;

    return 0;
}

void* search_at(void* a) {
    struct search_arguments* args = (struct search_arguments*) a;
    char* buf = malloc(sizeof(*buf) * args->len);
    if (buf == NULL) {
        goto error_malloc;
    }
    int fd = open(args->filename, O_RDWR);
    if (fd < 0) {
        goto error_open;
    }
    ssize_t read = pread(fd, buf, args->len, (size_t)args->start);
    if (read < 0) {
        goto error_read;
    }

    int search_len = strlen(args->pattern);
    for (size_t i = 0; i < args->len; i++) {
        if (buf[i] == args->pattern[0]) {
            if (!memcmp(buf + i, args->pattern, search_len)) {
                parse_xml(buf + i, args->output_list);
                i += strlen(buf + i) - 1;
            }
        }
    }

error_read:
    close(fd);
error_open:
    free(buf);
error_malloc:
    free(a);
    return NULL;
}

void parse_xml(char* str, message_entry_t** new_list) {
    int sender_id;
    char* sender_name = NULL;
    char* message = NULL;
    if (sscanf(str, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\"%*[^\"]\">[%*d. %*[^]]]</a> <a name=\"PresenceId\" href=\"%d\">%m[^:]:</a> %m[^<]</s>",
        &sender_id, &sender_name, &message) < 3) {
        goto leave;
    }
    if (!find_message(tracked_messages_g, sender_id, sender_name, message)) {
        clear_chat();   // try to free game memory
        if (message[0] == BOT_PREFIX) {
            on_new_message(sender_id, sender_name, message);
        }
    }
    pthread_mutex_lock(&list_lock);
    push_message(new_list, sender_id, sender_name, message);
    pthread_mutex_unlock(&list_lock);

leave:
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

    pthread_mutex_lock(&output_lock);
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
    pthread_mutex_unlock(&output_lock);

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
    pthread_mutex_lock(&output_lock);

    XWriteSymbol(x_display, XK_slash);
    XWriteString(x_display, "clear");
    XWriteSymbol(x_display, XK_Return);

    pthread_mutex_unlock(&output_lock);
}
