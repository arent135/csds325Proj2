/*
Alex Renteria
abr92
proj2.c
10/1/2026

This project acts a router that can either forward a packet table, dump packets, or 
simulate a router, sending each packet to the correct interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/ip.h>


//modes
int forward_mode = 0;
int dump_mode = 0;
int sim_mode = 0;

//files
char *forwarding_table = NULL;
char *packet_trace = NULL;

//forward table struct
typedef struct forward_table{
    unsigned short vers;
    unsigned short interface;
    unsigned int addr;
} ForwardTable;

void usage(char *progname){
    printf("Usage: %s [-h]\n", progname);
    exit(0);
}

void parseargs(int argc, char *argv[]){
    int opt;
    while((opt = getopt(argc, argv, "fdrt:p:")) != -1){
        switch(opt){
            case 'f':
                forward_mode = 1;
                break;
            case 'd':
                dump_mode = 1;
                break;
            case 'r':
                sim_mode = 1;
                break;
            case 't':
                forwarding_table = optarg;
                break;
            case 'p':
                packet_trace = optarg;
                break;
            default:
                fprintf(stderr, "Invalid option.");
                exit(1);
        }
    }

    //check to make sure the modes all work
    if(forward_mode + dump_mode + sim_mode > 1){
        fprintf(stderr, "Only select one mode.\n");
        exit(1);
    }

    if(forward_mode && forwarding_table == NULL){
        fprintf(stderr, "Need to attach forwarding table for Forward Printing Mode\n");
        exit(1);
    }

    if((dump_mode || sim_mode) && packet_trace == NULL){
        fprintf(stderr, "Need to attach packet trace for Dump or Simulate Mode.\n");
        exit(1);
    }
}

int read_route(FILE *file, ForwardTable *route)
{
    unsigned short prefix;
    unsigned short interface;
    unsigned int addr;

    if (fread(&prefix, sizeof(prefix), 1, file) != 1) {
        if (feof(file))
            return 0;

        fprintf(stderr, "Error reading forwarding table.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&interface, sizeof(interface), 1, file) != 1 ||
        fread(&address, sizeof(address), 1, file) != 1) {
        fprintf(stderr, "Error: incomplete forwarding table record.\n");
        exit(EXIT_FAILURE);
    }

    route->prefix = ntohs(vers);
    route->interface = ntohs(interface);
    route->address = ntohl(address);

    return 1;
}

void print_table(char *filename)
{
    FILE *file;
    ForwardTable route;

    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening forwarding table.");
        exit(1);
    }

    while (read_route(file, &route)) {
        print_ip(route.addr);
        printf(" %u %u\n", route.prefix, route.interface);
    }

    fclose(file);
}

int main(int argc,char *argv[]){
    parseargs(argc, argv);

    if(forward_mode == 1){
        forwarding_mode();
    }
    else if(dump_mode == 1){
        printf("Dump Mode\n");
    }
    else if(sim_mode == 1){
        printf("Simulate Mode\n");
    }
    else {
        fprintf(stderr, "No mode selected.\n");
        exit(1);
    }
    printf("Hello World\n");
    return 0;
}