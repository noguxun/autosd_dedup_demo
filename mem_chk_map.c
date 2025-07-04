#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#include <stdint.h>
#include <errno.h>

#define MAX_REGIONS 256
//#define DEBUG

#if __SIZEOF_POINTER__ == 8
#define ADDRESS_SPACE_END 0xFFFFFFFFFFFFFFFF
#else
#define ADDRESS_SPACE_END 0xFFFFFFFF
#endif

#define LINE_FORMAT_REGULAR  "| 0x%016lx - 0x%016lx | %-4s | %-9s | %-10lu | %-30s | %-31s |\n"

typedef struct {
    int status[3];
    unsigned long start;
    unsigned long end;
    char perms[5];
    char name[256];
    unsigned long inode;
    char dev[12];
} MemRegion;

typedef struct {
	MemRegion regions[MAX_REGIONS];
	int region_count;
} MemMap;


static void format_region_name(const char *src, char *dst, size_t maxlen) {
    size_t len = strlen(src);
    if (len <= maxlen) {
        strncpy(dst, src, maxlen);
        dst[maxlen] = '\0';
    } else {
        // Copy '...' and the last (maxlen-3) characters
        strcpy(dst, "...");
        strncpy(dst + 3, src + len - (maxlen - 3), maxlen - 3);
        dst[maxlen] = '\0';
    }
}

// Helper to get PFN from /proc/self/pagemap
unsigned long get_pfn(unsigned long vaddr) {
    FILE *f = fopen("/proc/self/pagemap", "rb");
    if (!f) return 0;
    unsigned long offset = (vaddr / sysconf(_SC_PAGESIZE)) * 8;
    if (fseek(f, offset, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    uint64_t entry = 0;
    if (fread(&entry, sizeof(entry), 1, f) != 1) {
        fclose(f);
        return 0;
    }
    fclose(f);
    if (!(entry & (1ULL << 63))) // page not present
        return 0;
    return entry & ((1ULL << 55) - 1); // bits 0-54 are PFN
}

void parse_proc_maps(MemMap *m) {
	FILE *fp = fopen("/proc/self/maps", "r");
	MemRegion *region = malloc(sizeof(MemRegion));

	if (!fp) {
		perror("Failed to open /proc/self/maps");
		exit(EXIT_FAILURE);
	}

	unsigned long prev_end = 0;
	char line[512];
	while (fgets(line, sizeof(line), fp)) {
		char perms[5], dev[12], mapname[256] = "";
		unsigned long offset, inode;

		if (sscanf(line, "%lx-%lx %4s %lx %11s %lu %255[^\n]s",
                   &(region->start), &(region->end), perms, &offset, dev, &inode, mapname) >= 6) {
            strncpy(region->perms, perms, 4);
            region->perms[4] = '\0';
            strncpy(region->name, mapname, 255);
            region->name[255] = '\0';
            region->inode = inode;
            strncpy(region->dev, dev, 11);
            region->dev[11] = '\0';

			if (m->region_count == 0 && region->start > 0) {
				m->regions[m->region_count].start = 0;
				m->regions[m->region_count].end = region->start;
				strcpy(m->regions[m->region_count].perms, "----");
				strcpy(m->regions[m->region_count].name, "[unmapped]");
				m->region_count++;
			}

			if (prev_end != 0 && region->start > prev_end) {
				m->regions[m->region_count].start = prev_end;
				m->regions[m->region_count].end = region->start;
				strcpy(m->regions[m->region_count].perms, "----");
				strcpy(m->regions[m->region_count].name, "[unmapped]");
				m->region_count++;
			}

			memcpy(&(m->regions[m->region_count++]), region, sizeof(MemRegion));
			prev_end = region->end;
		}
	memset(m->regions[m->region_count-1].status, 0, sizeof(m->regions[m->region_count-1].status));
#ifdef DEBUG
	printf("%lx-%lx %s %s [%d,%d,%d]\n", 
			m->regions[m->region_count-1].start, 
			m->regions[m->region_count-1].end, 
			m->regions[m->region_count-1].perms, 
			m->regions[m->region_count-1].name, 
			m->regions[m->region_count-1].status[0], 
			m->regions[m->region_count-1].status[1], 
			m->regions[m->region_count-1].status[2]);
#endif

	}
	fclose(fp);
	free(region);

	if (prev_end < ADDRESS_SPACE_END) {
		m->regions[m->region_count].start = prev_end;
		m->regions[m->region_count].end = ADDRESS_SPACE_END;
		strcpy(m->regions[m->region_count].perms, "----");
		strcpy(m->regions[m->region_count].name, "[unmapped]");
		m->region_count++;
	}
}

void print_memory_map(MemMap *m) {
    printf("Memory Layout:\n");
    printf("+-----------------------------------------+------+-----------+------------+--------------------------------+---------------------------------+\n");
    printf("| Memory Region                           | Perms| Dev       | Inode      | content                        | PFNs (start/mid/end)            |\n");
    for (int i = 0; i < m->region_count; i++) {
        unsigned long addrs[3] = {
            m->regions[i].start,
            (m->regions[i].start + m->regions[i].end) / 2,
            m->regions[i].end - 2 * sizeof(int)
        };
        unsigned long pfns[3];
        for (int j = 0; j < 3; j++)
            pfns[j] = get_pfn(addrs[j]);

        char pfn_str[64];
        snprintf(pfn_str, sizeof(pfn_str), "0x%lx / 0x%lx / 0x%lx", pfns[0], pfns[1], pfns[2]);

        char content_str[31];
        if (m->regions[i].name[0])
            format_region_name(m->regions[i].name, content_str, 30);
        else
            strncpy(content_str, "[anonymous]", 31);

        printf("+-----------------------------------------+------+-----------+------------+--------------------------------+---------------------------------+\n");
        printf(
            LINE_FORMAT_REGULAR,
            m->regions[i].start, m->regions[i].end, m->regions[i].perms,
            m->regions[i].dev, m->regions[i].inode,
            content_str,
            pfn_str
        );
    }
    printf("+-----------------------------------------+------+-----------+------------+--------------------------------+---------------------------------+\n");
}

extern void my_wait(char *s);

int main(int argc, char *argv[]) {
	MemMap *m;

	m = malloc(sizeof(MemMap));
	memset(m, 0, sizeof(m));
 	parse_proc_maps(m);
	print_memory_map(m);

	my_wait(END_WORD);

	free(m);

	return 0;
}

