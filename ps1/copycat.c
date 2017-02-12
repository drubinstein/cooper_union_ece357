#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int wr_to_fd(int fd, char *buff, int len) {
    int n_written = 0;
    if ((n_written = write(fd, buff, len)) < 0)
        fprintf(stderr, "Error: Failed to write to output with error: %s.\n", strerror(errno));
    return n_written;
}

int wr_to_buff_and_flush(int ifd, int ofd, char *buff, int *used, int buff_len) {
    int n_read = 0, n_written = 0;
    do {
        n_read = read(ifd, buff + *used, buff_len - *used);
        if (n_read < -1){
            fprintf(stderr, "Error: Failed to read from input file with error %s.\n", strerror(errno));
            return -1;
        }
        else if (n_read > 0) {
            *used += n_read;
            if(*used == buff_len) {
                if ((n_written = wr_to_fd(ofd, buff, buff_len) < 0)) return -1;
                *used -= buff_len;
                memmove(buff, buff + n_written, buff_len - n_written);
            }
        }
    } while(n_read > 0);
    return 0;
}

int main(int argc, char **argv) {

    //variables related to internal buffer handling
    const int STDIN = 0, STDOUT = 1, STDERR = 2;
    int buffer_size = 256;
    char *buff = NULL;
    int used_buff_bytes = 0;

    //file descriptor related
    char *outfile = NULL;
    int ofd = STDOUT, ifd = STDIN;
    int index;
    int n_written, n_read;

    //get arguments
    int c;
    while ((c = getopt(argc, argv, "b:o:")) != -1)
        switch (c) {
            //buffer
            case 'b':
                buffer_size = atoi(optarg);
                if(buffer_size > 1<<30) {
                    fprintf(stderr, "Error: Buffer size must be less than %d.\n", 1<<30);
                    return -1;
                }
                break;
            //output file
            case 'o':
                outfile = optarg;
                break;
            //everything else
            case '?':
                if (optopt == 'b' || optopt == 'o') {
                    fprintf(stderr, "Usage Error: Option -%c requires an argument.\n", optopt);
                }
                return -1;
                break;
            default:
                fprintf(stderr, "Usage: [-b ###] [-o outfile] infile1 [...infile2...]");
                return -1;
        }

    //allocate the copy buffer
    buff = malloc(buffer_size);
    if(buff == NULL) {
        fprintf(stderr, "Error: Failed to allocate %d bytes for the copy buffer.\n", buffer_size);
        return -1;
    }

    //check if output can be written to
    if(outfile != NULL){
        if((ofd = open(outfile, O_WRONLY | O_CREAT)) == -1) {
            fprintf(stderr, "Error: Failed to open output file %s with error: %s.\n", outfile, strerror(errno));
            return -1;
        }
    }

    //process all input files. If there are none, take from stdin
    if(optind < argc)
        for(int index = optind; index < argc; ++index) {
            if ((ifd = open(argv[optind], O_RDONLY)) == -1) {
                fprintf(stderr, "Error: Failed to open input file %s with error: %s.\n", argv[optind], strerror(errno));
                return -1;
            }

            if(wr_to_buff_and_flush(ifd, ofd, buff, &used_buff_bytes, buffer_size) == -1) return -1;

            if(close(ifd) == -1) {
                fprintf(stderr, "Error closing input file %s.\n", argv[optind]);
                return -1;
            }
        }
    else if (wr_to_buff_and_flush(STDIN, ofd, buff, &used_buff_bytes, buffer_size) == -1 ) return -1;

    //write out any remaining bytes in the buffer
    if ((n_written = wr_to_fd(ofd, buff, used_buff_bytes) < 0)) return -1;

    free(buff);
    if (ofd != STDOUT)
        close(ofd);
}
