#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define T18_OFFS 0x16500

uint8_t *disk;

#define BAM_TRACK(t) disk + T18_OFFS + (t*4)
#define BAM_OFFS(t,s) BAM_TRACK(t) + (s/8) + 1

#define SECTOR(s) disk + T18_OFFS + (s*256)

void err(char* s)
{
    puts(s);
    exit(EXIT_FAILURE);
}

int is_sector_free(int t, int s)
{
    return (*BAM_OFFS(t,s)) & (1 << (s&7));
}

void take_sector(int t, int s)
{
    uint8_t *p = BAM_OFFS(t,s);
    *p = *p & (0xff ^ (1 << (s&7)));
    p = BAM_TRACK(t);
    *p = *p - 1;
}


int main(int argc, char* argv[])
{
    char* srcname = NULL;
    char* diskname = NULL;
    char* outname = NULL;
    
    int srcbin = 0;
    int srcpet = 0;
    
    int needhelp = 0;
    if (argc < 4) needhelp++;
    for (int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        if (strlen(arg) == 2 && arg[0] == '-')
        {
            switch (arg[1])
            {
                case 't':
                    srcbin = 0;
                    break;
                case 'b':
                    srcbin = 1;
                    break;
                case 's':
                    srcpet = 0;
                    break;
                case 'p':
                    srcpet = 1;
                    break;
                case '?':
                    needhelp++;
                    break;
                default:
                    printf("Unrecognized option -%c\n", arg[1]);
                    exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp(arg, "--help"))
            needhelp++;
        else
        {
            if (outname) err("Too many arguments");
            else if (diskname) outname = arg;
            else if (srcname) diskname = arg;
            else srcname = arg;
        }
    }
    
    if (needhelp)
        err("dirarter by <karmic.c64@gmail.com>\n" \
            "usage: dirarter [options] src disk outfile\n" \
            "\n" \
            "Options:\n" \
            "  -t  Source file is a text file\n" \
            "  -b  Source file is a binary file\n" \
            "  -s  Source file is a list of screencodes\n" \
            "  -p  Source file is a list of petscii codes");
    
    uint8_t *art = NULL;
    size_t artlen = 0;
    
    FILE *f = fopen(srcname, srcbin ? "rb" : "r");
    if (!f) err("Couldn't open source file");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    char *fbuf = malloc(fsize+1);
    fread(fbuf, 1, fsize, f);
    fclose(f);
    
    if (srcbin)
    {
        art = malloc(fsize);
        artlen = fsize;
        memcpy(art, fbuf, fsize);
    }
    else
    {
        fbuf[fsize] = 0;
        char *tok = strtok(fbuf, " ,\n");
        while (tok)
        {
            int hex = 0;
            if (tok[0] == '0' && tok[1] == 'x')
            {
                hex++;
                tok += 2;
            }
            else if (tok[0] == '$')
            {
                hex++;
                tok++;
            }
            int conv = 0;
            int place = 1;
            for (char *p = tok+strlen(tok)-1; p >= tok; p--)
            {
                char c = tolower(*p);
                if (c >= '0' && c <= '9')
                {
                    conv += (c - '0') * place;
                    goto good_char;
                }
                if (hex && (c >= 'a' && c <= 'f'))
                {
                    conv += (c - 'a' + 0x0a) * place;
                    goto good_char;
                }
                goto bad_tok;
good_char:      place *= hex ? 0x10 : 10;
            }
            if (conv >= 0 && conv < 256)
            {
                art = realloc(art, artlen+1);
                art[artlen++] = conv;
            }
bad_tok:
            tok = strtok(NULL, " ,\n");
        }
        
    }
    
    free(fbuf);
    
    
    
    if (artlen % 0x10)
        err("Art length must be a multiple of 16");
    
    for (int i = 0; i < artlen; i++)
    {
        uint8_t c = art[i];
        if (!srcpet)
        {
            if (c < 0x20) c += 0x40;
            else if (c < 0x40)  /* nothing */ ;
            else if (c == 0x5e) c = 0xff;
            else if (c < 0x60) c += 0x80;
            else if (c < 0x80) c += 0x40;
            else if (c < 0xa0) c += 0x80;
            else if (c < 0xc0) err("Art contains illegal screencode in $a0-$bf");
            else if (c < 0xe0) c += 0xc0;
            else err("Art contains illegal screencode in $e0-$ff");
            
            art[i] = c;
        }
        if (c == 0x0d)
            err("Art contains illegal petscii code $0D");
        if (c == 0x8d)
            err("Art contains illegal petscii code $8D");
        if (c == 0xa0)
            err("Art contains illegal petscii code $A0");
    }
    
    
    f = fopen(diskname, "rb");
    if (!f) err("Couldn't open disk file");
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    if (fsize != 174848 && fsize != 175531 && fsize != 176608 && fsize != 197376)
    {
        fclose(f);
        err("Disk file is not a valid D64");
    }
    rewind(f);
    disk = malloc(fsize);
    fread(disk, 1, fsize, f);
    fclose(f);
    
    uint8_t *file = SECTOR(1);
    int sector = 1;
    int makenew = 0;
    for (int artindex = 0; artindex < artlen; artindex += 16)
    {
        if (!makenew && !(file[2] & 0x80))
            makenew++;
        if (makenew)
        {
            memset(file, 0, 0x20);
            file[2] = 0x80;
        }
        memcpy(file+5, art+artindex, 0x10);
        
        if (((artindex & 0x70) == 0x70) && (artindex < artlen-0x10)) /* next sector? */
        {
            if (file[0]) /* t/s link exists */
            {
                if (file[0] != 18 || file[1] >= 19)
                {
                    printf("Invalid T/S link to %i/%i\n", file[0], file[1]);
                    exit(EXIT_FAILURE);
                }
                sector = file[1];
                
                file = SECTOR(sector);
            }
            else  /* allocate new */
            {
                int nextsector = -1;
                for (int s = (sector + 3) % 19; s != sector; s = (s+3) % 19)
                {
                    if (is_sector_free(18, s))
                    {
                        nextsector = s;
                        break;
                    }
                }
                if (nextsector < 0)
                    err("Not enough free sectors on track 18");
                
                file = SECTOR(sector);
                file[0] = 18;
                file[1] = nextsector;
                
                sector = nextsector;
                file = SECTOR(sector);
                memset(file, 0, 0x100);
                
                take_sector(18, sector);
            }
        }
        else
        {
            file += 0x20;
        }
    }
    
    f = fopen(outname, "wb");
    fwrite(disk, 1, fsize, f);
    fclose(f);
    
    
    return EXIT_SUCCESS;
}

