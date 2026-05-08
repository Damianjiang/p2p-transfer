#ifndef P2P_TRANSFER_H
#define P2P_TRANSFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

#define VERSION "1.0"
#define BUFFER_SIZE 65536
#define MAX_FILENAME 512
#define MAX_PASSWORD 64
#define HANDSHAKE_MAGIC 0x50503250
#define MSG_TYPE_HELLO 1
#define MSG_TYPE_AUTH 2
#define MSG_TYPE_FILE_REQUEST 3
#define MSG_TYPE_FILE_ACCEPT 4
#define MSG_TYPE_FILE_REJECT 5
#define MSG_TYPE_FILE_DATA 6
#define MSG_TYPE_FILE_END 7
#define MSG_TYPE_TEXT 8
#define MSG_TYPE_DISCOVER 9
#define MSG_TYPE_DISCOVER_ACK 10

typedef struct {
    int magic;
    int type;
    int length;
    char data[];
} Message;

typedef struct {
    int magic;
    int type;
    char password[MAX_PASSWORD];
    char username[64];
} AuthMessage;

typedef struct {
    int magic;
    int type;
    char filename[MAX_FILENAME];
    long filesize;
    char checksum[64];
} FileRequestMessage;

typedef struct {
    int magic;
    int type;
    int chunk_size;
    long offset;
    long total_size;
} FileDataHeader;

typedef struct {
    char local_ip[64];
    char external_ip[64];
    int local_port;
    int external_port;
} PeerInfo;

int g_running = 1;
int g_connected = 0;
int g_socket = -1;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
char g_remote_ip[64] = {0};
int g_remote_port = 0;
char g_password[MAX_PASSWORD] = {0};
char g_username[64] = "peer";

void print_banner();
void print_usage(const char *prog);
int create_socket();
int set_nonblocking(int fd);
int bind_port(int sock, int port);
int connect_to_peer(const char *ip, int port);
void *receive_thread(void *arg);
void *send_thread(void *arg);
int send_message(int sock, int type, const void *data, int len);
int recv_message(int sock, int *type, void **data);
int handle_message(int type, void *data, int len);
int send_file(const char *filepath);
int recv_file(int sock, const char *filename, long filesize);
int discover_external_ip(char *ip, int *port);
int tcp_hole_punch(const char *target_ip, int target_port, int listen_port);
int authenticate(int sock);
int verify_password(const char *password);
void console_input();
char *get_password_input();
int create_listening_socket(int port);
void *accept_thread(void *arg);
void print_peer_info();
int try_connect_with_strategy(const char *ip, int port, int local_port);
void signal_handler(int sig);

#endif
