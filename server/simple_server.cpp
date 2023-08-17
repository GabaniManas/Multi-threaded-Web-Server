/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "http_server.hh"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <queue>

#define MAX_THREADS 20
#define QUEUE_SIZE 10000

int current_working_threads = 0;
int current_accepted_file_descriptors = 0; // file descriptors in queue 

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// int rc = pthread_mutex_init(&lock, NULL);
// assert(rc == 0); // always check success!
pthread_cond_t file_descriptor_queue_fill = PTHREAD_COND_INITIALIZER;
// pthread_cond_t thread_pool_exhausted = PTHREAD_COND_INITIALIZER;
pthread_cond_t file_descriptor_queue_empty = PTHREAD_COND_INITIALIZER;

queue<int> file_descriptor_queue;

pthread_t tid[MAX_THREADS];
bool signal_threads_for_exit = false;

void error(char *msg) {
  perror(msg);
  // exit(EXIT_FAILURE);
}

void *handle_HTTP_request(void *arg)
{
  while(true) {
    // cout << "thread_started: before wait" << endl;
    
    pthread_mutex_lock(&lock);
    // while(file_descriptor_queue.empty())
    while ( file_descriptor_queue.size()==0 ){
      pthread_cond_wait(&file_descriptor_queue_fill, &lock);
      if(signal_threads_for_exit)
      {
        pthread_mutex_unlock(&lock);
        pthread_exit(NULL);
      }
    }
    int newsockfd = file_descriptor_queue.front();
    file_descriptor_queue.pop();
    // printf("Queue Size: %lu\n", file_descriptor_queue.size());
    // current_accepted_file_descriptors--;
    // cout << "current_accepted_file_descriptors from worker : " << file_descriptor_queue.size() << endl;
    pthread_cond_signal(&file_descriptor_queue_empty);
    pthread_mutex_unlock(&lock);
    
    // cout << "thread_started: after wait" << endl;
    char buffer[1024];
    int n;
    
    bzero(buffer, 1024);
    n = read(newsockfd, buffer, 1023);
    if (n == 0)
      error("client terminated without sending request");
    else if(n < 0)
      error("ERROR reading to socket");
    else {
      // printf("Here is the HTTP request from sockfd-%d:\n%s", newsockfd, buffer);
      HTTP_Response *http_response = handle_request(buffer);
      string response = http_response->get_string();
      
      n = write(newsockfd, response.c_str(), response.length());
      if (n < 0)
        error("ERROR writing to socket");
      delete http_response;
    }
    close(newsockfd);
  }
}

void my_signal_handler(int signal_code)
{
  signal_threads_for_exit = true;
  pthread_cond_broadcast(&file_descriptor_queue_fill);

  for(int i=0; i<MAX_THREADS; i++)
  {
    // pthread_kill(tid[i]);
    pthread_cancel(tid[i]);
    pthread_join(tid[i], NULL);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  cout << " -------- Server started -------- " << endl;
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  // char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;

  // signal handler
	signal(SIGINT, my_signal_handler);

  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(EXIT_FAILURE);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
    exit(EXIT_FAILURE);
  }

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");    
    exit(EXIT_FAILURE);
  }

  /* listen for incoming connection requests */

  listen(sockfd, QUEUE_SIZE);
  clilen = sizeof(cli_addr);

  // thread pooling starts here

  // pthread_t tid[MAX_THREADS];
  /* start all threads */    
  for (int i = 0; i < MAX_THREADS; i++) {
    if (pthread_create(&tid[i], NULL, handle_HTTP_request, NULL) < 0) {
      error("ERROR creating thread");
      exit(EXIT_FAILURE);
    }
  }

  /* accept a new request, create a newsockfd */

  while(true)
  {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
      error("ERROR on accept");
      exit(EXIT_FAILURE);
    }
    
    pthread_mutex_lock(&lock);
    while( file_descriptor_queue.size() == QUEUE_SIZE )
      pthread_cond_wait(&file_descriptor_queue_empty, &lock);
    file_descriptor_queue.push(newsockfd);
    // current_accepted_file_descriptors++;
    // cout << "current_accepted_file_descriptors from master: " << file_descriptor_queue.size() << endl;
    pthread_cond_signal(&file_descriptor_queue_fill);
    pthread_mutex_unlock(&lock);
  }
  
  return 0;
}
