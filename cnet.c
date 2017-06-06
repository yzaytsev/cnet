#include <stdio.h>
#include <sys/mman.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <ctype.h>

#include "detection/paging.h"
#include "util/error.h"
#include "util/colorprint.h"

#include "cnet.h"
#include "jag/common.h"
#include "jag/send.h"
#include "jag/receive.h"
#include "detection/cache.h"
#include "detection/cpu.h"
#include "util/getopt_helper.h"
#include <net/transfer.h>

static size_t *usable_sets;

static getopt_arg_t options[] =
        {
                {"receive",    no_argument,       NULL, 'r', "Start in [c]receive mode[/c] (default: [m]send mode[/m])", NULL},
                {"threshold",  required_argument, NULL, 't', "Set the cache-miss threshold to [y]THRESHOLD[/y] (default: 280)",                             "THRESHOLD"},
                {"delay",      required_argument, NULL, 'd', "Increase the [m]jamming[/m] and [c]listening[/c] delays by factor [y]DELAY[/y] (default: 1)", "DELAY"},
                {"patience",   required_argument, NULL, 'p', "Sets the watchdog timeout to [y]SECONDS[/y] seconds (default: 10)",                           "SECONDS"},
                {"cache-size", required_argument, NULL, 'c', "Set the size of the L3 cache to [y]SIZE[/y] bytes (default: auto detect)",                    "SIZE"},
                {"ways",       required_argument, NULL, 'w', "Set the number of cache ways to [y]WAYS[/y] ways (default: auto detect)",                     "WAYS"},
                {"slices",     required_argument, NULL, 's', "Set the number of cache slices to [y]SLICES[/y] slices (default: auto detect)",               "SLICES"},
                {"channels",   required_argument, NULL, 'a', "Set the number of cache channels to establish (default: 8)",                                  "CHANNELS"},
                {"message",    required_argument, NULL, 'm', "Message to send",                                                                             "MESSAGE"},
                {"file",       required_argument, NULL, 'f', "File to send/receive",                                                                                "FILE"},
                {"no-color",   no_argument,       NULL, 'n', "Disable color output.",                                    NULL},
                {"verbose",    no_argument,       NULL, 'v', "Verbose mode",                                             NULL},
                {"help",       no_argument,       NULL, 'h', "Show this help.",                                          NULL},
                {NULL, 0,                         NULL, 0, NULL,                                                         NULL}
        };

#if 0
int string_to_bits(uint8_t **dst_ptr, const char *str);
#endif
void print_message_data(const uint8_t *data, int data_len, const char *action_description);

int main(int argc, char **argv) {
    int verbose = 0;
    watchdog_t watchdog_settings;
    cjag_config_t config = {
            .send = 1,
            .color_output = 1,
            .channels = 6, /* YZ */
            .cache_miss_threshold = 280,
            .cache_probe_count = 3,
            .jag_send_count = 5000 * 3,
            .jag_recv_count = 15000 * 3,
            .set_offset = 10,
            .timeout = 10,
            .packet_size = config.channels /* - 2 */,
            .watchdog = &watchdog_settings
    };

    cache_config_l3_t l3 = get_l3_info();
    config.cache_size = l3.size;
    config.cache_ways = l3.ways;
    config.cache_kill_count = config.cache_ways - 1;
    config.cache_slices = get_slices();


    struct option *long_options = getopt_get_long_options((getopt_arg_t *) options);
    ERROR_ON(!long_options, ERROR_OOM, config.color_output);

    uint8_t *send_bits     = NULL;
    const char *message    = NULL;
    const char *file_name  = NULL;

    int c;
    while ((c = getopt_long(argc, argv, ":c:t:w:rs:nhd:p:a:m:f:v", long_options, NULL)) != EOF) {
        switch (c) {
            case 'r':
                config.send = 0;
                break;
            case 'c':
                config.cache_size = atoi(optarg);
                break;
            case 't':
                config.cache_miss_threshold = atoi(optarg);
                break;
            case 'w':
                config.cache_ways = atoi(optarg);
                config.cache_kill_count = config.cache_ways - 1;
                break;
            case 's':
                config.cache_slices = atoi(optarg);
                break;
            case 'n':
                config.color_output = 0;
                break;
            case 'd':
                config.jag_send_count = (int) (config.jag_send_count * atof(optarg));
                config.jag_recv_count = (int) (config.jag_recv_count * atof(optarg));
                break;
            case 'p':
                config.timeout = atoi(optarg);
                break;
            case 'a':
                config.channels = atoi(optarg);
                break;
            case 'f':
                file_name = strdup(optarg);
                break;
            case 'm':
                message= strdup(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                show_usage(argv[0], &config);
                return 0;
            case ':':
                printf_color(config.color_output, ERROR_TAG "Option [c]-%c[/c] requires an [y]argument[/y].\n",
                             optopt);
                printf("\n");
                show_usage(argv[0], &config);
                return 1;
            case '?':
                if (isprint(optopt)) {
                    printf_color(config.color_output, ERROR_TAG "[y]Unknown[/y] option [c]-%c[/c].\n", optopt);
                } else {
                    printf_color(config.color_output, ERROR_TAG "[y]Unknown[/y] option character [c]\\x%x[/c].\n",
                                 optopt);
                }
                printf("\n");
                show_usage(argv[0], &config);
                return 1;
            default:
                show_usage(argv[0], &config);
                return 0;
        }
    }

    free(long_options);

    show_welcome(&config);
    show_parameters(&config);
    //printf("packet_size=%d.\n", config.packet_size);

    ERROR_ON(config.cache_slices > 16, ERROR_TOO_MANY_CPUS, config.color_output);
    ERROR_ON(config.cache_slices < 1 || config.cache_slices > 8 || (config.cache_slices & (config.cache_slices - 1)),
             ERROR_SLICES_NOT_SUPPORTED, config.color_output);
    ERROR_ON(config.cache_ways < 2, ERROR_INVALID_WAYS, config.color_output);
    ERROR_ON(config.timeout <= 1, ERROR_TIMEOUT_TOO_LOW, config.color_output);

    ERROR_ON(config.jag_send_count <= 0 || config.jag_recv_count <= 0, ERROR_INVALID_SPEED, config.color_output);

    ERROR_ON(!has_huge_pages(), ERROR_NO_HUGEPAGES, config.color_output);

    int init = jag_init(&config);
    ERROR_ON(!init, ERROR_NO_CACHE_SETS, config.color_output);

    usable_sets = calloc(config.channels, sizeof(size_t));
    ERROR_ON(!usable_sets, ERROR_OOM, config.color_output);
    ERROR_ON(config.send && !message && !file_name, ERROR_INVALID_PARAMETERS, config.color_output);

    watchdog_start(config.watchdog, config.timeout, timeout, (void *) &config);

    void **addrs = malloc(config.cache_ways * config.channels * sizeof(void *));
    if (config.send) {
        printf_color(config.color_output, "[y]Send mode[/y]: start [m]jamming[/m]...\n\n");
        printf_color(config.color_output, "[g][ # ][/g] Jamming set...\n");
        watchdog_reset(config.watchdog);
        jag_send(&config, send_callback);
        if(verbose) {
            printf_color(config.color_output, "[y]Eviction sets[/y]:\n");
            print_eviction_sets(config.addr, &config);
        }

        printf_color(config.color_output, "\n[y]Verification mode[/y]: start [c]receiving[/c]...\n\n");
        watchdog_reset(config.watchdog);
        sleep(1);
        watchdog_reset(config.watchdog);
        sleep(1);
        for (int i = 0; i < 15; i++) {
            watchdog_reset(config.watchdog);
            sched_yield();
        }

        printf_color(config.color_output, "[g][ # ][/g] Checking sets...\n");
        watchdog_reset(config.watchdog);
        jag_receive(addrs, usable_sets, &config, watch_callback);
        watchdog_done(config.watchdog);
        if(verbose) {
            printf_color(config.color_output, "[y]Eviction sets[/y]:\n");
            print_eviction_sets(addrs, &config);
        }

        int correct = 0;
        for (int i = 0; i < config.channels; i++) {
            if (i == usable_sets[i] - config.set_offset) {
                correct++;
            }
        }

        printf_color(config.color_output,
                     "\n[g][ # ][/g] Covert channels setup finished. [g]%.2f%%[/g] of the channels are established, your system is [r]%s[/r][y]%s[/y].\n\n",
                     (correct * 100.0 / config.channels), correct ? "[ V U L N E R A B L E ]" : "",
                     correct ? "" : "[ maybe not vulnerable ]");
#if 0
        uint8_t bits[] = { 0, 0, 1, 1, 0, 1 };
        if (send_bits_len > 0) {
            printf("sending [ ");
            for (int i = 0; i < send_bits_len; i++) {
                printf("%d ", send_bits[i]);
            }
            printf("]\n");
            uint8_t bits1[] = { 0, 1, 0, 0, 1, 1 };
            uint8_t bits2[] = { 1, 0, 0, 1, 1, 0 };
            uint8_t bits3[] = { 1, 0, 1, 0, 0, 1 };
            //jag_send_bits(&config, send_bits, send_bits_len);
            jag_send_bits(&config, bits1, sizeof(bits1));
            jag_send_bits(&config, bits2, sizeof(bits2));
            jag_send_bits(&config, bits3, sizeof(bits3));
        }
#endif
        //const uint8_t data[] = { 'A', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!' };
        //const uint8_t data[strlen(message) + 1];
        //uint8_t data[1024];
        //for (int i = 0; i < 1024; i++) {
        //    data[i] = (uint8_t)(((i * i) >> 8) ^ (i * (i + 3)));
        //}
        int data_len = strlen(message) + 1;
        print_message_data((const uint8_t*)message, data_len, "Sending");
        time_t t1 = time(NULL);
        send_packet(&config, (const uint8_t*)message, data_len);
        time_t t2 = time(NULL);
        printf("Sent %d bytes for %d seconds, speed = %g bytes/sec.\n", (int)data_len, (int)(t2 - t1), (double)data_len / (t2 - t1));
    } else {
        printf_color(config.color_output, "[y]Receive mode[/y]: start [c]listening[/c]...\n\n");
        printf_color(config.color_output, "[g][ # ][/g] Receiving sets...\n");
        watchdog_reset(config.watchdog);
        jag_receive(addrs, usable_sets, &config, receive_callback);
        if(verbose) {
            printf_color(config.color_output, "[y]Eviction sets[/y]:\n");
            print_eviction_sets(addrs, &config);
        }

        printf_color(config.color_output, "\n[g][ # ][/g] Reconstructing mapping...\n");
        for (int i = 0; i < config.channels; i++) {
            printf_color(config.color_output, "[g][ + ][/g]   Sender[[m]%d[/m]] -> Receiver[[c]%zd[/c]]\n", i,
                         usable_sets[i]);
        }

        printf_color(config.color_output, "\n[y]Test mode[/y]: start [m]probing[/m]...\n\n");
        watchdog_reset(config.watchdog);
        sleep(1);
        for (int i = 0; i < 15; i++) {
            watchdog_reset(config.watchdog);
            sched_yield();
        }

        watchdog_reset(config.watchdog);
        void *old_addr = config.addr;
        config.addr = addrs; // send back the received addresses
        jag_send(&config, probe_callback);
        config.addr = old_addr; // bit of a hack to prevent double free and memory leak...
        watchdog_done(config.watchdog);
        printf_color(config.color_output, "\n[g][ # ][/g] Covert channels setup succeed!\n\n");

        //sleep(3); /* wait for sender */
        //printf("Press any key to start receiving...");
        //getchar();
        /*
        uint8_t dst_buf[config.channels];
        memset(dst_buf, 0, sizeof(dst_buf));
        printf("receive #1 started.\n");
        jag_receive_bits(dst_buf, usable_sets, &config);
        jag_receive_bits(dst_buf, usable_sets, &config);
        jag_receive_bits(dst_buf, usable_sets, &config);
        */
        uint8_t dst_buf[4096];
        time_t t1 = time(NULL);
        int data_size = receive_packet(&config, usable_sets, dst_buf, sizeof(dst_buf));
        time_t t2 = time(NULL);
        printf("Received %d bytes for %d seconds, speed = %g bytes/sec.\n", (int)data_size, (int)(t2 - t1), (double)data_size / (t2 - t1));
        print_message_data(dst_buf, data_size, "Received");
        
#if 0
        printf("receive #2 started.\n");
        jag_receive_bits(dst_buf, usable_sets, &config);
#endif
       
#if 0
        printf("received: [ ");
        for (int i = 0; i < data_size; i++) {
            printf("%x ", (unsigned)dst_buf[i]);
        }
        printf("]\n");
#endif
        printf("Transfer finished.\n");

    }

    ERROR_ON(!jag_free(&config), ERROR_UNMAP_FAILED, config.color_output);
    free(usable_sets);
    free(addrs);

    return 0;
}


void show_welcome(cjag_config_t *config) {
    printf_color(config->color_output,
                 "\n___[y]/\\/\\/\\/\\/\\[/y]__________[r]/\\/\\[/r]______[r]/\\/\\[/r]________[r]/\\/\\/\\/\\/\\[/r]_\n");
    printf_color(config->color_output,
                 "_[y]/\\/\\[/y]__________________[r]/\\/\\[/r]____[r]/\\/\\/\\/\\[/r]____[r]/\\/\\[/r]_________\n");
    printf_color(config->color_output,
                 "_[y]/\\/\\[/y]__________________[r]/\\/\\[/r]__[r]/\\/\\[/r]____[r]/\\/\\[/r]__[r]/\\/\\[/r]__[r]/\\/\\/\\[/r]_\n");
    printf_color(config->color_output,
                 "_[y]/\\/\\[/y]__________[r]/\\/\\[/r]____[r]/\\/\\[/r]__[r]/\\/\\/\\/\\/\\/\\[/r]__[r]/\\/\\[/r]____[r]/\\/\\[/r]_\n");
    printf_color(config->color_output,
                 "___[y]/\\/\\/\\/\\/\\[/y]____[r]/\\/\\/\\/\\[/r]____[r]/\\/\\[/r]____[r]/\\/\\[/r]____[r]/\\/\\/\\/\\/\\[/r]_\n");
    printf_color(config->color_output, "________________________________________________________\n\n");
}

void show_parameters(cjag_config_t *config) {
    int size = config->cache_size / 1024;
    const char *unit = "KB";
    if (size >= 1024 && size % 1024 == 0) {
        size /= 1024;
        unit = "MB";
    }
    printf_color(config->color_output, "      %10s %10s %10s %10s\n", "Size", "Ways", "Slices", "Threshold");
    printf_color(config->color_output, "L3    [g]%7d %s[/g] [g]%9d[/g]  [g]%7d[/g]  [g]%9d[/g]\n\n", size, unit,
                 config->cache_ways, config->cache_slices, config->cache_miss_threshold);
}

void show_usage(char *binary, cjag_config_t *config) {
    printf_color(config->color_output, "[y]USAGE[/y]\n  %s [g][options][/g] \n\n", binary);
    getopt_arg_t null_option = {0};
    size_t count = 0;
    printf_color(config->color_output, "\n[y]OPTIONS[/y]\n");
    do {
        if (!memcmp((void *) &options[count], (void *) &null_option, sizeof(getopt_arg_t))) {
            break;
        } else if (options[count].description) {
            printf_color(config->color_output, "  [g]-%c[/g]%s[y]%s[/y]%s [g]%s%s%s[/g][y]%s[/y]\n     ",
                         options[count].val,
                         options[count].has_arg != no_argument ? " " : "",
                         options[count].has_arg != no_argument ? options[count].arg_name : "",
                         options[count].name ? "," : "", options[count].name ? "--" : "", options[count].name,
                         options[count].has_arg != no_argument ? "=" : "",
                         options[count].has_arg != no_argument ? options[count].arg_name : "");
            printf_color(config->color_output, options[count].description);
            printf_color(config->color_output, "\n\n");
        }
        count++;
    } while (1);
    printf("\n");
    printf_color(config->color_output,
                 "[y]EXAMPLE[/y]\n  Start [m]%s[/m] and [c]%s --receive[/c] in two different terminals (or on two co-located VMs).\n\n\n",
                 binary, binary);
    printf_color(config->color_output,
                 "[y]AUTHORS[/y]\n  Written by Michael Schwarz, Lukas Giner, Daniel Gruss, Clémentine Maurice and Manuel Weber.\n\n");

}

void send_callback(cjag_config_t *config, int set) {
    watchdog_reset(config->watchdog);
    printf_color(config->color_output, "[g][ + ][/g]   ...set #%d\n", set);
}

void probe_callback(cjag_config_t *config, int set) {
    watchdog_reset(config->watchdog);
    printf_color(config->color_output, "[g][ + ][/g] Probing set #%d ([[c]%d[/c]] -> [[m]%d[/m]])\n", set,
                 usable_sets[set - 1], set - 1);
}

void receive_callback(cjag_config_t *config, int set) {
    watchdog_reset(config->watchdog);
    printf_color(config->color_output, "[g][ + ][/g]   ...set #%d\n", set);
}

void watch_callback(cjag_config_t *config, int set) {
    watchdog_reset(config->watchdog);
    printf_color(config->color_output, "[g][ + ][/g]   Sender[[m]%d[/m]] <-> Sender[[c]%zd[/c]] [g]%s[/g][r]%s[/r]\n",
                 set - 1, usable_sets[set - 1] - config->set_offset,
                 (set - 1 == usable_sets[set - 1] - config->set_offset) ? "[ OK ]" : "",
                 (set - 1 != usable_sets[set - 1] - config->set_offset) ? "[FAIL]" : "");
}

void timeout(void *arg) {
    cjag_config_t *config = (cjag_config_t *) arg;

    printf_color(config->color_output,
                 ERROR_TAG "Timeout after [g]%d seconds[/g].\n        Please [y]try to restart[/y] the application.\n\n",
                 config->timeout);
    exit(1);
}

void print_eviction_sets(void** addr, cjag_config_t* config) {
    for (int i = 0; i < config->channels; i++) {
        printf_color(config->color_output, "[g][ . ] Set #%d[/g]\n", i + 1);
        for (int j = 0; j < config->cache_ways; j++) {
            void* v_addr = GET_EVICTION_SET(addr, i, config)[j];
            printf_color(config->color_output, "  %p\n", v_addr);
        }
    }
}

/* prints data either as string or as hex data */
void print_message_data(const uint8_t *data, int data_len, const char *action_description) {
    const char *message = (const char*)data;

    printf("%s data: ", action_description);
    int is_string = 1;
    for (int i = 0; i < data_len; i++) {
        if (!isascii(message[i]) || ((i == data_len - 1) && message[i] != 0)) {
            is_string = 0;
            break;
        }
    }
    if (is_string) {
        printf("\"%s\"\n", (const char*)data);
    }
    else {
        printf("[ ");
        for (size_t i = 0; i < data_len; i++) {
            printf("0x%02x ", (unsigned)data[i]);
        }
        printf("]\n");
    }
}

#if 0
int string_to_bits(uint8_t **dst_ptr, const char *str) {
    int len      = -1;
    uint8_t *dst = NULL;
    if (!str) {
        goto finalize;
    }

    len = strlen(str);
    dst = (uint8_t*)malloc(len * sizeof(uint8_t));
    for (int i = 0; i < len; i++) {
        switch (str[i]) {
            case '0': {
                dst[i] = 0;
            }; break;
            case '1': {
                dst[i] = 1;
            }; break;
            default: {
                len = -1;
                goto finalize;
            }
        }
    }

finalize:
    if (len < 0) {
        if (dst) {
            free(dst);
            dst = NULL;
        }
    }
    *dst_ptr = dst;
    return len;
}
#endif

