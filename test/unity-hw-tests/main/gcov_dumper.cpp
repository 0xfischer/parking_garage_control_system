
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// GCOV Struct definitions for GCC 9+ (used by ESP-IDF)
// We derived these from GDB inspection

typedef uint32_t gcov_unsigned_t;
typedef uint64_t gcov_type;

struct gcov_ctr_info {
    gcov_unsigned_t num;
    gcov_type *values;
};

struct gcov_fn_info {
    const struct gcov_info *key;
    gcov_unsigned_t ident;
    gcov_unsigned_t lineno_checksum;
    gcov_unsigned_t cfg_checksum;
    struct gcov_ctr_info ctrs[1]; // flexible array
};

typedef void (*gcov_merge_fn) (gcov_type *, gcov_unsigned_t);

struct gcov_info {
    gcov_unsigned_t version;
    struct gcov_info *next;
    gcov_unsigned_t stamp;
    gcov_unsigned_t checksum;
    const char *filename;
    gcov_merge_fn merge[9];
    gcov_unsigned_t n_functions;
    const struct gcov_fn_info * const *functions;
};

struct gcov_root {
    struct gcov_info *list;
    unsigned int dumped : 1;
    unsigned int run_counted : 1;
    struct gcov_root *next;
    struct gcov_root *prev;
};

struct gcov_master {
    gcov_unsigned_t version;
    struct gcov_root *root;
};

// Extern the master symbol
extern struct gcov_master __gcov_master;

// Constants for GCDA format
#define GCOV_DATA_MAGIC ((gcov_unsigned_t)0x67636461) // "gcda"
#define GCOV_TAG_FUNCTION ((gcov_unsigned_t)0x01000000)
#define GCOV_TAG_COUNTER_BASE ((gcov_unsigned_t)0x01a10000)

void dump_u32(gcov_unsigned_t v) {
    uint8_t *p = (uint8_t*)&v;
    printf("%02x%02x%02x%02x", p[0], p[1], p[2], p[3]);
}

void dump_u64(gcov_type v) {
    uint8_t *p = (uint8_t*)&v;
    for (int i = 0; i < 8; i++) {
        printf("%02x", p[i]);
    }
}


void gcov_dump_to_console() {
    printf("\n=== GCOV DUMP START ===\n");
    
    // Check if master is linked
    // If not used, might be 0? 
    // We assume it exists.
    
    struct gcov_root *root = __gcov_master.root;
    gcov_unsigned_t version = __gcov_master.version;
    
    printf("VERSION: %08lx\n", (unsigned long)version);
    
    while (root) {
        struct gcov_info *info = root->list;
        while (info) {
            printf("FILE: %s\n", info->filename);
            printf("DATA: ");
            
            // Generate GCDA content hex stream
            // Magic
            dump_u32(GCOV_DATA_MAGIC);
            // Version
            dump_u32(version);
            // Stamp
            dump_u32(info->stamp);
            // Checksum (New in GCC/IDF?)
            dump_u32(info->checksum);
            
            for (gcov_unsigned_t i = 0; i < info->n_functions; i++) {
                const struct gcov_fn_info *fn = info->functions[i];
                
                // Function Record
                dump_u32(GCOV_TAG_FUNCTION);
                dump_u32(3); // Length (ident, lineno_cs, cfg_cs)
                dump_u32(fn->ident);
                dump_u32(fn->lineno_checksum);
                dump_u32(fn->cfg_checksum);
                
                // Counters (ARCS only usually)
                // We check merge functions to see which are active?
                // Or just check ctrs[0].num?
                // Usually GCC instrumented code has merge[0] set for arcs.
                // We assume index 0 is ARCS.
                
                const struct gcov_ctr_info *ctr = &fn->ctrs[0];
                if (ctr->num > 0) {
                     dump_u32(GCOV_TAG_COUNTER_BASE + 0);
                     dump_u32(ctr->num * 2); // Length in words (64bit values)
                     
                     for (gcov_unsigned_t j = 0; j < ctr->num; j++) {
                         dump_u64(ctr->values[j]);
                     }
                }
            }
            printf("\n"); // End of line for this file
            
            info = info->next;
            fflush(stdout);
        }
        root = root->next;
    }
    printf("=== GCOV DUMP END ===\n");
    fflush(stdout);
}
