/*
Alex Renteria
abr92
proj2.c
10/1/2026

This project acts a router that can either forward a packet table, dump packets, or 
simulate a router, sending each packet to the correct interface.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <stdio.h>

//constants
#define SUCCESS 1

typedef struct {
    unsigned short prefix;
    unsigned short interface;
    unsigned int addr;
} Route;

typedef struct {
    unsigned short interface;
    unsigned int seconds;
    unsigned int microseconds;
    struct iphdr ip;
} Packet;

//print the ip address in the correct format
void getIP(unsigned int ip) {
    printf("%u.%u.%u.%u",
           (ip >> 24) & 0xff,
           (ip >> 16) & 0xff,
           (ip >> 8) & 0xff,
           ip & 0xff);
}

//get the route in the forwarding table
int getRoute(FILE *file, Route *route) {
    unsigned short prefix;
    unsigned short interface;
    unsigned int addr;

    if (fread(&prefix, sizeof(prefix), 1, file) != 1) {
        if (feof(file))
            return 0;

        fprintf(stderr, "Error reading forwarding table.\n");
        exit(1);
    }

    if (fread(&interface, sizeof(interface), 1, file) != 1 ||
        fread(&addr, sizeof(addr), 1, file) != 1) {
        fprintf(stderr, "Error: incomplete forwarding table record.\n");
        exit(1);
    }

    route->prefix = ntohs(prefix);
    route->interface = ntohs(interface);
    route->addr = ntohl(addr);

    return SUCCESS;
}

//print the forwarding table
void forwardMode(char *filename) {
    FILE *file;
    Route route;

    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening forwarding table");
        exit(1);
    }

    while (getRoute(file, &route)) {
        getIP(route.addr);
        printf(" %u %u\n", route.prefix, route.interface);
    }

    fclose(file);
}

//get the packet from the packet trace
int getPacket(FILE *file, Packet *packet) {
    unsigned short interface;
    unsigned short padding;
    unsigned int seconds;
    unsigned int microseconds;

    if (fread(&interface, sizeof(interface), 1, file) != 1) {
        if (feof(file))
            return 0;

        fprintf(stderr, "Error reading packet trace.\n");
        exit(1);
    }

    if (fread(&padding, sizeof(padding), 1, file) != 1 ||
        fread(&seconds, sizeof(seconds), 1, file) != 1 ||
        fread(&microseconds, sizeof(microseconds), 1, file) != 1 ||
        fread(&packet->ip, sizeof(packet->ip), 1, file) != 1) {
        fprintf(stderr, "Error: incomplete packet record.\n");
        exit(1);
    }

    packet->interface = ntohs(interface);
    packet->seconds = ntohl(seconds);
    packet->microseconds = ntohl(microseconds);

    return SUCCESS;
}

//print every packet in the trace file
void dumpMode(char *filename) {
    FILE *file;
    Packet packet;
    unsigned int source;
    unsigned int destination;

    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening packet trace");
        exit(1);
    }

    while (getPacket(file, &packet))
    
    {
        source = ntohl(packet.ip.saddr);
        destination = ntohl(packet.ip.daddr);

        printf("%u.%06u %u ",
               packet.seconds,
               packet.microseconds,
               packet.interface);

        getIP(source);
        printf(" ");

        getIP(destination);

        printf(" %u %u %c\n",
               packet.ip.ttl,
               packet.ip.protocol,
               ntohs(packet.ip.check) == 54321 ? 'C' : 'I');
    }

    fclose(file);
}

//returns whether the address matches the prefix
int matches(unsigned int addr1, unsigned int addr2) {
    return (addr1 >> 24) == (addr2 >> 24);
}

int findInterface(Route routes[], int route_count, unsigned int destination) {
    int i;

    for (i = 0; i < route_count; i++) {
        if (matches(destination, routes[i].addr))
            return routes[i].interface;
    }

    return -1;
}

//get the routes
Route *getRoutes(char *filename, int *route_count) {
    FILE *file;
    Route *routes;
    int capacity = 16;

    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening forwarding table");
        exit(1);
    }

    routes = malloc(capacity * sizeof(Route));
    if (routes == NULL) {
        fprintf(stderr, "Error: unable to allocate memory.\n");
        exit(1);
    }

    *route_count = 0;

    while (getRoute(file, &routes[*route_count])) {
        //check for duplicate prefixes
        if (*route_count > 0 &&
            findInterface(routes, *route_count, routes[*route_count].addr) != -1) {
            fprintf(stderr, "Error: duplicate IPv4 prefix in forwarding table.\n");
            free(routes);
            fclose(file);
            exit(1);
        }

        (*route_count)++;

        if (*route_count == capacity) {
            Route *new_routes;

            capacity *= 2;
            new_routes = realloc(routes, capacity * sizeof(Route));

            if (new_routes == NULL) {
                fprintf(stderr, "Error: unable to allocate memory.\n");
                free(routes);
                fclose(file);
                exit(1);
            }

            routes = new_routes;
        }
    }

    fclose(file);
    return routes;
}

void simMode(char *table_filename, char *packet_filename) {
    FILE *file;
    Route *routes;
    Packet packet;
    int route_count;
    int interface;

    routes = getRoutes(table_filename, &route_count);

    file = fopen(packet_filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening packet trace");
        free(routes);
        exit(1);
    }

    while (getPacket(file, &packet)) {
        unsigned int destination;

        printf("%u.%06u ", packet.seconds, packet.microseconds);

        if (ntohs(packet.ip.check) != 54321) {
            printf("drop checksum\n");
            continue;
        }

        if (packet.ip.ttl == 1) {
            printf("drop expired\n");
            continue;
        }

        destination = ntohl(packet.ip.daddr);
        interface = findInterface(routes, route_count, destination);

        if (interface == -1) {
            interface = findInterface(routes, route_count, 0);

            if (interface == -1) {
                printf("drop unknown\n");
            } else if (interface == packet.interface) {
                printf("drop loop %d\n", interface);
            } else if (interface == 0) {
                printf("drop policy\n");
            } else {
                printf("default %d\n", interface);
            }

            continue;
        }

        if (interface == packet.interface) {
            printf("drop loop %d\n", interface);
        } else if (interface == 0) {
            printf("drop policy\n");
        } else {
            printf("send %d\n", interface);
        }
    }

    fclose(file);
    free(routes);
}

void usage(char *program)
{
    fprintf(stderr,
            "Usage: %s [-d] [-f] [-r] [-t forwarding_table] "
            "[-p trace_file]\n",
            program);
}

int main(int argc, char *argv[]) {
    int opt;
    int mode = 0;
    char *table_filename = NULL;
    char *packet_filename = NULL;

    while ((opt = getopt(argc, argv, "dfrt:p:")) != -1) {
        switch (opt) {
        case 'd':
            if (mode != 0) {
                fprintf(stderr, "Error: multiple modes specified.\n");
                exit(1);
            }
            mode = 'd';
            break;

        case 'f':
            if (mode != 0) {
                fprintf(stderr, "Error: multiple modes specified.\n");
                exit(1);
            }
            mode = 'f';
            break;

        case 'r':
            if (mode != 0) {
                fprintf(stderr, "Error: multiple modes specified.\n");
                exit(1);
            }
            mode = 'r';
            break;

        case 't':
            table_filename = optarg;
            break;

        case 'p':
            packet_filename = optarg;
            break;

        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (mode == 0) {
        fprintf(stderr, "Error: must specify -d, -f, or -r.\n");
        exit(1);
    }

    if ((mode == 'f' || mode == 'r') && table_filename == NULL) {
        fprintf(stderr, "Error: -t is required for this mode.\n");
        exit(1);
    }

    if ((mode == 'd' || mode == 'r') && packet_filename == NULL) {
        fprintf(stderr, "Error: -p is required for this mode.\n");
        exit(1);
    }

    if (mode == 'f')
        forwardMode(table_filename);
    else if (mode == 'd')
        dumpMode(packet_filename);
    else
        simMode(table_filename, packet_filename);

    exit(0);
}