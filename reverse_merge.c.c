#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define STACK_SIZE 100 

int readaline(FILE *fin);
void reverse_out(FILE* fout);
int IsEmpty();
void Push(char c);
char Pop();

int top = -1;
int realloc_flag = 1;
char* stack;

int main(int argc, char *argv[])
{
    FILE *file1, *file2, *fout;
    int eof1 = 0, eof2 = 0;
    struct timeval before, after;
    int duration;
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

    stack = (char*)malloc(sizeof(char) * STACK_SIZE);
   
    gettimeofday(&before, NULL);

    do {
        if (!eof1) {
            if (!readaline(file1)) {
		reverse_out(fout);
                line1++; lineout++;
            } else
                eof1 = 1;
        }
        if (!eof2) {
            if (!readaline(file2)) {
		reverse_out(fout);
                line2++; lineout++;
            } else
                eof2 = 1;
        }
    } while (!eof1 || !eof2);

    gettimeofday(&after, NULL);
    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);
    printf("File1 = %ld, File2= %ld, Total = %ld Lines\n", line1, line2, lineout);
    free(stack);
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

int readaline(FILE *fin)
{    
    int ch, count = 0;

    do {
        if ((ch = fgetc(fin)) == EOF) {
            if (!count)
                return 1;
            else {
                Push(0x0a);
                break;
            }
        }
        if (count >= 100 && realloc_flag == 1) {
	    stack = (char*)realloc(stack, sizeof(char) * (STACK_SIZE*2));
	    realloc_flag = 0;
        }
	
        Push(ch);  
        count++;
    } while (ch != 0x0a);
    return 0;
}

void reverse_out(FILE* fout)
{
    char line[STACK_SIZE * 2];
    int i;

    i = 0;
    while (top != -1) {
        line[i] = Pop();
	i++;
    }
    line[i] = '\0';
    fputs(line, fout);
}

void Push(char c)
{
    stack[++top] = c;
}

char Pop()
{
    char c;

    if (!IsEmpty()) {
        c = stack[top];
        top--;
    }

    return c;
}
 
int IsEmpty()
{
    return (top == -1);
}

