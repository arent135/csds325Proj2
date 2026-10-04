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

//modes
int forward_mode = 0;
int dump_mode = 0;
int sim_mode = 0;

//files
char *forwarding_table = NULL;
char *packet_trace = NULL;

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

    
}

int main(int argc,char *argv[]){
    parseargs(argc, argv);
    printf("Hello World\n");
    return 0;
}