#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <libwebsockets.h>
#include <cjson/cJSON.h>

#define QUEUESIZE 100
#define MSGSIZE 50000

// Circular buffer used for producer/consumer
char buffer[QUEUESIZE][MSGSIZE];
int head = 0;
int tail = 0;
int count = 0;

// Mutex and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notEmpty = PTHREAD_COND_INITIALIZER;

// Counters for the 4 messages
unsigned long commit_count = 0;
unsigned long identity_count = 0;
unsigned long account_count = 0;
unsigned long info_count = 0;

// Bbecomes 1 if lost connection
int connection_closed = 0;

// for calculation of CPU
unsigned long long previous_total = 0;
unsigned long long previous_idle = 0;

// Add one message to the buffer
void queueAdd(const char *message, size_t len)
{
    pthread_mutex_lock(&mutex);

    // Add the message only if there is free space
    if (count < QUEUESIZE) {
        memcpy(buffer[head], message, len);
        buffer[head][len] = '\0';
        head = (head + 1) % QUEUESIZE;
        count++;

        // Wake the consumer because a message is available
        pthread_cond_signal(&notEmpty);
    }
    pthread_mutex_unlock(&mutex);
}

// Remove one message from thebuffer
void queueDel(char *message)
{
    pthread_mutex_lock(&mutex);

    // Wait until the producer adds a message
    while (count == 0)
        pthread_cond_wait(&notEmpty, &mutex);

    strcpy(message, buffer[tail]);

    tail = (tail + 1) % QUEUESIZE;
    count--;

    pthread_mutex_unlock(&mutex);
}

// read the cpu line from /proc/stat and calculate
double get_cpu_usage(void)
{
    FILE *fp;
    char cpu[10];

    unsigned long long user, nice, system, idle;
    unsigned long long iowait, irq, softirq, steal;

    fp = fopen("/proc/stat", "r");

    if (fp == NULL)
        return 0.0;

    fscanf(fp, "%s %llu %llu %llu %llu %llu %llu %llu %llu",cpu, &user, &nice, &system, &idle,&iowait, &irq, &softirq, &steal);

    fclose(fp);
    // cpu time
    unsigned long long idle_now = idle + iowait;

    unsigned long long total_now =user+nice +system +idle+iowait +irq+softirq + steal;

    // from the starting point
    if (previous_total == 0) {
        previous_total = total_now;
        previous_idle = idle_now;
        return 0.0;
    }
    // difference from previus sample
    unsigned long long total_diff = total_now - previous_total;
    unsigned long long idle_diff = idle_now - previous_idle;

    previous_total = total_now;
    previous_idle = idle_now;

    if (total_diff == 0)
        return 0.0;
    return 100.0 * (total_diff - idle_diff) / total_diff;
}

// WebSocket func
// Called when message arrives, connection start or lost
static int websocket_callback(struct lws *wsi,enum lws_callback_reasons reason,void *user,void *in,size_t len)
{
    // message may arrive in more than one part
    static char message[MSGSIZE];
    static size_t message_len = 0;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            connection_closed = 0;
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            // First part of a new message
            if (lws_is_first_fragment(wsi))
                message_len = 0;

            // Store the received part
            if (message_len + len < MSGSIZE) {
                memcpy(message + message_len, in, len);
                message_len += len;

                // When the whole Json message has arrived add to the queue
                if (lws_is_final_fragment(wsi)) {
                    message[message_len] = '\0';
                    queueAdd(message, message_len);
                    message_len = 0;
                }
            }
            // too long
            else {
                message_len = 0;
            }
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_CLOSED:
            connection_closed = 1;
            break;

        default:
            break;
    }
    return 0;
}

// to connect the websock with client
static struct lws_protocols protocols[] = {
    { "jetstream", websocket_callback, 0, MSGSIZE },
    { NULL, NULL, 0, 0 }
};

// Producer
// Connects to Bluesky Jetstream and places incoming json mess to the buffer
void *producer(void *arg)
{
    while (1) {
        // Create websocket context
        struct lws_context_creation_info info = {0};

        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

        struct lws_context *context = lws_create_context(&info);

        if (context == NULL) {
            sleep(5);
            continue;
        }

        // Needed to connect the bluesky
        struct lws_client_connect_info connection = {0};

        connection.context = context;
        connection.address = "jetstream1.us-east.bsky.network";
        connection.port = 443;
        connection.path ="/subscribe?wantedCollections=app.bsky.feed.post";

        connection.host = connection.address;
        connection.origin = connection.address;
        connection.ssl_connection = LCCSCF_USE_SSL;
        connection.protocol = protocols[0].name;

        connection_closed = 0;
        // Stat the connection
        if (lws_client_connect_via_info(&connection) == NULL) {
            lws_context_destroy(context);
            sleep(5);
            continue;
        }
        // Wait, let libwebsockets receive messages
        while (!connection_closed)
            lws_service(context, 250);

        lws_context_destroy(context);

        // If the network disconnects, try again after 5 secs
        sleep(5);
    }
    return NULL;
}

// Consumer 
// Takes the json from the buffer and read the kind and increase the counter=
void *consumer(void *arg)
{
    char message[MSGSIZE];

    while (1) {
        queueDel(message);

        // Convert the text to json
        cJSON *json = cJSON_Parse(message);

        if (json == NULL)
            continue;

        // Read kind
        cJSON *kind = cJSON_GetObjectItemCaseSensitive(json, "kind");

        if (cJSON_IsString(kind)) {
            // mutex for protection
            pthread_mutex_lock(&mutex);

            if (strcmp(kind->valuestring, "commit") == 0)
                commit_count++;
            else if (strcmp(kind->valuestring, "identity") == 0)
                identity_count++;
            else if (strcmp(kind->valuestring, "account") == 0)
                account_count++;
            else if (strcmp(kind->valuestring, "info") == 0)
                info_count++;
            pthread_mutex_unlock(&mutex);
        }
        cJSON_Delete(json);
    }
    return NULL;
}

// Monitor thread
// run each sec adn record the counters, buffer occupancy and cpu usage
void *monitor(void *arg)
{
    FILE *log = fopen("metrics_log.txt", "a");

    if (log == NULL)
        return NULL;
    struct timespec next;
    struct timespec now;

    // First CPU reading as reference
    get_cpu_usage();

    // Find the next exact second
    clock_gettime(CLOCK_REALTIME, &next);
    next.tv_sec++;
    next.tv_nsec = 0;

    while (1) {
        // Sleep until the exact next second
        clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&next,NULL);

        // Exat time when the monitor woke up
        clock_gettime(CLOCK_REALTIME, &now);

        unsigned long commits;
        unsigned long identities;
        unsigned long accounts;
        unsigned long infos;
        int buffer_count;

        // Copy the counters and reset them for the next second
        pthread_mutex_lock(&mutex);

        commits = commit_count;
        identities = identity_count;
        accounts = account_count;
        infos = info_count;
        buffer_count = count;
        commit_count = 0;
        identity_count = 0;
        account_count = 0;
        info_count = 0;

        pthread_mutex_unlock(&mutex);

        // Current buffer occupancy
        double buffer_pct =100.0*buffer_count / QUEUESIZE;

        // CPU usage during the previous second
        double cpu_pct = get_cpu_usage();

        // Write the required metrics
        fprintf(log,"%lld,%ld,%lu,%lu,%lu,%lu,%.2f,%.2f\n",(long long)now.tv_sec,now.tv_nsec,commits,identities,accounts,infos,buffer_pct,cpu_pct);
        fflush(log);
        next.tv_sec++;
    }
    return NULL;
}

int main(void)
{
    pthread_t producer_thread;
    pthread_t consumer_thread;
    pthread_t monitor_thread;

    // Start the three threads
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    pthread_create(&monitor_thread, NULL, monitor, NULL);

    // Wait for the threads
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    pthread_join(monitor_thread, NULL);

    return 0;
}
