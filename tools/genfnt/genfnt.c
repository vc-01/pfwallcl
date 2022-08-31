/*
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define DIGIT_WIDTH_PX 32
#define COLON_WIDTH_PX 8
#define FNT_HEIGHT_PIXEL 36
#define RADIX10_DIGITS 10
#define BITS_PER_BYTE 8

#define BITMAP_PIXELARRAY_OFFSADDR 0x0a
#define BITMAP_PIXELARRAY_ROWSIZE_MULTIPLE_B 4

uint8_t   digit_pixarray[RADIX10_DIGITS]
    [FNT_HEIGHT_PIXEL][DIGIT_WIDTH_PX / BITS_PER_BYTE];
uint8_t   colon_pixarray[FNT_HEIGHT_PIXEL]
    [COLON_WIDTH_PX / BITS_PER_BYTE];

const size_t bitmap_pixarray_row_pad_b =
    BITMAP_PIXELARRAY_ROWSIZE_MULTIPLE_B -
    ( sizeof( digit_pixarray[0] ) * sizeof( digit_pixarray[0][0] ) +
      sizeof( colon_pixarray[0] ) ) % BITMAP_PIXELARRAY_ROWSIZE_MULTIPLE_B;

void
print_usage( char *prog_name ) {
    printf( "Usage: %s <fnt_bmp_file> <fnt_tasm_file>\n", prog_name );
}

int
main( int argc, char **argv ) {
    int       fd_inpf, fd_outpf;
    FILE     *stream_outpf;

    if ( argc < 3 )
    {
        print_usage( argv[0] );
        return EXIT_FAILURE;
    }

    fd_inpf = open( argv[1], O_RDONLY, NULL );
    if ( fd_inpf == -1 )
    {
        perror( "open input file" );
        return EXIT_FAILURE;
    }

    fd_outpf =
        open( argv[2], O_CREAT | O_TRUNC | O_WRONLY,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
    if ( fd_outpf == -1 )
    {
        perror( "open output file" );
        return EXIT_FAILURE;
    }

    stream_outpf = fdopen( fd_outpf, "w" );
    if ( stream_outpf == NULL )
    {
        perror( "open output file stream" );
        return EXIT_FAILURE;
    }

    union ret_t {
        int       ret_i;
        ssize_t   ret_ssize;
        off_t     ret_off;
    } ret;

    char      bitmap_header_ident[2];

    ret.ret_ssize = read( fd_inpf,
                          bitmap_header_ident,
                          sizeof( bitmap_header_ident ) );
    if ( ret.ret_ssize == -1 )
    {
        perror( "read input file" );
        return EXIT_FAILURE;
    }
    else if ( ret.ret_ssize != sizeof( bitmap_header_ident ) )
    {
        fprintf(stderr, "err: read bitmap_header_ident\n" );
        return EXIT_FAILURE;
    }

    if ( bitmap_header_ident[0] != 'B' || bitmap_header_ident[1] != 'M' )
    {
        printf( "Input file is not a bitmap (BMP) file.\n" );
        return EXIT_FAILURE;
    }

    ret.ret_off = lseek( fd_inpf, BITMAP_PIXELARRAY_OFFSADDR, SEEK_SET );
    if ( ret.ret_off == -1 )
    {
        perror( "lseek BITMAP_PIXELARRAY_OFFSADDR" );
        return EXIT_FAILURE;
    }

    uint32_t  pixelarray_offset;

    ret.ret_ssize =
        read( fd_inpf, &pixelarray_offset, sizeof( pixelarray_offset ) );
    if ( ret.ret_ssize == -1 )
    {
        perror( "read pixelarray_offset" );
        return EXIT_FAILURE;
    }
    else if ( ret.ret_ssize != 4 )
    {
        fprintf(stderr, "err: read pixelarray_offset, expect 4 bytes" );
        return EXIT_FAILURE;
    }

    ret.ret_off = lseek( fd_inpf, pixelarray_offset, SEEK_SET );
    if ( ret.ret_off == -1 )
    {
        perror( "lseek pixelarray_offset" );
        return EXIT_FAILURE;
    }

    // Read in the input data

    // loop over pixelarray lines (y-axis inverted)
    for ( int line_i = FNT_HEIGHT_PIXEL - 1; line_i >= 0; line_i-- )
    {
        // loop over digits
        for ( int digit_i = 0; digit_i < RADIX10_DIGITS; digit_i++ )
        {
            ret.ret_ssize = read( fd_inpf,
                                  digit_pixarray[digit_i][line_i],
                                  sizeof( digit_pixarray[digit_i][line_i] ) );
            if ( ret.ret_ssize == -1 )
            {
                perror( "read input file" );
                return EXIT_FAILURE;
            }
            else if ( ret.ret_ssize !=
                      sizeof( digit_pixarray[digit_i][line_i] ) )
            {
                fprintf(stderr, "err: read record from input file" );
                return EXIT_FAILURE;
            }
        }

        // the colon character
        ret.ret_ssize = read( fd_inpf,
                              colon_pixarray[line_i],
                              sizeof( colon_pixarray[line_i] ) );
        if ( ret.ret_ssize == -1 )
        {
            perror( "read input file" );
            return EXIT_FAILURE;
        }
        else if ( ret.ret_ssize != sizeof( colon_pixarray[line_i] ) )
        {
            fprintf(stderr, "err: read record from input file" );
            return EXIT_FAILURE;
        }

        ret.ret_off = lseek( fd_inpf, bitmap_pixarray_row_pad_b, SEEK_CUR );
        if ( ret.ret_off == -1 )
        {
            perror( "lseek bitmap_pixarray_row_pad_b" );
            return EXIT_FAILURE;
        }
    }

    // Write out the output
    printf( "\t.MODEL small\n" );
    fprintf( stream_outpf, "\t.MODEL small\n" );
    printf( "\t.DATA\n" );
    fprintf( stream_outpf, "\t.DATA\n" );
    printf( "\tPUBLIC _digits_pixeldata_laligned\n" );
    fprintf( stream_outpf, "\tPUBLIC _digits_pixeldata_laligned\n" );
    printf( "\tPUBLIC _colon_pixeldata\n" );
    fprintf( stream_outpf, "\tPUBLIC _colon_pixeldata\n" );
    printf( "\n" );
    fprintf( stream_outpf, "\n" );

    // loop over digits
    printf( "_digits_pixeldata_laligned LABEL DWORD\n" );
    fprintf( stream_outpf, "_digits_pixeldata_laligned LABEL DWORD\n" );
    for ( int digit_i = 0; digit_i < RADIX10_DIGITS; digit_i++ )
    {
        printf( "; digit %d\n", digit_i );
        fprintf( stream_outpf, "; digit %d\n", digit_i );

        // loop over pixelarray lines
        for ( int line_i = 0; line_i < FNT_HEIGHT_PIXEL; line_i++ )
        {
            printf( "\tDB " );
            fprintf( stream_outpf, "\tDB " );

            // loop over pixelarray line bytes
            for ( int line_b_i = 0;
                  line_b_i < DIGIT_WIDTH_PX / BITS_PER_BYTE; line_b_i++ )
            {
                // print out byte in radix 2 format
                for ( uint8_t mask =
                      1 << sizeof( uint8_t ) * BITS_PER_BYTE - 1;
                      mask > 0; mask >>= 1 )
                {
                    if ( digit_pixarray[digit_i][line_i][line_b_i] & mask )
                    {
                        printf( "1" );
                        fprintf( stream_outpf, "1" );
                    }
                    else
                    {
                        printf( "0" );
                        fprintf( stream_outpf, "0" );
                    }
                }

                printf( "b" );
                fprintf( stream_outpf, "b" );

                if (line_b_i < DIGIT_WIDTH_PX / BITS_PER_BYTE - 1) {
                    printf( "," );
                    fprintf( stream_outpf, "," );
                }
            }

            printf( " " );
            fprintf( stream_outpf, "" );

            // loop over pixelarray line bytes
            for ( int line_b_i = 0;
                  line_b_i < DIGIT_WIDTH_PX / BITS_PER_BYTE; line_b_i++ )
            {
                // print out in hex format (informative output only)
                printf( "%.2x", digit_pixarray[digit_i][line_i][line_b_i] );

                if ( line_b_i < DIGIT_WIDTH_PX / BITS_PER_BYTE - 1 )
                {
                    printf( "-" );
                }
            }

            printf( "\n" );
            fprintf( stream_outpf, "\n" );
        }

        printf( "\n" );
        fprintf( stream_outpf, "\n" );
    }

    // print out the colon character
    printf( "_colon_pixeldata LABEL DWORD\n" );
    fprintf( stream_outpf, "_colon_pixeldata LABEL DWORD\n " );

    // loop over pixelarray lines
    for ( int line_i = 0; line_i < FNT_HEIGHT_PIXEL; line_i++ )
    {
        printf( "\tDB " );
        fprintf( stream_outpf, "\tDB " );

        // loop over pixelarray line bytes
        for ( int line_b_i = 0;
              line_b_i < COLON_WIDTH_PX / BITS_PER_BYTE; line_b_i++ )
        {
            // print out in binary format
            for ( uint8_t mask =
                  1 << sizeof( uint8_t ) * BITS_PER_BYTE - 1;
                  mask > 0; mask >>= 1 )
            {
                if ( colon_pixarray[line_i][line_b_i] & mask )
                {
                    printf( "1" );
                    fprintf( stream_outpf, "1" );
                }
                else
                {
                    printf( "0" );
                    fprintf( stream_outpf, "0" );
                }
            }
        }

        printf( "b " );
        fprintf( stream_outpf, "b" );

        // loop over pixelarray line bytes
        for ( int line_b_i = 0;
              line_b_i < COLON_WIDTH_PX / BITS_PER_BYTE; line_b_i++ )
        {
            // print out in hex format (informative output only)
            printf( "%.2x", colon_pixarray[line_i][line_b_i] );

            if ( line_b_i < COLON_WIDTH_PX / BITS_PER_BYTE - 1 )
            {
                printf( "-" );
            }
        }

        printf( "\n" );
        fprintf( stream_outpf, "\n" );
    }

    printf( "\nEND\n" );
    fprintf( stream_outpf, "\nEND\n" );

    ret.ret_i = fclose( stream_outpf );
    if ( ret.ret_i == EOF )
    {
        perror( "close output file stream" );
        return EXIT_FAILURE;
    }

    ret.ret_i = close( fd_inpf );
    if ( ret.ret_i == -1 )
    {
        perror( "close input file" );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
