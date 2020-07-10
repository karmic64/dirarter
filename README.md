# dirarter

Utility program for overwriting the filenames of a Commodore 1541 D64 disk image, designed specifically for directory art.

You will need to supply a "source file" containing the art itself, the disk image to use, and the filename which the new disk image will be written to.

The source file can be either a raw binary file whose contents are the direct character codes to use, or a plain text file where every character is represented as a comma/space/newline-separated string. To minimize manual correction of automatic exports from PETSCII art tools, any strings in a text file which cannot be interpreted as a hex or decimal number are ignored rather than causing an error. Every filename is 16 characters long, and if the amount of character codes in the source file is not a multiple of 16, that's an error.

The characters in a source file can be encoded as either screen codes or PETSCII codes, but mind the limitations of which ones you can use in dir-art:

* Screen codes `$A0`-`$BF` - no petscii equivalent
* Screen codes `$E0`-`$FF` - no petscii equivalent
* Screen code `$8D`, PETSCII code `$0D` - newline
* Screen code `$CD`, PETSCII code `$8D` - line feed
* Screen code `$60`, PETSCII code `$A0` - used in the disk to mark end of filename

All filenames on the disk will be modified in the same order that they appear in the source file. If there are less files on the disk than there are filenames supplied, new DEL files will be created instead. An error will occur if you supply too many filenames to fit on track 18.
