#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define MAX_SIZE 1024

int readaline_and_reverse_out(FILE *fin, FILE *fout);
char* strrev(char* str);

int main(int argc, char *argv[])
{
    FILE *file1, *file2, *fout;
    int eof1 = 0, eof2 = 0;
    int duration;
    struct timeval before, after;
    long line1 = 0, line2 = 0, lineout = 0;
    int ret = 1;

    if (argc != 4) {
        fprintf(stderr, "usage: %s file1 file2 fout\n", argv[0]);
        goto leave0;
    }
    if ((file1 = fopen(argv[1], "rt")) == NULL) {
        perror(argv[1]);
        goto leave0;
    }
    if ((file2 = fopen(argv[2], "rt")) == NULL) {
        perror(argv[2]);
        goto leave1;
    }
    if ((fout = fopen(argv[3], "wt")) == NULL) {
        perror(argv[3]);
        goto leave2;
    }
    
    gettimeofday(&before, NULL);
    do {
        if (!eof1) {
            if (!readaline_and_reverse_out(file1, fout)) {
                line1++; lineout++;
            } else
                eof1 = 1;
        }
        if (!eof2) {
            if (!readaline_and_reverse_out(file2, fout)) {
                line2++; lineout++;
            } else
                eof2 = 1;
        }
    } while (!eof1 || !eof2);
    gettimeofday(&after, NULL);
    
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %ld, File2= %ld, Total = %ld Lines\n", line1, line2, lineout);
    ret = 0;
    
leave3:
    fclose(fout);
leave2:
    fclose(file2);
leave1:
    fclose(file1);
leave0:
    return ret; 
}

/* Read a line from fin and write it to fout */
/* return 1 if fin meets end of file */

int readaline_and_reverse_out(FILE *fin, FILE *fout)
{    
    int ch,count = 0;
    char buf[MAX_SIZE];
    char* rev;

    do {
        if ((ch = fgetc(fin)) == EOF) {
            if (!count)
                return 1;
            else {
                buf[count] = 0x0a;
                break;
            }
        }
        buf[count] = ch;
        count++;
    } while (ch != 0x0a);
    buf[count] = '\0';

    rev = strrev(buf);
    fputs(rev, fout);
    return 0;
}

char* strrev(char *str)  
{  
    char *p1, *p2;
    char temp;
    int i;
  
    if (! str || ! *str) 
        return str;

    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {  
        *p1 ^= *p2;  
        *p2 ^= *p1;  
        *p1 ^= *p2;  
    }  

    return str;  
 }  

