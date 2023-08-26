#include <stdio.h> 
#include <malloc.h>
#include <omp.h>
#include <math.h>

#define correctSizeSections 9000
#define correctSizeTask 1000

int partition(int* arr, unsigned int start, unsigned int end) {
    int i = start, j = end, pivot = arr[(i + j) / 2], element = 0;

    do
    {
        while (arr[i] < pivot) i++;

        while (arr[j] > pivot) j--;

        if (i <= j)
        {
            element = arr[i];
            arr[i] = arr[j];
            arr[j] = element;
            i++;
            j--;
        }
    } while (i <= j);
    return i;
}

void quicksort(int* arr, unsigned int start, unsigned int end) {
    if (start >= end) return;

    while (start < end) {
        unsigned int pivotIndex = partition(arr, start, end);

        if (pivotIndex - 1 - start < end - pivotIndex) {
            quicksort(arr, start, pivotIndex - 1);
            start = pivotIndex;
        }

        else {
            quicksort(arr, pivotIndex, end);
            end = pivotIndex - 1;
        }
    }
}

void quicksortSections(int* arr, unsigned int start, unsigned int end, int numThreads) {
    if (start >= end) return;

    unsigned int pivotIndex = partition(arr, start, end);
    if (end + 1 - start < correctSizeSections || ((end + 1 - pivotIndex < correctSizeSections) && (pivotIndex - start < correctSizeSections))) {
        if (pivotIndex < end)
            quicksort(arr, pivotIndex, end);

        if (start < pivotIndex - 1)
            quicksort(arr, start, pivotIndex - 1);
    }
    else {

#pragma omp parallel num_threads(2) if(numThreads>1)
        {
#pragma omp sections nowait
            {
#pragma omp section
                {
                    if (pivotIndex < end)
                        quicksortSections(arr, pivotIndex, end, numThreads - 2);
                }
#pragma omp section
                {
                    if (start < pivotIndex - 1)
                        quicksortSections(arr, start, pivotIndex - 1, numThreads - 2);
                }
            }
        }
    }
}


void quicksortTasks(int* arr, unsigned int start, unsigned int end) {
    if (start >= end) return;

    unsigned int pivotIndex = partition(arr, start, end);
    if (end + 1 - start < correctSizeTask || ((end + 1 - pivotIndex < correctSizeTask) && (pivotIndex - start < correctSizeTask))) {
        if (pivotIndex < end)
            quicksort(arr, pivotIndex, end);

        if (start < pivotIndex - 1)
            quicksort(arr, start, pivotIndex - 1);
    }
    else {
#pragma omp task
        {
            if (pivotIndex < end)
                quicksortTasks(arr, pivotIndex, end);
        }
        if (start < pivotIndex - 1)
            quicksortTasks(arr, start, pivotIndex - 1);
    }
}

int main(int argc, char* argv[])
{
    /*подано корректное число аргументов*/
    if (argc != 5) {
        fprintf(stderr, "Incorrect number of arguments.\n");
        return 1;
    }
    int numThreads = atoi(argv[3]);
    /*подано допустимое количества тредов*/
    if (numThreads > omp_get_max_threads() || numThreads < -1) {
        fprintf(stderr, "Invalid number of threads\n");
        return 1;
    }

    int options = atoi(argv[4]);

    if (numThreads == -1) options = 0;
    if (numThreads == 0) numThreads = omp_get_max_threads();

    FILE* file, * outfile;
    /* проверка открытия файла*/
    if ((file = fopen(argv[1], "r"))) {

        if (!(outfile = fopen(argv[2], "w"))) {
            fclose(file);
            fprintf(stderr, "Failed to open out file.\n");
            return 1;
        }

        unsigned int size;
        /* проверка корректности размера будущего массива.*/
        if (fscanf(file, "%u", &size) && size > 0) {
            int* arr;

            /*проверка выделения памяти*/
            if (!(arr = (int*)malloc(size * sizeof(int)))) {
                fprintf(stderr, "Failed to allocate mamory.\n");
                return 1;
            }
            for (unsigned int i = 0; i < size; i++) {
                /*проверка корректности формата и количества чисел*/
                if (fscanf(file, "%i", &arr[i]) > 0);
                else {
                    free(arr);
                    fclose(file);
                    fprintf(stderr, "Invalid element value.\n");
                    return 1;
                }
            }

            double start = 0, end = 0;
            int actualThreads = 0;

            switch (options)
            {
            case 0:
                start = omp_get_wtime();
                quicksort(arr, 0, size - 1);
                end = omp_get_wtime();
                break;

            case 1:
                /*omp_set_num_threads(numThreads);*/
                omp_set_max_active_levels(log2(numThreads));

                start = omp_get_wtime();
                quicksortSections(arr, 0, size - 1, numThreads);
                end = omp_get_wtime();

                actualThreads = numThreads;
                break;

            case 2:
                start = omp_get_wtime();
#pragma omp parallel num_threads(numThreads) if(numThreads>1)
                {
#pragma omp single
                    {
                        quicksortTasks(arr, 0, size - 1);
                        actualThreads = omp_get_num_threads();
                    }
                }
                end = omp_get_wtime();
                break;

            default:
                fclose(file);
                fclose(outfile);
                free(arr);
                fprintf(stderr, "Invalid key.\n");
                return 1;
            }

            printf("Time (%i thread(s)): %g ms\n", actualThreads, (end - start) * 1000);

            for (unsigned int i = 0; i < size; i++)
                fprintf(outfile, "%i ", arr[i]);
            fprintf(outfile, "\n");

            fclose(file);
            fclose(outfile);
            free(arr);
            return 0;
        }
        else {
            fclose(file);
            fclose(outfile);
            fprintf(stderr, "Invalid array size.\n");
            return 1;
        }
    }
    else {
        fprintf(stderr, "Failed to open input file.\n");
        return 1;
    }
}