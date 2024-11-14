# BSTOOL (BootSector tool)
A small tool for PC/DOS that dumps master/volume bootsector into file.

Created in OpenWatcom v2.0 beta.

## Usage
Program is called with arguments
```
C:\> bstool.exe <bios_device> <part_num> <output_file>
```
 - *bios_device* is BIOS disk device number (0 = floppy A, 0x80 = HDD0)
 - *part_num* is partition index, use '-' to address MBR itself.
 - *output_file* path where to save output
 