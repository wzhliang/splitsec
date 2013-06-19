/*  
 * Copyright (C) 2008 Lorenzo Pallara, l.pallara@avalpa.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <netinet/in.h>

#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <assert.h>

#define TS_PACKET_SIZE 188
#define MAX_TABLE_LEN 4096
#define MAX_PID 8192
#define SECTION_HEADER_SIZE 3


unsigned char section[MAX_TABLE_LEN];
int table_id_exts[1024];
int section_len = 0;
int table_id = -1;

int find_ext(int ext)
{
	int i;

	for (i = 0; i < 1024 && table_id_exts[i] != -1; i++)
	{
		if (table_id_exts[i] == ext)
		{
			return 1;
		}
	}

	return 0;
}

void add_ext(int ext) {
	int i = 0;

	while (i < 1024 && table_id_exts[i] != -1)
	{
		i++;
	}

	assert( i < 1024 );

	table_id_exts[i] = ext;
}


void dump_section( char *basename, int table_id_ext ) {
	int fd;
	char fn[4096];
	int sec_len;

	sprintf( fn, "%s-%x", basename, table_id_ext );

	memcpy( &sec_len, section + 1, 2 );
	sec_len = ntohs( sec_len );
	sec_len = sec_len & 0x0fff;

	if ( sec_len + 3 != write( STDOUT_FILENO, section, sec_len + 3))
	{
		perror("write");
	}
}

int get_section(int fd_sec, int *table_id_ext) {
	unsigned char sec_header[3];
	int r;
	int sec_len;
	
	r = read( fd_sec, sec_header, 3 );
	if ( r != 3 )
	{
		return -1;
	}

	memcpy( &sec_len, sec_header + 1, 2 );
	sec_len = ntohs( sec_len );
	sec_len = sec_len & 0x0fff;

	memcpy( section, sec_header, 3 );
	r = read( fd_sec, section + 3, sec_len );
	if ( r != sec_len )
	{
		return -1;
	}

	memcpy( table_id_ext, section + 3, 2 );	
	*table_id_ext = ntohs( *table_id_ext );

	return 0;
}

int main(int argc, char *argv[])
{
	int fd_sec;			/* File descriptor of ts file */
	int table_id_ext;
    int st;
    int count = 0;

	/* Open ts file */
	if (argc >= 2) {
		fd_sec = open(argv[1], O_RDONLY);
	} else {
		fprintf(stderr, "Usage: 'splitsec filename.sec'\n");
		fprintf(stderr, "the tool splits multiple section tables into individual sections.\n");
		return 2;
	}
	if (fd_sec < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	
	memset( table_id_exts, -1, 1024 );

	while( 1 ) {
		st = get_section( fd_sec, &table_id_ext );
		if ( st != 0 )
		{
			fprintf(stderr, "File format error. %s\n", argv[1]);
			break;
		}

		if ( table_id_ext == 0x6001 )
		{
			dump_section( argv[1], table_id_ext );
            count ++;
            if ( count == 4 )
            {
                break;
            }
			add_ext( table_id_ext );
		}

		fprintf( stderr, "Found table id ext: %d\n", table_id_ext );
	}

	close( fd_sec );

	return 0;
}

