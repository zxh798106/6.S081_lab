#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier(long n)
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex); 
  //printf("nd_num = %d, acquire lock, bstate.nd = %d\n", (int)n, bstate.nthread);
  ++bstate.nthread;
  // 非最后一个到此的线程，阻塞在此。
  if (bstate.nthread != nthread) {
    //printf("nd_num = %d, bstate.nd = %d, wait\n", (int)n,  bstate.nthread);
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    pthread_mutex_unlock(&bstate.barrier_mutex); // 被唤醒后会重新获取锁，所以这里要解锁。
  }
  // 最后一个进入该round barrier的线程，自增bstate.round，并唤醒其他线程。
  else {
    ++bstate.round;
    bstate.nthread = 0;
    pthread_mutex_unlock(&bstate.barrier_mutex); 
    //printf("nd_num = %d, bstate.nd = %d, wake up\n", (int)n, bstate.nthread);
    if (pthread_cond_broadcast(&bstate.barrier_cond) != 0) 
      printf("broadcast error\n");
  }
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier(n);
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  //printf("nthread = %d\n", nthread);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
