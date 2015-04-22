/* Parallel sample sort
 */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"



static int compare(const void *a, const void *b)
{
    int *da = (int *)a;
    int *db = (int *)b;

    if (*da > *db)
    return 1;
    else if (*da < *db)
    return -1;
    else
    return 0;
}

int main( int argc, char *argv[])
{
    int rank, size, i;
    int *vec;
    int MAX_FILE_NAME=40;
    MPI_Status status;

    if (argc != 2) {
        printf("USAGE: ./ssort.o <Numbers per node>\n");
        abort();
    }
    int N = atoi(argv[1]);
    
    timestamp_type time1, time2;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        get_timestamp(&time1);
    }
    /* Number of random numbers per processor (this should be increased
    * for actual tests or could be passed in through the command line */
    N = 1000;

    vec = calloc(N, sizeof(int));
    /* seed random number generator differently on every core */
    srand((unsigned int) (rank + 393919));

    /* fill vector with random integers */
    for (i = 0; i < N; ++i) {
        vec[i] = rand();
    }
    printf("rank: %d, first entry: %d\n", rank, vec[0]);

    /* sort locally */
    qsort(vec, N, sizeof(int), compare);

    /* randomly sample s entries from vector or select local splitters,
    * i.e., every N/P-th entry of the sorted vector */
    int* splitters = calloc(size, sizeof(int));

    for (i=0; i<size; i++) {
        splitters[i] = vec[((i+1)*(N/size))-1];
    }

    /* every processor communicates the selected entries
    * to the root processor; use for instance an MPI_Gather */
    int num_all_splitters = 0;
    int *all_splitters = NULL;

    if (rank == 0) {
        num_all_splitters = size*size;
        all_splitters = calloc(num_all_splitters, sizeof(int));
    }
    
    MPI_Gather(splitters, size, MPI_INT, all_splitters, size, MPI_INT, 0, MPI_COMM_WORLD);

    /* root processor does a sort, determinates splitters that
    * split the data into P buckets of approximately the same size */
    if (rank == 0) {
        qsort(all_splitters, num_all_splitters, sizeof(int), compare);
        for (i=0; i<size; i++) {
            splitters[i] = all_splitters[((i+1)*(num_all_splitters/size))-1];
        }
    }
    
    
    /* root process broadcasts splitters */
    MPI_Bcast(splitters, size, MPI_INT, 0, MPI_COMM_WORLD);
    
    /* every processor uses the obtained splitters to decide
    * which integers need to be sent to which other processor (local bins) */
    int* nsends = calloc(size, sizeof(int));
    int p_idx = 0;
    for (i=0; i<N; i++) {
        if (p_idx == 0) {
            if (vec[i]<=splitters[p_idx]) {
                nsends[p_idx] += 1;
            } else {
                p_idx++;
                nsends[p_idx] += 1;
            }
        }
        else if (p_idx == size-1) {
            nsends[p_idx] += 1;
        }
        else if (vec[i]>splitters[p_idx-1] && vec[i]<=splitters[p_idx]) {
                nsends[p_idx] += 1;
            } else {
                p_idx++;
                nsends[p_idx] += 1;
            }
        }

    /* send and receive: either you use MPI_AlltoallV, or
    * (and that might be easier), use an MPI_Alltoall to share
    * with every processor how many integers it should expect,
    * and then use MPI_Send and MPI_Recv to exchange the data */
    int* nrecvs = calloc(size, sizeof(int));
    MPI_Alltoall(nsends, 1, MPI_INT, nrecvs, 1, MPI_INT, MPI_COMM_WORLD);
    
    
    int total_recvs = 0;
    for (i=0; i<size; i++) {
        total_recvs += nrecvs[i];
    }
    
    int* sorted_vec = calloc(total_recvs, sizeof(int));
    
    int recv_offset=0, send_offset=0, sender=0, recvr=0;
    
    for (sender=0; sender<size; sender++) {
        for (recvr=0; recvr<size; recvr++) {
            if ((sender == recvr) && (rank == sender)) {
                for (i=0; i<nsends[rank]; send_offset++, recv_offset++, i++) {
                    sorted_vec[recv_offset] = vec[send_offset];
                }
            }
            else{
                if (rank == sender) {
                    MPI_Send(&vec[send_offset], nsends[recvr], MPI_INT, recvr, 0, MPI_COMM_WORLD);
                    send_offset += nsends[recvr];
                } else if (rank == recvr) {
                    MPI_Recv(&sorted_vec[recv_offset], nrecvs[sender], MPI_INT, sender, 0, MPI_COMM_WORLD, &status);
                    recv_offset += nrecvs[sender];
                }
            }
        }
    }

    /* do a local sort */
    qsort(sorted_vec, total_recvs, sizeof(int), compare);

    /* every processor writes its result to a file */
    char filename[MAX_FILE_NAME];
    sprintf(filename, "sorted-%d.txt", rank);
    FILE* fp = fopen(filename, "w");
    for (i=0; i<total_recvs; i++) {
        fprintf(fp, "%d \n", sorted_vec[i]);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    /* free all allocated memory */
    free(vec);
    free(sorted_vec);
    free(splitters);
    free(nsends);
    free(nrecvs);
    
    if (rank == 0) {
        free(all_splitters);
        get_timestamp(&time2);
        double elapsed = timestamp_diff_in_seconds(time1,time2);
        printf("Time elapsed is %f seconds.\n", elapsed);
    }
    
    
    
    MPI_Finalize();

    
    return 0;
}
