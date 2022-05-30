#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>

int stop = 0;

void sig_handler( int signo, siginfo_t *info, void *ptr ) {
  stop = 1;
}

void set_sig_handler( void ) {
  struct sigaction action;

  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = sig_handler;

  if( sigaction(SIGINT, &action, NULL) == -1 ){
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

void *accum_stat( void *msg_queue_name ) {
  sigset_t mask;
  sigemptyset( &mask );
  sigaddset( &mask, SIGINT );
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  mqd_t mq_fd;

  mq_fd = mq_open( msg_queue_name, O_RDONLY | O_NONBLOCK );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    pthread_exit( NULL );
  }

  u_int32_t msg_value;
  int       ret;

  while( stop == 0 ) {
    ret = mq_receive( mq_fd, (char*)&msg_value, 4, NULL );
    if( ret != -1 ) {
      printf("Message value: %0d\n", msg_value);
    } else if( errno == EAGAIN ) {
      sleep(1);
    } else {
      perror("mq_timedreceive");
      if( mq_close( mq_fd ) )
        perror("mq_close");
      pthread_exit( NULL );
    }
  }
  
  if( mq_close( mq_fd ) )
    perror("mq_close");
  pthread_exit( NULL );
}

int main ( int argc, char *argv[] ) {

  set_sig_handler();

  pthread_t accum_thread;

  mqd_t mq_fd;
  struct mq_attr attr;

  attr.mq_flags   = 0;
  attr.mq_maxmsg  = 10;
  attr.mq_msgsize = 4;
  attr.mq_curmsgs = 0;

  char msg_queue_name [12] = "/test-msg-q\0";

  int raw_socket = socket(SOCK SOCK_DGRAM, htons(3));

  mq_fd = mq_open( msg_queue_name, O_WRONLY | O_CREAT, 0200, &attr );

  if( mq_fd == (mqd_t)-1 ) {
    perror("mq_open");
    exit(EXIT_FAILURE);
  }
  else
  {
    printf("Main thread opened msg queue\n");
  }

  pthread_create( &accum_thread, NULL, accum_stat, msg_queue_name );

  int msg_value = 0;
  int ret;

  while( stop == 0 ) {
    ret = mq_send( mq_fd, (char*)&msg_value, 4, 0 );
    if( ret == -1 ) {
      perror("mq_send");
      if( mq_close( mq_fd ) )
        perror("mq_close");
    }
    msg_value++;
    sleep(2);
  }

  if( mq_close( mq_fd ) )
    perror("mq_close");

  pthread_join( accum_thread, NULL );

  if( mq_unlink( msg_queue_name ) )
    perror("mq_unlink");

  exit(EXIT_SUCCESS);
}
