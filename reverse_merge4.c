#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define SIZE 1024 * 1024 * 100  

long line1, line2, total;
 
int readaline_and_reverse_out(FILE *fin1, FILE *fin2, FILE *fout);
char* strrev(char* str);

int main(int argc, char *argv[])
{
    FILE *file1, *file2, *fout;
    int duration;
    struct timeval before, after;
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
    
    line1 = line2 = total = 0;
    if ((readaline_and_reverse_out(file1, file2, fout)) == 1) {
	fprintf(stderr, "error: error occurred from readaline_and_reverse_out function.\n");
	return 1;
    }

    gettimeofday(&after, NULL);
    
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %ld, File2= %ld, Total = %ld Lines\n", line1, line2, total);
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

int readaline_and_reverse_out(FILE *fin1, FILE *fin2, FILE *fout)
{    
    char *buf1, *buf2;
    char *start_pt1, *start_pt2;
    char *end_pt1, *end_pt2;

    if ((buf1 = (char*)malloc(sizeof(char) * SIZE)) == NULL) {
	fprintf(stderr, "error: out of memory.\n");
	return 1;
    }

    if ((buf2 = (char*)malloc(sizeof(char) * SIZE)) == NULL) {
        fprintf(stderr, "error: out of memory.\n");
	free(buf1);
        return 1;
    }

    if ((fread(buf1, SIZE, 1, fin1)) == -1) {
	fprintf(stderr, "error: fread function occurred an error.\n");
	return 1;
    }

    if ((fread(buf2, SIZE, 1, fin2)) == -1) {
        fprintf(stderr, "error: fread function occurred an error.\n");
        return 1;
    }

    start_pt1 = strtok_r(buf1, "\n", &end_pt1);
    start_pt2 = strtok_r(buf2, "\n", &end_pt2);
    line1++; line2++; total += 2;
    do {
        if (start_pt1 != NULL) {
            if ((fwrite(strrev(start_pt1), sizeof(char), strlen(start_pt1), fout)) ==  -1) {
	        fprintf(stderr, "error: fwrite function occurred an error.\n");
	        return 1;
            }
	    fputc('\n', fout);
            start_pt1 = strtok_r(NULL, "\n", &end_pt1);
            line1++;
            total++;
        }

        if (start_pt2 != NULL) {
            if ((fwrite(strrev(start_pt2), sizeof(char), strlen(start_pt2), fout)) ==  -1) {
               fprintf(stderr, "error: fwrite function occurred an error.\n");
               return 1;
            }
            fputc('\n', fout);
            start_pt2 = strtok_r(NULL, "\n", &end_pt2);
            line2++;
	    total++;
        }

    } while (start_pt1 != NULL || start_pt2 != NULL);
   
    free(buf1);
    free(buf2);
    return 0;
} 

char *strrev(char *str) 
{ 
     char *p1, *p2; 
  
     if (! str || ! *str) 
         return str; 
     for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) 
     { 
         *p1 ^= *p2; 
         *p2 ^= *p1; 
         *p1 ^= *p2; 
     } 
     return str; 
} 
