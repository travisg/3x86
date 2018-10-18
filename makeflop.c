/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

void usage(char const *progname) {
    printf("usage: %s [-p #padding] [-v] [-x xres ] [-y yres] [-d bitdepth] bootblock payload outfile\n", progname);
}

int main(int argc, char *argv[]) {
    struct stat st;
    int err;
    unsigned int blocks;
    unsigned char bootsector[1024];
    unsigned char buf[512];
    size_t read_size;
    size_t written_bytes;
    int padding;
    int enable_vesa;
    int vesa_x, vesa_y, vesa_bit;
    int infd;
    int outfd;
    signed char opt;
    char const *progname;

    padding = 0;
    progname = argv[0];
    enable_vesa = 0;
    vesa_x = 640;
    vesa_y = 480;
    vesa_bit = 16;

    while ( (opt = getopt(argc, argv, "p:vx:y:d:")) != -1)
        switch (opt) {
            case 'p':
                padding= atoi(optarg);
                if (padding < 0) {
                    usage(progname);
                    return -1;
                }
                break;
            case 'v':
                enable_vesa = 1;
                break;
            case 'x':
                vesa_x = atoi(optarg);
                if (vesa_x < 0) {
                    usage(progname);
                    return -1;
                }
                break;
            case 'y':
                vesa_y = atoi(optarg);
                if (vesa_y < 0) {
                    usage(progname);
                    return -1;
                }
                break;
            case 'd':
                vesa_bit = atoi(optarg);
                if (vesa_bit < 0) {
                    usage(progname);
                    return -1;
                }
                break;
            case '?':
            default:
                usage(progname);
                return -1;
        }
    argc -= optind - 1;
    argv += optind - 1;

    if (argc < 4) {
        fprintf(stderr, "insufficient args\n");
        usage(progname);
        return -1;
    }

    err = stat(argv[2], &st);
    if (err < 0) {
        fprintf(stderr, "error stating file '%s'\n", argv[2]);
        return -1;
    }

    outfd = open(argv[3], O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (outfd < 0) {
        fprintf(stderr, "error: cannot open output file '%s'\n", argv[3]);
        return -1;
    }

    // first read the bootblock
    infd = open(argv[1], O_BINARY|O_RDONLY);
    if (infd < 0) {
        fprintf(stderr, "error: cannot open bootblock file '%s'\n", argv[1]);
        return -1;
    }
    if (read(infd, bootsector, sizeof(bootsector)) < sizeof(bootsector)
            || lseek(infd, 0, SEEK_END) != sizeof(bootsector)) {
        printf ("error: size of bootblock file '%s' must match %zu bytes.\n", argv[1], sizeof(bootsector));
        return -1;
    }
    close(infd);

    // patch the size of the output into bytes 3 & 4 of the bootblock
    blocks = st.st_size / 512;
    if ((st.st_size % 512) != 0) {
        blocks++;
    }
    printf("size %llu, blocks %d (size %d)\n", (unsigned long long)st.st_size, blocks, blocks * 512);
    bootsector[2] = (blocks & 0x00ff);
    bootsector[3] = (blocks & 0xff00) >> 8;

    if (enable_vesa) {
        // patch the vesa mode stuff in the bootsector (first few bytes of the second sector)
        bootsector[512] = 1;
        bootsector[513] = (vesa_x & 0x00ff);
        bootsector[514] = (vesa_x & 0xff00) >> 8;
        bootsector[515] = (vesa_y & 0x00ff);
        bootsector[516] = (vesa_y & 0xff00) >> 8;
        bootsector[517] = vesa_bit;
    }

    ssize_t written = write(outfd, bootsector, sizeof(bootsector));
    if (written != sizeof(bootsector)) {
        fprintf(stderr, "error writing to output file\n");
        return 1;
    }
    written_bytes = written;

    infd = open(argv[2], O_BINARY|O_RDONLY);
    if (infd < 0) {
        fprintf(stderr, "error: cannot open input file '%s'\n", argv[1]);
        return -1;
    }

    while ((read_size = read(infd, buf, sizeof(buf))) > 0) {
        written = write(outfd, buf, read_size);
        if (written != read_size) {
            fprintf(stderr, "error writing to output file\n");
            return 1;
        }
        written_bytes += written;
    }

    if (padding) {
        if (written_bytes % padding) {
            size_t towrite = padding - written_bytes % padding;
            unsigned char *buf = malloc(towrite);

            memset(buf, 0, towrite);
            ssize_t written = write(outfd, buf, towrite);
            if (written != towrite) {
                fprintf(stderr, "error writing to output file\n");
                return 1;
            }
            written_bytes += written;

            printf("output file padded to %zu\n", written_bytes);
        }
    }

    close(outfd);
    close(infd);

    return 0;
}

