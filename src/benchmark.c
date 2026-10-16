#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    clock_t start, end;
    double time;

    start = clock();
    char *ptr = (char *)malloc(1048576);

    char *ptr_array[1024];

    int i;
    for (i = 0; i < 1024; i++)
    {
        ptr_array[i] = (char *)malloc(16384);
        ptr_array[i] = ptr_array[i];
    }

    free(ptr);

    for (i = 0; i < 1024; i++)
    {
        if (i % 2 == 0)
        {
            free(ptr_array[i]);
        }
    }

    ptr = (char *)malloc(524288);
    free(ptr);
    end = clock();

    time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Time in seconds: %f\n", time);

    return 0;
}