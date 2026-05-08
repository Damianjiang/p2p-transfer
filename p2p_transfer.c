#include "p2p_transfer.h"

void signal_handler(int sig) {
    g_running = 0;
}

void print_banner() {
    printf("\033[1;36m");
    printf("\n");
    printf("  ████████╗██╗  ██╗███████╗\n");
    printf("  ╚══██╔══╝██║  ██║██╔════╝\n");
    printf("     ██║   ███████║█████╗  \n");
    printf("     ██║   ██╔══██║██╔══╝  \n");
    printf("     ██║   ██║  ██║███████╗\n");
    printf("     ╚═╝   ╚═╝  ╚═╝╚══════╝\n");
    printf("\033[0m");
    printf("\033[1;33m P2P File Transfer v%s\033[0m\n", VERSION);
    printf("\033[0;32m Direct peer-to-peer connection\033[0m\n");
    printf("\033[0;32m No server required!\033[0m\n");
    printf("\n");
}

void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("\nOptions:\n");
    printf("  -h, --help          Show this help\n");
    printf("  -l, --listen <port> Start in server mode (listen on port)\n");
    printf("  -c, --connect <ip>  Connect to peer at IP\n");
    printf("  -p, --port <port>   Port for connection (default: 8888)\n");
    printf("  -P, --password <pw> Room password\n");
    printf("  -u, --username <n>  Username (default: peer)\n");
    printf("\nExample:\n");
    printf("  Host: %s -l 8888 -P secret123\n", prog);
    printf("  Client: %s -c 192.168.1.100 -p 8888 -P secret123\n", prog);
    printf("\n");
}

int create_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sock;
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int bind_port(int sock, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }
    
    if (listen(sock, 5) < 0) {
        perror("listen");
        return -1;
    }
    
    return 0;
}

int connect_to_peer(const char *ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(ip);
        if (!he) {
            fprintf(stderr, "Failed to resolve host: %s\n", ip);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    int sock = create_socket();
    if (sock < 0) return -1;
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    
    return sock;
}

void *receive_thread(void *arg) {
    int sock = *(int *)arg;
    free(arg);
    
    printf("\033[1;34m[+] Receive thread started\033[0m\n");
    
    while (g_running) {
        int type;
        void *data = NULL;
        int len = recv_message(sock, &type, &data);
        
        if (len < 0) {
            if (g_running) {
                printf("\033[1;31m[-] Connection closed\033[0m\n");
            }
            break;
        }
        
        if (len > 0) {
            handle_message(type, data, len);
            free(data);
        }
    }
    
    pthread_mutex_lock(&g_mutex);
    g_connected = 0;
    pthread_mutex_unlock(&g_mutex);
    
    return NULL;
}

int send_message(int sock, int type, const void *data, int len) {
    Message *msg = malloc(sizeof(Message) + len);
    if (!msg) return -1;
    
    msg->magic = htonl(HANDSHAKE_MAGIC);
    msg->type = htonl(type);
    msg->length = htonl(len);
    if (data && len > 0) {
        memcpy(msg->data, data, len);
    }
    
    int total = sizeof(Message) + len;
    int sent = 0;
    while (sent < total) {
        int n = send(sock, (char *)msg + sent, total - sent, 0);
        if (n <= 0) {
            free(msg);
            return -1;
        }
        sent += n;
    }
    
    free(msg);
    return 0;
}

int recv_message(int sock, int *type, void **data) {
    char header_buf[sizeof(Message)];
    int received = 0;
    
    while (received < sizeof(Message)) {
        int n = recv(sock, header_buf + received, sizeof(Message) - received, 0);
        if (n <= 0) return -1;
        received += n;
    }
    
    Message *header = (Message *)header_buf;
    int magic = ntohl(header->magic);
    int msg_type = ntohl(header->type);
    int len = ntohl(header->length);
    
    if (magic != HANDSHAKE_MAGIC) {
        fprintf(stderr, "Invalid magic: 0x%x\n", magic);
        return -1;
    }
    
    *type = msg_type;
    *data = NULL;
    
    if (len > 0 && len < 10 * 1024 * 1024) {
        *data = malloc(len);
        if (!*data) return -1;
        
        received = 0;
        while (received < len) {
            int n = recv(sock, (char *)*data + received, len - received, 0);
            if (n <= 0) {
                free(*data);
                *data = NULL;
                return -1;
            }
            received += n;
        }
    }
    
    return len;
}

int handle_message(int type, void *data, int len) {
    switch (type) {
        case MSG_TYPE_TEXT: {
            printf("\033[1;36m[Peer]: %.*s\033[0m\n", len, (char *)data);
            break;
        }
        
        case MSG_TYPE_FILE_REQUEST: {
            FileRequestMessage *req = (FileRequestMessage *)data;
            printf("\033[1;33m[+] Received file request: %s (%ld bytes)\033[0m\n", 
                   req->filename, (long)ntohl(req->filesize));
            break;
        }
        
        case MSG_TYPE_FILE_ACCEPT: {
            FileRequestMessage *req = (FileRequestMessage *)data;
            printf("\033[1;32m[+] Peer accepted: %s\033[0m\n", req->filename);
            break;
        }
        
        case MSG_TYPE_FILE_REJECT: {
            printf("\033[1;31m[-] Peer rejected file transfer\033[0m\n");
            break;
        }
        
        case MSG_TYPE_DISCOVER_ACK: {
            PeerInfo *info = (PeerInfo *)data;
            printf("\033[1;35m[*] Peer info - Local: %s:%d, External: %s:%d\033[0m\n",
                   info->local_ip, ntohl(info->local_port),
                   info->external_ip, ntohl(info->external_port));
            break;
        }
        
        default:
            printf("\033[1;34m[*] Received message type: %d (len=%d)\033[0m\n", type, len);
            break;
    }
    return 0;
}

int send_file(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) < 0) {
        perror("stat");
        return -1;
    }
    
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Not a regular file\n");
        return -1;
    }
    
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    const char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;
    
    FileRequestMessage req;
    memset(&req, 0, sizeof(req));
    req.magic = htonl(HANDSHAKE_MAGIC);
    req.type = htonl(MSG_TYPE_FILE_REQUEST);
    strncpy(req.filename, filename, MAX_FILENAME - 1);
    req.filesize = htonl(st.st_size);
    
    printf("\033[1;33m[*] Sending file: %s (%ld bytes)\033[0m\n", filename, (long)st.st_size);
    
    if (send_message(g_socket, MSG_TYPE_FILE_REQUEST, &req, sizeof(req)) < 0) {
        close(fd);
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    long total_sent = 0;
    int progress = 0;
    
    while (1) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0) break;
        
        FileDataHeader hdr;
        memset(&hdr, 0, sizeof(hdr));
        hdr.magic = htonl(HANDSHAKE_MAGIC);
        hdr.type = htonl(MSG_TYPE_FILE_DATA);
        hdr.chunk_size = htonl(n);
        hdr.offset = htonl(total_sent);
        hdr.total_size = htonl(st.st_size);
        
        if (send(g_socket, &hdr, sizeof(hdr), 0) != sizeof(hdr)) {
            close(fd);
            return -1;
        }
        
        if (send(g_socket, buffer, n, 0) != n) {
            close(fd);
            return -1;
        }
        
        total_sent += n;
        int new_progress = (int)((total_sent * 100) / st.st_size);
        if (new_progress != progress) {
            progress = new_progress;
            printf("\r\033[1;32m[*] Progress: %3d%% (%ld/%ld bytes)\033[0m", 
                   progress, total_sent, (long)st.st_size);
            fflush(stdout);
        }
    }
    
    printf("\n\033[1;32m[+] File sent successfully\033[0m\n");
    close(fd);
    return 0;
}

int recv_file(int sock, const char *filename, long filesize) {
    char filepath[MAX_FILENAME + 64];
    snprintf(filepath, sizeof(filepath), "received_%s", filename);
    
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    printf("\033[1;33m[*] Receiving file: %s (%ld bytes)\033[0m\n", filename, filesize);
    
    long total_received = 0;
    int progress = 0;
    
    while (total_received < filesize) {
        FileDataHeader hdr;
        int n = recv(sock, &hdr, sizeof(hdr), 0);
        if (n != sizeof(hdr)) break;
        
        int magic = ntohl(hdr.magic);
        int type = ntohl(hdr.type);
        int chunk_size = ntohl(hdr.chunk_size);
        
        if (magic != HANDSHAKE_MAGIC || type != MSG_TYPE_FILE_DATA) {
            break;
        }
        
        char buffer[BUFFER_SIZE];
        int received = 0;
        while (received < chunk_size) {
            n = recv(sock, buffer + received, chunk_size - received, 0);
            if (n <= 0) {
                close(fd);
                return -1;
            }
            received += n;
        }
        
        if (write(fd, buffer, received) != received) {
            close(fd);
            return -1;
        }
        
        total_received += received;
        int new_progress = (int)((total_received * 100) / filesize);
        if (new_progress != progress) {
            progress = new_progress;
            printf("\r\033[1;32m[*] Progress: %3d%% (%ld/%ld bytes)\033[0m", 
                   progress, total_received, filesize);
            fflush(stdout);
        }
    }
    
    printf("\n\033[1;32m[+] File received: %s\033[0m\n", filepath);
    close(fd);
    return 0;
}

int discover_external_ip(char *ip, int *port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr);
    
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr *)&local_addr, &addr_len) < 0) {
        close(sock);
        return -1;
    }
    
    getifaddrs(NULL);
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    
    close(sock);
    
    if (ip) {
        struct hostent *he = gethostbyname(hostname);
        if (he && he->h_addr_list[0]) {
            strncpy(ip, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), 63);
        } else {
            strcpy(ip, "127.0.0.1");
        }
    }
    
    if (port) {
        *port = ntohs(local_addr.sin_port);
    }
    
    return 0;
}

int authenticate(int sock) {
    AuthMessage auth;
    memset(&auth, 0, sizeof(auth));
    auth.magic = htonl(HANDSHAKE_MAGIC);
    auth.type = htonl(MSG_TYPE_AUTH);
    strncpy(auth.password, g_password, MAX_PASSWORD - 1);
    strncpy(auth.username, g_username, 63);
    
    return send_message(sock, MSG_TYPE_AUTH, &auth, sizeof(auth));
}

int verify_password(const char *password) {
    return strcmp(password, g_password) == 0;
}

char *get_password_input() {
    static char password[MAX_PASSWORD];
    struct termios old, new;
    
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    
    printf("Password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        password[0] = '\0';
    } else {
        size_t len = strlen(password);
        if (len > 0 && password[len - 1] == '\n') {
            password[len - 1] = '\0';
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    printf("\n");
    
    return password;
}

void console_input() {
    char input[1024];
    
    while (g_running) {
        printf("\033[1;36mp2p> \033[0m");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        if (strlen(input) == 0) continue;
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            g_running = 0;
            break;
        }
        else if (strcmp(input, "help") == 0) {
            printf("Available commands:\n");
            printf("  send <file>     - Send a file\n");
            printf("  msg <text>      - Send a text message\n");
            printf("  info            - Show connection info\n");
            printf("  discover        - Discover peer info\n");
            printf("  quit            - Exit\n");
        }
        else if (strncmp(input, "send ", 5) == 0) {
            if (!g_connected) {
                printf("\033[1;31m[-] Not connected\033[0m\n");
                continue;
            }
            const char *filepath = input + 5;
            send_file(filepath);
        }
        else if (strncmp(input, "msg ", 4) == 0) {
            if (!g_connected) {
                printf("\033[1;31m[-] Not connected\033[0m\n");
                continue;
            }
            const char *msg = input + 4;
            send_message(g_socket, MSG_TYPE_TEXT, msg, strlen(msg));
            printf("\033[1;32m[You]: %s\033[0m\n", msg);
        }
        else if (strcmp(input, "info") == 0) {
            print_peer_info();
        }
        else if (strcmp(input, "discover") == 0) {
            PeerInfo info;
            memset(&info, 0, sizeof(info));
            discover_external_ip(info.external_ip, &info.external_port);
            strncpy(info.local_ip, "127.0.0.1", 63);
            info.local_port = htonl(g_remote_port);
            info.external_port = htonl(info.external_port);
            send_message(g_socket, MSG_TYPE_DISCOVER, &info, sizeof(info));
        }
        else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
}

int create_listening_socket(int port) {
    int sock = create_socket();
    if (sock < 0) return -1;
    
    if (bind_port(sock, port) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

void *accept_thread(void *arg) {
    int listen_sock = *(int *)arg;
    free(arg);
    
    printf("\033[1;34m[+] Accept thread started, waiting for connections...\033[0m\n");
    
    while (g_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (g_running) perror("accept");
            continue;
        }
        
        char *client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);
        
        printf("\033[1;32m[+] Incoming connection from %s:%d\033[0m\n", client_ip, client_port);
        
        int *auth_result = malloc(sizeof(int));
        *auth_result = 0;
        
        pthread_t auth_thread;
        pthread_create(&auth_thread, NULL, (void *(*)(void *))authenticate, (void *)(long)client_sock);
        pthread_detach(auth_thread);
        
        pthread_mutex_lock(&g_mutex);
        if (g_connected) {
            printf("\033[1;33m[!] Already connected, rejecting new connection\033[0m\n");
            close(client_sock);
        } else {
            g_socket = client_sock;
            g_connected = 1;
            strncpy(g_remote_ip, client_ip, 63);
            g_remote_port = client_port;
            
            pthread_t *recv_t = malloc(sizeof(pthread_t));
            pthread_create(recv_t, NULL, receive_thread, (void *)(long)client_sock);
        }
        pthread_mutex_unlock(&g_mutex);
    }
    
    return NULL;
}

void print_peer_info() {
    printf("\033[1;35m[*] Connection Info:\033[0m\n");
    printf("  Status: %s\n", g_connected ? "Connected" : "Disconnected");
    if (g_connected) {
        printf("  Remote: %s:%d\n", g_remote_ip, g_remote_port);
    }
}

int try_connect_with_strategy(const char *ip, int port, int local_port) {
    printf("\033[1;33m[*] Attempting direct connection to %s:%d...\033[0m\n", ip, port);
    
    g_socket = connect_to_peer(ip, port);
    if (g_socket < 0) {
        printf("\033[1;31m[-] Direct connection failed\033[0m\n");
        return -1;
    }
    
    printf("\033[1;32m[+] Connected successfully!\033[0m\n");
    return 0;
}

int main(int argc, char *argv[]) {
    int listen_port = 0;
    char *connect_ip = NULL;
    int connect_port = 8888;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--listen") == 0) {
            if (i + 1 < argc) {
                listen_port = atoi(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--connect") == 0) {
            if (i + 1 < argc) {
                connect_ip = argv[++i];
            }
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                connect_port = atoi(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--password") == 0) {
            if (i + 1 < argc) {
                strncpy(g_password, argv[++i], MAX_PASSWORD - 1);
            }
        }
        else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
            if (i + 1 < argc) {
                strncpy(g_username, argv[++i], 63);
            }
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    print_banner();
    
    if (listen_port > 0) {
        printf("\033[1;32m[*] Starting in listen mode on port %d\033[0m\n", listen_port);
        
        if (strlen(g_password) == 0) {
            printf("\033[1;33m[!] No password set, using default 'p2p'\033[0m\n");
            strcpy(g_password, "p2p");
        }
        
        int listen_sock = create_listening_socket(listen_port);
        if (listen_sock < 0) {
            fprintf(stderr, "Failed to create listening socket\n");
            return 1;
        }
        
        pthread_t accept_t;
        int *arg = malloc(sizeof(int));
        *arg = listen_sock;
        pthread_create(&accept_t, NULL, accept_thread, arg);
        
        printf("\033[1;36m[i] Waiting for peer to connect...\033[0m\n");
        printf("\033[1;36m[i] Share this info with your peer:\033[0m\n");
        printf("    Your IP and port: <your_ip>:%d\n", listen_port);
        printf("    Password: %s\n", g_password);
        
        while (!g_connected && g_running) {
            sleep(1);
        }
        
        if (g_connected) {
            pthread_t *recv_t = malloc(sizeof(pthread_t));
            pthread_create(recv_t, NULL, receive_thread, (void *)(long)g_socket);
        }
    }
    
    if (connect_ip) {
        printf("\033[1;32m[*] Starting in connect mode to %s:%d\033[0m\n", connect_ip, connect_port);
        
        if (strlen(g_password) == 0) {
            printf("\033[1;33m[!] Password required: \033[0m");
            strcpy(g_password, get_password_input());
        }
        
        if (try_connect_with_strategy(connect_ip, connect_port, 0) < 0) {
            fprintf(stderr, "\033[1;31m[-] Failed to connect to peer\033[0m\n");
            return 1;
        }
        
        if (authenticate(g_socket) < 0) {
            fprintf(stderr, "\033[1;31m[-] Authentication failed\033[0m\n");
            close(g_socket);
            return 1;
        }
        
        pthread_t *recv_t = malloc(sizeof(pthread_t));
        pthread_create(recv_t, NULL, receive_thread, (void *)(long)g_socket);
    }
    
    if (listen_port == 0 && !connect_ip) {
        print_usage(argv[0]);
        return 1;
    }
    
    console_input();
    
    if (g_socket >= 0) {
        close(g_socket);
    }
    
    printf("\033[1;34m[*] Goodbye!\033[0m\n");
    return 0;
}
