#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "mem-stats.h"
#include "x-additions.h"

#define PID_LEN 12
#define MEM_FILENAME_LEN PID_LEN + 10
#define PROC_NAME "HeroesOfTheStor"
#define MIN_ADDRESS 0x6000000
#define MAX_ADDRESS 0x40000000

int search_memory(pid_t pid);
void* search_at(void* args);
void parse_xml(char* str);
void clear_chat();

pthread_mutex_t output_lock;
Display* x_display;
char* channel_name_g;
void (*new_message_cb_g)(int, char*, char*);

struct search_arguments {
    char* filename;
    void* start;
    size_t len;
    char* pattern;
};

/* public interface */
int hots_bot_listen(char* channel_name, int polling_rate) {
    char line[PID_LEN];
    FILE *cmd = popen("pidof "PROC_NAME, "r");
    fgets(line, PID_LEN, cmd);
    pid_t pid = strtoul(line, NULL, 10 /* base */);
    pclose(cmd);
    if (!pid) {
        fprintf(stderr, "No such process: " PROC_NAME "\n");
        return EXIT_FAILURE;
    }
    x_display = XOpenDisplay(NULL);
    if (!x_display) {
        fprintf(stderr, "Could not open X display");
        return EXIT_FAILURE;
    }
    channel_name_g = strdup(channel_name);
    pthread_mutex_init(&output_lock, NULL);

    for (;;) {
        search_memory(pid);
        sleep(polling_rate);
    }

    XCloseDisplay(x_display);
    free(channel_name_g);
    pthread_mutex_destroy(&output_lock);

    return 0;
}

void hots_bot_set_new_message_cb(void (*cb)(int, char*, char*)) {
    new_message_cb_g = cb;
}

void hots_bot_send_message(char* str) {
    pthread_mutex_lock(&output_lock);
    XWriteString(x_display, str);
    XWriteSymbol(x_display, XK_Return);
    pthread_mutex_unlock(&output_lock);
}
/* end public interface */

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

    char search_pattern[1024];
    sprintf(search_pattern, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\"%s\">", channel_name_g);

    pthread_t* tid = malloc(sizeof(pthread_t) * regions_count);   // one thread per region
    int thread_no = 0;
    for (curr = last; curr != first; curr = curr->next) {
        if (curr->perms & PERMS_READ) {
            struct search_arguments* args = malloc(sizeof(*args));  // thread must free me
            args->filename = mem_filename;
            args->start = curr->start;
            args->len = curr->length;
            args->pattern = search_pattern;
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
                parse_xml(buf + i);
                // try to corrupt this message so that next time we don't catch it again
                pwrite(fd, "\0", 1, (size_t)args->start + i);
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

void parse_xml(char* str) {
    int sender_id;
    char* sender_name = NULL;
    char* message = NULL;
    clear_chat();   // try to free game memory
    if (sscanf(str, "<s val=\"BattleChatChannel\"><a name=\"ChannelName\" href=\"%*[^\"]\">[%*d. %*[^]]]</a> <a name=\"PresenceId\" href=\"%d\">%m[^:]:</a> %m[^<]</s>",
        &sender_id, &sender_name, &message) == 3) {
        new_message_cb_g(sender_id, sender_name, message);
    }

    free(sender_name);
    free(message);
}

void clear_chat() {
    hots_bot_send_message("/clear");
}
