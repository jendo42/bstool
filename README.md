# BSTOOL (BootSector tool)
A small tool for PC/DOS that dumps master/volume bootsector into file.

Created in OpenWatcom v2.0 beta.

## Usage
Program is called with arguments
```
C:\> bstool.exe <bios device> <part num> <output file>
```
 - *<bios device>* is BIOS disk device number (0 = floppy A, 0x80 = HDD0)
 - *<part num>* is partition index, use '-' to address MBR itself.
 - *<output file>* path where to save output
 