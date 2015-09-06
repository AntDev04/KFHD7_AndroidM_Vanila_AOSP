/*
 * (C) Copyright 2011
 * Texas Instruments, <www.ti.com>
 * Author: Vikram Pandita <vikram.pandita@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <sparse.h>
#include <mmc.h>

// Internal static
typedef struct {
	unsigned int    remaining_chunks;
	unsigned int    chunk_offset;     // offset in the current chunk
	unsigned long   sector;
	chunk_header_t  last_header;
	unsigned int    block_size;
	         int    slot_no;
	unsigned char   data[512];

} sparse_state_t;

static sparse_state_t state;

//#define DEBUG

#define SPARSE_HEADER_MAJOR_VER 1

int mmc_compare(int mmcc, unsigned char *src, unsigned long sector, int len)
{
	u8 data[512];

	while (len > 0) {
		if (mmc_read(mmcc, sector, data, 512) != 1) {
			printf("mmc read error sector %d\n", sector);
			return -1;
		}
		if (memcmp(data, src, 512)) {
			printf("mmc data mismatch sector %d\n", sector);
			return -1;
		}
		len -= 512;
		sector++;
		src += 512;
	}
	return 0;
}


int _unsparse_read_header(unsigned char **source, u64 *section_size)
{
	sparse_header_t *header = (void*) *source;

	printf("sparse: Read header");

	if (sizeof(sparse_header_t) > *section_size) {
		printf("sparse: 1st chunk is smaller than the header");
		return 1;
	}

	if (header->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic\n");
		return 1;
	}

	if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		return 1;
	}
	/* todo: ensure image will fit */

	/* Skip the header now */
	*source       += header->file_hdr_sz;
	*section_size -= header->file_hdr_sz;

        /*save state */
	state.remaining_chunks = header->total_chunks;
	state.chunk_offset     = 0;
	state.block_size       = header->blk_sz;
	return 0;
}

int _unsparse_process_chunk(unsigned char **source, u64 *section_size,
	                    int (*WRITE)(int mwcc, unsigned char *src,
			               unsigned long sector, int len))
{
	int r;

	// If last header has not been completly read, read it
	if(state.chunk_offset < sizeof(chunk_header_t)) {
		int missing_bytes   = sizeof(chunk_header_t) - state.chunk_offset;
		int available_bytes = (*section_size>missing_bytes) ? missing_bytes : *section_size;
		memcpy((void*)&state.last_header + state.chunk_offset, *source, available_bytes);
		state.chunk_offset += available_bytes;

		*source       += available_bytes;
		*section_size -= available_bytes;
	}

	// If last header has been completly read, then process it
	if(state.chunk_offset >= sizeof(chunk_header_t)) {
		switch (state.last_header.chunk_type) {
		case CHUNK_TYPE_RAW:
			if(*section_size > 0) {
				unsigned int chunk_size     = state.last_header.chunk_sz * state.block_size;
				unsigned int chunk_cur_size = state.chunk_offset - sizeof(chunk_header_t);
                                unsigned int chunk_rem_size = chunk_size - chunk_cur_size;
				unsigned int chunk_end      = (*section_size >= chunk_rem_size) ? 1 : 0;
				unsigned int chunk_avl_size = chunk_end ? chunk_rem_size : *section_size;

				// During the last iteration we may have not been able to fill the last sector
				unsigned int prev_rem_data = chunk_cur_size % 512;
				unsigned int missing_bytes = prev_rem_data ? 512 - prev_rem_data : 0;

				if (state.last_header.total_sz != (chunk_size + sizeof(chunk_header_t))) {
					printf("sparse: bad chunk size for current chunk type Raw\n");
					return 1;
				}

				if(missing_bytes > 0) {
					// last time we hadn't enough data to fill a sector
					// fill it now
					if(chunk_avl_size < missing_bytes) {
						printf("sparse: buffer is too small FAIL\n");
						return 1;
					}

					memcpy(state.data + prev_rem_data,
					       *source, missing_bytes);

					WRITE(state.slot_no, state.data, state.sector, 512);

					state.sector   += 1;
					*source        += missing_bytes;
					*section_size  -= missing_bytes;
				}

				// make sure copy_len is a multiple of 512
				unsigned int copy_len = ((chunk_avl_size-missing_bytes)/512)*512;
				unsigned int rem_data = chunk_avl_size - copy_len - missing_bytes;

				WRITE(state.slot_no, *source, state.sector, copy_len);

				state.sector       += (copy_len / 512);
				*source            += copy_len;
				*section_size      -= copy_len;

				// If we have some data which remain keep them for the next itteration
				if(rem_data > 0) memcpy(state.data, *source, rem_data);

				*source            += rem_data;
				*section_size      -= rem_data;

				if(chunk_end) {
					// This chunk is completed
					state.chunk_offset = 0;
					state.remaining_chunks--;
				} else
					state.chunk_offset += chunk_avl_size;
			}
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (state.last_header.total_sz != sizeof(chunk_header_t)) {
				printf("sparse: bogus DONT CARE chunk\n");
				return 1;
			}
			else {
				unsigned long skip = state.last_header.chunk_sz * state.block_size;
				printf("sparse: skip 0x%08X bytes\n", skip);
				state.sector += (skip / 512);

				// This chunk is completed
				state.chunk_offset = 0;
				state.remaining_chunks--;
			}
			break;

		default:
			printf("sparse: unknown chunk ID %04x\n", state.last_header.chunk_type);
			return 1;
		}
	}
	return 0;
}

int _unsparse(unsigned char *source, u64 section_size,
	     int (*WRITE)(int mwcc, unsigned char *src,
			            unsigned long sector, int len))
{
	int ret = 0;	

	// read the header if it has not yet been read
	if(state.remaining_chunks == 0) ret = _unsparse_read_header(&source, &section_size);
	if(ret) return ret;

	// process as many chunks as possible
	while((state.remaining_chunks > 0) && (section_size>0) && (ret == 0)) {
		ret = _unsparse_process_chunk(&source, &section_size, WRITE);
	}

	printf("sparse: remaining chunks: %lu\n", state.remaining_chunks);
	return ret;
}

u8 do_unsparse_process(unsigned char *source, u64 section_size)
{
	if (_unsparse(source, section_size, mmc_write))
		return 1;
#ifdef DEBUG
	if (_unsparse(source, section_size, mmc_compare))
		return 1;
#endif
	return 0;
}

void do_unsparse_start(u32 sector, unsigned int slot_no) 	{
	state.remaining_chunks = 0;
	state.chunk_offset     = 0;
        state.sector           = sector;
	state.slot_no          = slot_no;
}
			
