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

static inline
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

#ifdef NICEWAIT_DEBUG
    fprintf(stderr, "%d: NiceWait used with usleep=%d\n", r, nicewait_usleep);
#else
    if (r == 0) {
        fprintf(stderr, "%d: NiceWait used with usleep=%d\n", r, nicewait_usleep);
    }
#endif
}

static inline
void NiceWait_pause(void)
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
#ifdef NICEWAIT_DEBUG
    fprintf(stderr, "NiceWait_pause called\n");
#endif
}

static inline
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

/* wait functions */

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    int rc = MPI_SUCCESS;
    int flag = 0;
    do {
        rc = PMPI_Test(request, &flag, status);
        if (rc != MPI_SUCCESS) return rc;
        if (flag) break;
        NiceWait_pause();
    } while(1);
    return rc;
}

int MPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status *status)
{
    int rc = MPI_SUCCESS;
    int flag = 0;
    do {
        rc = PMPI_Testany(count, requests, index, &flag, status);
        if (rc != MPI_SUCCESS) return rc;
        if (flag) break;
        NiceWait_pause();
    } while(1);
    return rc;
}

int MPI_Waitall(int count, MPI_Request requests[], MPI_Status statuses[])
{
    int rc = MPI_SUCCESS;
    int flag = 0;
    do {
        rc = PMPI_Testall(count, requests, &flag, statuses);
        if (rc != MPI_SUCCESS) return rc;
        if (flag) break;
        NiceWait_pause();
    } while(1);
    return rc;
}

int MPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[])
{
    int rc = MPI_SUCCESS;
    do {
        rc = PMPI_Testsome(incount, requests, outcount, indices, statuses);
        if (rc != MPI_SUCCESS) return rc;
        if (*outcount) break;
        NiceWait_pause();
    } while(1);
    return rc;
}

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
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ibcast(buffer, count, datatype, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype,
                  void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[],
                  void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

/* point-to-point communication */

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Isend(buf, count, datatype, dest, tag, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Ibsend(buf, count, datatype, dest, tag, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Issend(buf, count, datatype, dest, tag, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    rc = PMPI_Irsend(buf, count, datatype, dest, tag, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    return rc;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    int flag = 0;
    rc = PMPI_Irecv(buf, count, datatype, source, tag, comm, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    if (rc != MPI_SUCCESS) return rc;
    rc = MPI_Request_get_status(req, &flag, status);
    return rc;
}

int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status)
{
    int rc = MPI_SUCCESS;
    MPI_Request req = MPI_REQUEST_NULL;
    int flag = 0;
    rc = PMPI_Imrecv(buf, count, datatype, message, &req);
    if (rc != MPI_SUCCESS) return rc;
    rc = NiceWait_wait(&req);
    if (rc != MPI_SUCCESS) return rc;
    rc = MPI_Request_get_status(req, &flag, status);
    return rc;
}
