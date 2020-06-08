#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#if defined(HAVE_NANOSLEEP)

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include <time.h>     /* nanosleep */
#include <sys/time.h> /* nanosleep */

#elif defined(HAVE_USLEEP)

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* usleep */
#endif

#else

#warning No naptime available!

#endif

static int nicewait_usleep;

void NiceWait_init(void)
{
    int n = 0;

    int r = 0;
    PMPI_Comm_rank(MPI_COMM_WORLD, &r);

    if (r == 0) {
        char * c = getenv("NICEWAIT_TIME");
        int    i = (c) ? atoi(c) : 1;
        if (i > 1000) {
            fprintf(stderr, "NiceWait: requested wait time (%d) exceeds one second.  This is imprudent.\n",i);
        }
        if (i < 0) {
            fprintf(stderr, "NiceWait: requested wait time (%d) is negative and will be ignored.\n",i);
        }
        n = i;
    }

    /* Why does n exist?  Because I am paranoid some weird machine won't broadcast static globals properly. I have my reasons. */
    PMPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    nicewait_usleep = n;
}

static inline void NiceWait_pause(void)
{
#if defined(HAVE_NANOSLEEP)
    int naptime = 1000 * nicewait_usleep;
    struct timespec napstruct = { .tv_sec  = 0,
                                  .tv_nsec = naptime };
    if (naptime) nanosleep(&napstruct,NULL);
#elif defined(HAVE_USLEEP)
    int naptime = nicewait_usleep;
    if (naptime) usleep(naptime);
#endif
}

int NiceWait_wait(MPI_Request * req)
{
    int rc = MPI_SUCCESS;
    int flag = 0;
    do {
        rc = PMPI_Test(req, &flag, MPI_STATUS_IGNORE);
        if (rc != MPI_SUCCESS) return rc;
        if (flag) break;
        NiceWait_pause();
    } while(1);

    return rc;
}

/********************************************
 * MPI wrapper stuff
 ********************************************/

/* initialization and termination */

int MPI_Init(int * argc, char** * argv)
{
    int rc = PMPI_Init(argc, argv);
    NiceWait_init();
    return rc;
}

int MPI_Init_thread(int * argc, char** * argv, int requested, int * provided)
{
    int rc = PMPI_Init_thread(argc, argv, requested, provided);
    NiceWait_init();
    return rc;
}

int MPI_Finalize(void)
{
    return PMPI_Finalize();
}

/* collective utility functions */

int MPI_Barrier(MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ibarrier(comm,&req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

/* collective communication */

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    return rc;
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    return rc;
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
    return rc;
}

int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
    return rc;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int rc = PMPI_Bcast(buffer, count, datatype, root, comm);
    return rc;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    return rc;
}

int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
    return rc;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    return rc;
}

int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    return rc;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    return rc;
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    return rc;
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    return rc;
}

int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype,
                  void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
    return rc;
}

int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[],
                  void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int rc = PMPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
    return rc;
}

/* point-to-point communication */

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = PMPI_Send(buf, count, datatype, dest, tag, comm);
    return rc;
}

int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = PMPI_Bsend(buf, count, datatype, dest, tag, comm);
    return rc;
}

int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = PMPI_Ssend(buf, count, datatype, dest, tag, comm);
    return rc;
}

int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = PMPI_Rsend(buf, count, datatype, dest, tag, comm);
    return rc;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int rc = PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    return rc;
}

int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status)
{
    int rc = PMPI_Mrecv(buf, count, datatype, message, status);
    return rc;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    int rc = PMPI_Wait(request, status);
    return rc;
}

int MPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status *status)
{
    int rc = PMPI_Waitany(count, requests, index, status);
    return rc;
}

int MPI_Waitall(int count, MPI_Request requests[], MPI_Status statuses[])
{
    int rc = PMPI_Waitall(count, requests, statuses);
    return rc;
}

int MPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[])
{
    int rc = PMPI_Waitsome(incount, requests, outcount, indices, statuses);
    return rc;
}

int MPI_Testsome(int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[])
{
    int rc = PMPI_Testsome(incount, requests, outcount, indices, statuses);
    return rc;
}
