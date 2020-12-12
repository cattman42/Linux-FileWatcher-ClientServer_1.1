#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include "csapp.h"

int run;

void handleTERM(int x) {
  run = 0;
}

void status(char* request, char* fileName){
  int clientfd;
  char *host, *port, buf[MAXLINE];
  rio_t rio;
  host = "127.0.0.1";
  port = "8000";
  clientfd = Open_clientfd(host, port);
  Rio_writen(clientfd, request, strlen(request)); //Write request to server
  Rio_writen(clientfd, fileName, strlen(fileName)); //Write file name to server
  printf("Sent: %s for file %s", request, fileName);
  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, buf, MAXLINE); // Read processing from server
  printf("Received: %s", buf);
  Rio_readlineb(&rio, buf, MAXLINE); // Read last mod-time from server
  printf("Last Mod Time: %s", buf);
  Close(clientfd);
}
void upload(char* request, char* fileName, int size) {
  int clientfd, srcfd;
  char buf[MAXLINE], buffer[32];
  rio_t rio;
  char *host, *port, *srcp;
  host = "127.0.0.1";
  port = "8000";
  clientfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, clientfd);
  Rio_writen(clientfd, request, strlen(request)); //Write request to server
  Rio_writen(clientfd, fileName, strlen(fileName));//Write file name to server
  sprintf(buffer, "%d\n", size);
  Rio_writen(clientfd, buffer, strlen(buffer)); //Write file size
  srcfd = Open(fileName, O_RDONLY, 0); 
  srcp = Mmap(0, size, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(clientfd, srcp, size); // Send file
  Munmap(srcp, size); 
  printf("Sent: %s\n", request);
  Rio_readlineb(&rio, buf, MAXLINE); // Read processing from server
  printf("Received: %s\n", buf);
  Close(clientfd);
}
int main(int argc, char **argv)
{
  FILE* logFile;
  struct sigaction action, old_action;
  time_t timer;
  DIR* watchDIR;
  struct dirent* entp;
  struct stat s;
  char path[1024], *srcp, *filename;
  int srcfd;
  /* Install the handler for SIGTERM */
  action.sa_handler = handleTERM;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGTERM, &acti

  /* Open the log file and start watching for changes. **/
  run = 1;
  logFile = fopen("fwd.log","w");
  while(run == 1) {
    /* Note the time and go to sleep. */
    time(&timer);
    sleep(10);
    /* Scan the watched directory for changes. */
    watchDIR = opendir(argv[1]);
    while((entp = readdir(watchDIR))!=NULL) {
      strcpy(path,argv[1]);
      strcat(path,"/");
      strcat(path,entp->d_name);
      stat(path,&s);
      /* Only pay attention to files, and ignore directories. */
      if(S_ISREG(s.st_mode)) {
        /* Log any recently modified files. */
        char* request = "status\n";
        sprintf(filename, "%s\n",entp->d_name);
        status(request, filename);
        if(difftime(s.st_mtime,timer) > 0) {
          printf("%s", entp->d_name);
          request = "upload\n";
          fprintf(logFile,"%s: %ld\n",entp->d_name,s.st_mtime);
          /////////upload()
          sprintf(filename, "%s\n",entp->d_name);
          upload(request, filename, s.st_size);
        }
      }
    }
    closedir(watchDIR);
  }
  fclose(logFile);
  return 0;
}
