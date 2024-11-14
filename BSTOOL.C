#include <stdio.h>
#include <bios.h>
#include <dos.h>
#include <stdbool.h>

#define SECTOR_SIZE 512

#define ERR_OK      0
#define ERR_PARTINV 1 // invalid partition number
#define ERR_MBRREAD 2 // error reading MBR
#define ERR_MBRINV  3 // invalid MBR (no 0x55AA signature)
#define ERR_READ    4

// Structure to represent a partition entry in the MBR
struct PartitionEntry {
    unsigned char status;         // 0x80 = active, 0x00 = inactive
    unsigned char startHead;
    unsigned short startSectorCylinder;
    unsigned char type;           // Partition type (e.g., 0x0B for FAT32)
    unsigned char endHead;
    unsigned short endSectorCylinder;
    unsigned long startLBA;       // Start of the partition in LBA
    unsigned long totalSectors;   // Total number of sectors in the partition
};

// Read a sector from the disk using INT 13h
int read_sector(int drive, unsigned long sector, unsigned char *buffer) {
    union REGS regs;
    struct SREGS sregs;
    unsigned int cylinder, head, sector_num;

    // Calculate CHS values from LBA for INT 13h
    cylinder = sector / (64 * 63);
    head = (sector / 63) % 64;
    sector_num = (sector % 63) + 1;

    // Set up the registers for INT 13h call
    regs.h.ah = 0x02;             // Read sector
    regs.h.al = 0x01;             // Number of sectors to read
    regs.h.ch = cylinder & 0xFF;  // Cylinder low byte
    regs.h.cl = sector_num;       // Sector number
    regs.h.dh = head;             // Head number
    regs.h.dl = drive;            // Drive number (0x80 = first hard disk)
    sregs.es = FP_SEG(buffer);    // Segment:offset of the buffer
    regs.x.bx = FP_OFF(buffer);

    // Call INT 13h
    int86x(0x13, &regs, &regs, &sregs);

    // Check for error
    return regs.h.ah == 0;
}

// Function to read the partition table from the MBR and access a specific partition
int read_partition_sector(int drive, int partition_num, unsigned long partition_sector, unsigned char *buffer)
{
    struct PartitionEntry *partition_table;
    unsigned long partition_start;
    int i;

    // Validate the partition number (0 to 3 for primary partitions)
    if (partition_num < 0 || partition_num > 3) {
        return ERR_PARTINV;
    }

    // Step 1: Read the MBR (first sector of the drive)
    if (!read_sector(drive, 0, buffer)) {
        return ERR_MBRREAD;
    }

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        return ERR_MBRINV;
    }

    // Step 2: Locate the partition table in the MBR
    partition_table = (struct PartitionEntry *)(buffer + 0x1BE);  // Partition table starts at 0x1BE

    // Step 3: Get the start sector of the specified partition
    partition_start = partition_table[partition_num].startLBA;
    if (partition_start == 0) {
        return ERR_PARTINV;
    }

    // Step 4: Read a sector from within the partition
    if (!read_sector(drive, partition_start + partition_sector, buffer)) {
        return ERR_READ;
    }

    //printf("Partition %d, Sector %lu contents:\n", partition_num, partition_sector);
    //for (i = 0; i < SECTOR_SIZE; i++) {
    //    printf("%02X ", buffer[i]);
    //    if ((i + 1) % 16 == 0) printf("\n");  // Print 16 bytes per line
    //}
    //printf("\n");

    return ERR_OK;
}


bool is_volume_valid(char volume)
{
    unsigned int drive_num = (volume - 'A') + 1;
    struct diskfree_t df;
    return _dos_getdiskfree(drive_num, &df) == 0;
}

bool is_drive_valid(char drive)
{
    struct diskinfo_t di = {
        0x80, // drive
        0, // head
        0, // track
        1, // sector
        1, // count
        NULL
    };
    di.drive |= drive;
    return _bios_disk(_DISK_VERIFY, &di) == 0;
}

int main(int argc, char **argv)
{
    FILE *out = NULL;
    unsigned char buffer[SECTOR_SIZE];
    int drv, part;
    if (argc != 4) {
        char j, i;
        printf(
            "Boot sector dumper v1.0 by Jendo\n"
            "This tool dumps bootsector of disk/volume to file.\n"
            "Usage: bstool <disk> <part> <filename>\nAvailable devices:\n"
        );
        for (j = 0, i = 0; j < 4; j++) {
            if (is_drive_valid(j)) {
                unsigned char d = j | 0x80;
                //drives[i++] = d;
                printf(" -> physical disk %.2Xh\n", d);
            }
        }
        for (j = 'A', i = 0; j <= 'Z'; j++) {
            if (is_volume_valid(j)) {
                //volumes[i++] = j;
                printf(" -> volume '%c'\n", j);
            }
        }
        return 0;
    }

    if (sscanf(argv[1], "%i", &drv) != 1) {
            printf("Invalid parameter '%s'\n", argv[1]);
            return 1;
    }


    if (argv[2][0] == '-') {
        if (!read_sector(drv, 0, buffer)) {
            printf("Failed to read bootsector from drive %.2Xh\n", drv);
            return 1;
        }
    } else {
        if (sscanf(argv[2], "%i", &part) != 1) {
            printf("Invalid parameter '%s'\n", argv[2]);
            return 1;
        }
        if (read_partition_sector(drv, part, 0, buffer) != ERR_OK) {
            printf("Failed to read drive %.2X partition %d bootsector\n", drv, part);
            return 1;
        }
    }

    out = fopen(argv[3], "wb");
    if (out == NULL) {
        printf("Failed save bootsector to '%s'\n", argv[3]);
        return 1;
    }

    fwrite(buffer, 1, SECTOR_SIZE, out);
    fclose(out);
    printf("Done!\n");
    return 0;
}
