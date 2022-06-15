/*
  FUSE ssd: FUSE ioctl example
  Copyright (C) 2008       SUSE Linux Products GmbH
  Copyright (C) 2008       Tejun Heo <teheo@suse.de>
  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/
#define FUSE_USE_VERSION 35
#include <fuse.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "ssd_fuse_header.h"
#define SSD_NAME "ssd_file"
#define DEBUG_FILE "debug.txt"

FILE * debug;

enum
{
    SSD_NONE,
    SSD_ROOT,
    SSD_FILE,
};

static size_t physic_size;
static size_t logic_size;
static size_t host_write_size;
static size_t nand_write_size;

typedef union pca_rule PCA_RULE;
union pca_rule
{
    // this union means pca (32 bits) = [lba (16 bits), nand (16 bits)]
    unsigned int pca;
    struct
    {
        unsigned int lba : 16; // i-th page in that block
        unsigned int nand: 16; // i-th ssd block file
    } fields;
};

PCA_RULE curr_pca;
static unsigned int get_next_pca();

unsigned int* L2P,* P2L,* valid_count, free_block_number;

static int ssd_resize(size_t new_size)
{
    //set logic size to new_size
    if (new_size > NAND_SIZE_KB * 1024)
    {
        return -ENOMEM;
    }
    else
    {
        logic_size = new_size;
        return 0;
    }
}

static int ssd_expand(size_t new_size)
{
    //logic must less logic limit
    if (new_size > logic_size)
    {
        return ssd_resize(new_size);
    }
    return 0;
}

static int nand_read(char* buf, int pca)
{
    char nand_name[100];
    FILE* fptr;

    PCA_RULE my_pca;
    my_pca.pca = pca;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, my_pca.fields.nand);

    //read
    if ( (fptr = fopen(nand_name, "r") ))
    {
        fseek( fptr, my_pca.fields.lba * 512, SEEK_SET );
        fread(buf, 1, 512, fptr);
        fclose(fptr);
    }
    else
    {
        printf("open file fail at nand read pca = %d\n", pca);
        return -EINVAL;
    }
    return 512;
}

static int nand_write(const char* buf, int pca)
{
    char nand_name[100];
    FILE* fptr;

    PCA_RULE my_pca;
    my_pca.pca = pca;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, my_pca.fields.nand);

    //write
    if ( (fptr = fopen(nand_name, "r+")))
    {
        fseek( fptr, my_pca.fields.lba * 512, SEEK_SET );
        fwrite(buf, 1, 512, fptr);
        fclose(fptr);
        physic_size++;
        valid_count[my_pca.fields.nand]++;
    }
    else
    {
        printf("open file fail at nand (%s) write pca = %d, return %d\n", 
                nand_name, pca, -EINVAL);

        return -EINVAL;
    }

    nand_write_size += 512;
    return 512;
}

static int nand_erase(int block_index)
{
    if(valid_count[block_index] == FREE_BLOCK) {
        fprintf(debug, "[nand_erase] warning: freeing a already freed block!!!!\n");
    } else {
        free_block_number++; // should I do this here????????
    }
    
    char nand_name[100];
    FILE* fptr;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, block_index);
    fptr = fopen(nand_name, "w");
    if (fptr == NULL)
    {
        printf("erase nand_%d fail", block_index);
        return 0;
    }

    fclose(fptr);
    valid_count[block_index] = FREE_BLOCK;

    return 1;
}

int is_valid_lba(unsigned int lba){
    return ( lba != INVALID_LBA ) && ( lba != STALE_LBA );
}
void erase_free_block(){
    for(int i = 0; i< PHYSICAL_NAND_NUM; i++)
    {
        if(i == curr_pca.fields.nand)
        {
            fprintf(debug, "[GC] skip scanning current pca block, " 
                           "when finding dirty block, curr block = %d\n", i);   
            continue;
        }
        int stale_count = 0;
        for(int j = 0; j<PAGE_PER_BLOCK; j++)
        {
            if(P2L[i*10+j] == STALE_LBA)
            {
                stale_count++;
            }
        }
        if(stale_count == PAGE_PER_BLOCK && valid_count[i] != FREE_BLOCK){
            fprintf(debug, "[GC] free block detected! erasing and releasing...");
            nand_erase(i);
        }
    }
}

void garbage_collection2(){

    fprintf(debug, "\n\n\n\n\n%s\n", DIVIDER2);
    fprintf(debug, "\n\t\t\t\t\t[GC] garbage collection 2 start\n");
    fprintf(debug, "\n%s\n\n", DIVIDER2);
    fprintf(debug, "[GC] current pca [%d, %d]\n", 
            curr_pca.fields.nand, curr_pca.fields.lba);

    fprintf(debug, "[GC] L2P (PCA) before garbage collection: ");
    print_pca();
    fprintf(debug, "\n[GC] garbage collection start...\n");

    // finding block with the most stale pages.
    int most_stale_block, most_stale_count=0;
    for(int i = 0; i< PHYSICAL_NAND_NUM; i++)
    {
        if(i == curr_pca.fields.nand)
        {
            fprintf(debug, "[GC] skip scanning current pca block, " 
                           "when finding dirty block, curr block = %d\n", i);   
            continue;
        }

        //if(valid_count[i] == FREE_BLOCK)
        //{
        //    fprintf(debug, "[GC] skip scanning free block, " 
        //                   "when finding dirty block, curr block = %d\n", i);   
        //    continue;
        //}

        int stale_count = 0;
        for(int j = 0; j<PAGE_PER_BLOCK; j++)
        {
            if(P2L[i*10+j] == STALE_LBA)
            {
                stale_count++;
            }
        }

        fprintf(debug, "[GC] page %d, stale count: %d\n", i, stale_count);
        if(stale_count > most_stale_count){
            most_stale_count = stale_count;
            most_stale_block = i;
        }
    }

    // only run gc when there's just enough room 
    //  for moving least valid pages in the dirty block
    int least_valid_count = PAGE_PER_BLOCK - most_stale_count;
    if(curr_pca.fields.lba + 1 + least_valid_count < PAGE_PER_BLOCK) {
       fprintf(debug, "[GC] no need for gc yet...\n"); 
       return;
    } else if (curr_pca.fields.lba+1 + least_valid_count > PAGE_PER_BLOCK) {
        fprintf(debug, "[GC] error, no enough room for gc!!!!!\n");
    }

    // error detection
    if(valid_count[most_stale_block] ==FREE_BLOCK){
        fprintf(debug, "[GC] error, cleaning a free block!!!!!\n");
    }

    if(most_stale_count > 0)// check if there's any valid data in that block
    {         
        fprintf(debug, "\n[GC] cleaning a dirty block at %d, with %d stale blocks...\n", 
                most_stale_block, most_stale_count);

        // move valid data in most stale block
        for(int i = 0; i< PAGE_PER_BLOCK; i++)
        {
            int dirty_page_idx = most_stale_block*10 + i;
            if( is_valid_lba( P2L[dirty_page_idx] ) ){

                // move valid page to a free page
                PCA_RULE source_pca;
                source_pca.fields.nand = most_stale_block;
                source_pca.fields.lba =  i;

                PCA_RULE target_pca;
                target_pca.pca = get_next_pca();
                int free_block_idx = target_pca.fields.nand*10 + target_pca.fields.lba;

                fprintf(debug, "[GC] swaping page (dirty <-> free), (%d <-> %d)\n", 
                        dirty_page_idx, free_block_idx);
                fprintf(debug,"[GC] before swap:");
                print_pca();

                int target_lba = P2L[dirty_page_idx];  // get original valid page logical address (lba)

                P2L[dirty_page_idx] = STALE_LBA;       // mark old P2L STALE

                P2L[free_block_idx] = target_lba;      // store original lba to new P2L
                L2P[target_lba] = target_pca.pca;      // update L2P

                // moving the valid page to a free block
                char* tmp_buf;
                tmp_buf = calloc(512, sizeof(char));
                nand_read(tmp_buf, source_pca.pca);
                nand_write(tmp_buf, target_pca.pca);
                free(tmp_buf);

                fprintf(debug,"[GC] after swap:");
                print_pca();
            }
        }

        fprintf(debug,"[GC] after cleaning block %d:\n", most_stale_block);
        print_pca();
        nand_erase(most_stale_block);
    }
    else
    {
        fprintf(debug,"[GC] error: most stale block has no STALE page!!!!!!\n");
    }

    fprintf(debug, "[GC] total physical free block after gc: %d\n", free_block_number);
    fprintf(debug, "\n%s\n", DIVIDER2);
    fprintf(debug, "\n\t\t\t\t\t[GC] garbage collection done !!!!\n");
    fprintf(debug, "\n%s\n\n", DIVIDER2);
}

void garbage_collection(){

    fprintf(debug, "\n\n\n\n\n%s\n", DIVIDER2);
    fprintf(debug, "\n\t\t\t\t\t[GC] garbage collection start\n");
    fprintf(debug, "\n%s\n\n", DIVIDER2);
    fprintf(debug, "[GC] current pca [%d, %d]\n", curr_pca.fields.nand, curr_pca.fields.lba);

    int gc_end_block = curr_pca.fields.nand;
    int gc_start_block = (gc_end_block + 2)% PHYSICAL_NAND_NUM;

    int gc_start_page_idx = gc_start_block * PAGE_PER_BLOCK;
    int gc_end_page_idx = gc_end_block * PAGE_PER_BLOCK + (PAGE_PER_BLOCK-1);

    int safe_idx = 0; // prevent infinite loop

    fprintf(debug, "[GC] gc section [%d -> %d]\n", gc_start_block, gc_end_block);
    fprintf(debug, "[GC] L2P (PCA) before garbage collection: ");
    print_pca();
    fprintf(debug, "\n[GC] garbage collection start...\n");

    while(1)
    {
        // find a valid page that needs moving.
        if( is_valid_lba(P2L[gc_start_page_idx]) == 1 )
        {
            // find a empty page (not valid page) as target destination to move to.
            fprintf(debug,"\n\n\n\n[GC] step <%d>\n", safe_idx);
            fprintf(debug,"[GC] finding a free page for page %d...\n", gc_start_page_idx);

            while( gc_end_page_idx != gc_start_page_idx )
            {
                if( is_valid_lba(P2L[gc_end_page_idx]) == 0 )
                {
                    // move page to this target page
                    fprintf(debug,"[GC] found free page at %d, swap page (%d <-> %d)\n", 
                            gc_end_page_idx, gc_start_page_idx, gc_end_page_idx);

                    fprintf(debug,"[GC] before swap:", gc_start_page_idx, gc_end_page_idx);
                    print_pca();

                    PCA_RULE source_pca;
                    source_pca.fields.nand = gc_start_page_idx / PAGE_PER_BLOCK;
                    source_pca.fields.lba =  gc_start_page_idx % PAGE_PER_BLOCK;

                    PCA_RULE target_pca;
                    target_pca.fields.nand = gc_end_page_idx / PAGE_PER_BLOCK;
                    target_pca.fields.lba =  gc_end_page_idx % PAGE_PER_BLOCK;

                    // implementation note:  
                    //   my P2L stores lba, which means i-th page, a index
                    //   my L2P stores pca, which means [lba, nand], a PCA_RULE 

                    int target_lba = P2L[gc_start_page_idx];
                    P2L[gc_end_page_idx] = target_lba;
                    P2L[gc_start_page_idx] = STALE_LBA;
                    L2P[target_lba] = target_pca.pca;

                    char* tmp_buf;
                    tmp_buf = calloc(512, sizeof(char));
                    nand_read(tmp_buf, source_pca.pca);
                    nand_write(tmp_buf, target_pca.pca);
                    free(tmp_buf);
                    
                    fprintf(debug,"[GC] after swap:");
                    print_pca();
                    break;
                }

                // index decrement
                gc_end_page_idx = (gc_end_page_idx + TOTAL_LOGICAL_PAGE -1) % TOTAL_LOGICAL_PAGE;

                if(gc_end_page_idx == gc_start_page_idx)
                {
                    break;
                }

                // fail safe
                if(safe_idx++ > 400)
                {
                    fprintf(debug,"[GC] gc error exits!!!!!!!!\n");
                    break;
                }
            }
        }
        if( gc_end_page_idx == gc_start_page_idx )
        {
            fprintf(debug,"[GC] end page index: %d\n", gc_end_page_idx);
            fprintf(debug,"[GC] garbage collection done!!!\n");
            break;
        }
        if( safe_idx++ > 400 )
        {
            printf("[GC] gc error exits!!!!!!!!\n");
            break;
        }
        // index increment
        gc_start_page_idx = ( gc_start_page_idx +1) % TOTAL_LOGICAL_PAGE;
    }
    
    // free blocks with no valid data in it.
    for(int i = gc_start_block; ; i=(i+1)%PHYSICAL_NAND_NUM )
    {
        fprintf(debug, "[GC] checking block %d...\n", i);
        int is_free_block = 1;

        for(int j = 0; j< PAGE_PER_BLOCK; j++)
        {
            // check if a physical page containes any valid lba
            if( is_valid_lba(P2L[i*10+j]) )
            {
                is_free_block = 0;
                break;
            }
        }
        if(is_free_block)
        {
            fprintf(debug, "[GC] freeing block %d...\n", i);
            valid_count[i] = FREE_BLOCK;
            free_block_number++;
        }
        if(i == gc_end_block)
        {
            break;
        }
    }

    fprintf(debug, "[GC] total physical free block %d\n", free_block_number);
    fprintf(debug, "\n%s\n", DIVIDER2);
    fprintf(debug, "\n\t\t\t\t\t[GC] garbage collection done !!!!\n");
    fprintf(debug, "\n%s\n\n", DIVIDER2);
}

static unsigned int get_next_block()
{
    // original code
    for (int i = 0; i < PHYSICAL_NAND_NUM; i++)
    {
        if (valid_count[(curr_pca.fields.nand + i) % PHYSICAL_NAND_NUM] == FREE_BLOCK)
        {
            curr_pca.fields.nand = (curr_pca.fields.nand + i) % PHYSICAL_NAND_NUM;
            curr_pca.fields.lba = 0;
            free_block_number--;
            valid_count[curr_pca.fields.nand] = 0;
            return curr_pca.pca;
        }
    }
    fprintf(debug, "[get_next_block] error: out of block!!!!!\n\n\n");
    return OUT_OF_BLOCK;
}

static unsigned int get_next_pca()
{
    if (curr_pca.pca == INVALID_PCA)
    {
        //init, only the first block will get to here.
        curr_pca.pca = 0;
        valid_count[0] = 0; 
        free_block_number--;
        return curr_pca.pca;
    }

    if(curr_pca.fields.lba == 9)
    {
        int temp = get_next_block();
        if (temp == OUT_OF_BLOCK)
        {
            fprintf(debug, "\n\n[get_next_pca] error: out of block!!!!!\n");
            fprintf(debug, "[get_next_pca] error: free block count %d\n\n\n", free_block_number);
            print_pca();
            return OUT_OF_BLOCK;
        }
        else if(temp == -EINVAL)
        {
            fprintf(debug, "[get_next_pca] error: -EINVAL!!!!!\n\n\n");
            return -EINVAL;
        }
        else
        {
            return temp;
        }
    }
    else
    {
        curr_pca.fields.lba += 1;
    }
    return curr_pca.pca;

}

void print_lba()
{
    fprintf(debug,"\n\n%s LBA (L2P) %s\n\n", HALF_DIVIDER, HALF_DIVIDER);
    for(int i = 0; i<LOGICAL_NAND_NUM ; i++)
    {
        fprintf(debug,"[block %d]:\t", i);
        for(int j = 0; j<PAGE_PER_BLOCK ; j++)
        {
            PCA_RULE temp;
            temp.pca = L2P[i*10+j];
            unsigned int index = temp.fields.nand*10 + temp.fields.lba;

            if(temp.pca == INVALID_PCA)
            {
                fprintf(debug,"[IVLD],\t");
            }
            else
            {
                fprintf(debug,"[%d],\t", index);
            }
        }
        fprintf(debug,"\n");
    }
    fprintf(debug,"\n%s\n\n", DIVIDER);
}

void print_pca()
{
    fprintf(debug,"\n\n%s PCA (P2L) %s\n\n", HALF_DIVIDER, HALF_DIVIDER);
    for(int i = 0; i<PHYSICAL_NAND_NUM ; i++)
    {
        fprintf(debug,"[block %d]:\t", i);

        for(int j = 0; j<PAGE_PER_BLOCK ; j++)
        {
            unsigned int index = P2L[i*10+j];
            if (P2L[i*10+j] == STALE_LBA)
            {
                fprintf(debug,"[STLE],\t");
            }
            else if(P2L[i*10+j] == INVALID_LBA)
            {
                fprintf(debug,"[IVLD],\t");
            }
            else
            {
                fprintf(debug,"[%d],\t", index);
            }
        }
        fprintf(debug,"\n");
    }
    fprintf(debug,"\n%s\n\n", DIVIDER);
}

static int ftl_read( char* buf, size_t lba)
{
    // TODO
    // lba here means i-th logical page
    unsigned int pca = L2P[lba];
    if(pca != INVALID_PCA)
    {
        nand_read(buf, pca);
    }
    else
    {
        fprintf(debug,"[ftl_read] invalid PCA, ignoring nand_read\n");
    }
}

static int ftl_write(const char* buf, size_t lba_range, size_t lba)
{
    // TODO
    int idx, curr_size, remain_size, rst;
    fprintf(debug,"\n\n[ftl_write] lba_range: %d\n", lba_range);

    for (idx = 0; idx < lba_range; idx++)
    {
        // check if it's an overwrite on existing lba
        fprintf(debug,"[ftl_write] L2P index: %d\n", lba+idx);
        
        if(L2P[lba+idx] != INVALID_PCA)
        {
            // mark overwritten P2L as stale for later GC
            fprintf(debug,"[ftl_write] overwite to existing logical address (lba)...\n");

            PCA_RULE stale_pca;
            stale_pca.pca = L2P[lba+idx];

            fprintf(debug,"[ftl_write] marking stale at stale_pca: [%d, %d]\n", 
                    stale_pca.fields.nand, stale_pca.fields.lba);

            P2L[stale_pca.fields.nand*10 + stale_pca.fields.lba] = STALE_LBA; 
        }
        
        // gc code attept
        if(free_block_number <= 0)
        {
            garbage_collection2();
            //garbage_collection();
        }

        PCA_RULE new_pca;
        new_pca.pca = get_next_pca();

        fprintf(debug,"[ftl_write] write data at new pca: [%d:%d]\n\n", 
                curr_pca.fields.nand, curr_pca.fields.lba);

        L2P[lba+idx] = new_pca.pca;
        P2L[new_pca.fields.nand*10 + new_pca.fields.lba] = lba+idx; // note: L2P: PCA_RULE, P2L: page index
        
        //print_lba();
        //print_pca();

        nand_write(buf+(idx*512), new_pca.pca);
    }
}

static int ssd_file_type(const char* path)
{
    if (strcmp(path, "/") == 0)
    {
        return SSD_ROOT;
    }
    if (strcmp(path, "/" SSD_NAME) == 0)
    {
        return SSD_FILE;
    }
    return SSD_NONE;
}

static int ssd_getattr(const char* path, struct stat* stbuf,
                       struct fuse_file_info* fi)
{
    (void) fi;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = stbuf->st_mtime = time(NULL);
    switch (ssd_file_type(path))
    {
        case SSD_ROOT:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;
        case SSD_FILE:
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = logic_size;
            break;
        case SSD_NONE:
            return -ENOENT;
    }
    return 0;
}

static int ssd_open(const char* path, struct fuse_file_info* fi)
{
    (void) fi;
    if (ssd_file_type(path) != SSD_NONE)
    {
        return 0;
    }
    return -ENOENT;
}

static int ssd_do_read(char* buf, size_t size, off_t offset)
{
    int tmp_lba, tmp_lba_range, rst ;
    char* tmp_buf;

    //off limit
    if ((offset ) >= logic_size)
    {
        return 0;
    }
    if ( size > logic_size - offset)
    {
        //is valid data section
        size = logic_size - offset;
    }

    // creating a buffer to hold the values from ssd

    // index of your logical block from offset
    tmp_lba = offset / 512; 
    // how many logical blocks you need
    tmp_lba_range = (offset + size - 1) / 512 - (tmp_lba) + 1; 
    // use calloc for 'clean' memory (init with zero), why not size_t???
    tmp_buf = calloc(tmp_lba_range * 512, sizeof(char)); 

    for (int i = 0; i < tmp_lba_range; i++) 
    {
        // TODO
        ftl_read(tmp_buf + (512*i), tmp_lba+i);
    }

    memcpy(buf, tmp_buf + offset % 512, size);

    free(tmp_buf);
    return size;
}

static int ssd_read(const char* path, char* buf, size_t size,
                    off_t offset, struct fuse_file_info* fi)
{
    (void) fi;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    return ssd_do_read(buf, size, offset);
}

static int ssd_do_write(const char* buf, size_t size, off_t offset)
{    
    fprintf(debug, "[ssd_do_write] strlen: %d\n", strlen(buf));
    int tmp_lba, tmp_lba_range, tmp_offset, process_size;
    int idx, curr_size, remain_size, rst;
    char* tmp_buf;

    host_write_size += size;
    if (ssd_expand(offset + size) != 0)
    {
        return -ENOMEM;
    }

    tmp_lba = offset / 512; // index of your logical block from offset
    tmp_offset = offset % 512; // offset in the page
    tmp_lba_range = (offset + size - 1) / 512 - (tmp_lba) + 1; // how many logical blocks you need
    tmp_buf = calloc(tmp_lba_range * 512, sizeof(char));
    
    //fprintf(debug,"\n\n[offset ]: %d\n", offset);
    //fprintf(debug,"[tmp_lba_range]: %d\n", tmp_lba_range);
    //fprintf(debug,"[tmp_lba]: %d\n\n", tmp_lba);

    for (idx = 0; idx < tmp_lba_range; idx++)
    {
        // TODO
        ftl_read(tmp_buf+(idx*512), tmp_lba+idx);
    }
 
    memcpy(tmp_buf+tmp_offset, buf, size);

    ftl_write(tmp_buf, tmp_lba_range, tmp_lba);
    return size;
}

static int ssd_write(const char* path, const char* buf, size_t size,
                     off_t offset, struct fuse_file_info* fi)
{
    (void) fi;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    return ssd_do_write(buf, size, offset);
}

static int ssd_truncate(const char* path, off_t size,
                        struct fuse_file_info* fi)
{
    (void) fi;
    memset(L2P, INVALID_PCA, sizeof(int) * LOGICAL_NAND_NUM * PAGE_PER_BLOCK);
    memset(P2L, INVALID_LBA, sizeof(int) * PHYSICAL_NAND_NUM * PAGE_PER_BLOCK);
    memset(valid_count, FREE_BLOCK, sizeof(int) * PHYSICAL_NAND_NUM);
    curr_pca.pca = INVALID_PCA;
    free_block_number = PHYSICAL_NAND_NUM;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }

    return ssd_resize(size);
}
static int ssd_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info* fi,
                       enum fuse_readdir_flags flags)
{
    (void) fi;
    (void) offset;
    (void) flags;
    if (ssd_file_type(path) != SSD_ROOT)
    {
        return -ENOENT;
    }
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, SSD_NAME, NULL, 0, 0);
    return 0;
}
static int ssd_ioctl(const char* path, unsigned int cmd, void* arg,
                     struct fuse_file_info* fi, unsigned int flags, void* data)
{

    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    if (flags & FUSE_IOCTL_COMPAT)
    {
        return -ENOSYS;
    }
    switch (cmd)
    {
        case SSD_GET_LOGIC_SIZE:
            *(size_t*)data = logic_size;
            return 0;
        case SSD_GET_PHYSIC_SIZE:
            *(size_t*)data = physic_size;
            return 0;
        case SSD_GET_WA:
            *(double*)data = (double)nand_write_size / (double)host_write_size;
            return 0;
    }
    return -EINVAL;
}
static const struct fuse_operations ssd_oper =
{
    .getattr        = ssd_getattr,
    .readdir        = ssd_readdir,
    .truncate       = ssd_truncate,
    .open           = ssd_open,
    .read           = ssd_read,
    .write          = ssd_write,
    .ioctl          = ssd_ioctl,
};

int main(int argc, char* argv[])
{
    int idx;
    char nand_name[100];
    physic_size = 0;
    logic_size = 0;
    curr_pca.pca = INVALID_PCA;
    free_block_number = PHYSICAL_NAND_NUM;

    L2P = malloc(LOGICAL_NAND_NUM * PAGE_PER_BLOCK * sizeof(int));
    memset(L2P, INVALID_PCA, sizeof(int) * LOGICAL_NAND_NUM * PAGE_PER_BLOCK);
    P2L = malloc(PHYSICAL_NAND_NUM * PAGE_PER_BLOCK * sizeof(int));
    memset(P2L, INVALID_LBA, sizeof(int) * PHYSICAL_NAND_NUM * PAGE_PER_BLOCK);
    valid_count = malloc(PHYSICAL_NAND_NUM * sizeof(int));
    memset(valid_count, FREE_BLOCK, sizeof(int) * PHYSICAL_NAND_NUM);

    debug = fopen(DEBUG_FILE, "w+");

    print_lba();
    print_pca();

    //create nand file
    for (idx = 0; idx < PHYSICAL_NAND_NUM; idx++)
    {
        FILE* fptr;
        snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, idx);
        fptr = fopen(nand_name, "w");

        if (fptr == NULL)
        {
            printf("open fail\n");
        }else{
            printf("open success\n");
        }
        fclose(fptr);
    }
    return fuse_main(argc, argv, &ssd_oper, NULL);
}
