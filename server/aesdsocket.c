#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/fs.h>
#include <pthread.h>
#include "queue.h"
#include <time.h>

#define PORT "9000" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define BUFSIZE 128

#define USE_AESD_CHAR_DEVICE 1
#ifdef USE_AESD_CHAR_DEVICE
	#define FILEPATH "/dev/aesdchar"
#else
	#define FILEPATH "/var/tmp/aesdsocketdata"
#endif

bool is_signal_caught = false;
static pthread_mutex_t the_mutex = PTHREAD_MUTEX_INITIALIZER;

struct slist_data_s {
	pthread_t thread;
	bool thread_complete;
	int new_sockfd;
	SLIST_ENTRY(slist_data_s) entries;
};


static void sig_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        is_signal_caught = true;
    }
}

//int receive(int new_fd ) {
void *receive_thread( void *arg ) {
	struct slist_data_s *threadp = (struct slist_data_s *)arg;
	int new_fd = threadp->new_sockfd;

	char buf[BUFSIZE];
	int  err, ret, file;
	//int rc = 0;
	
	// ready to communicate on socket descriptor new_fd!
	memset(buf, 0, BUFSIZE);
	
	
	
	if((file = open(FILEPATH, O_CREAT | O_APPEND | O_RDWR, 0644)) == -1) {
		err = errno;
		perror("open");
		syslog(LOG_ERR, "open error: %s\n", strerror(err));
		//rc = -1;
	}
	
	while((ret = recv(new_fd, buf, BUFSIZE-1, 0)) > 0) {
		//printf("Received %d\n", ret);
		buf[ret] = '\0';
		char *newline = strchr(buf, '\n');
		
		if (newline) {
			pthread_mutex_lock(&the_mutex);
			if(write(file, buf, newline - buf + 1) < 0) {
				err = errno;
				perror("write");
				syslog(LOG_ERR, "write error: %s\n", strerror(err));
				//rc = -1;
				//break;
			} else {
				//printf("1writing %s to %s\n", buf, FILEPATH);
				syslog(LOG_DEBUG, "writing %s to %s", buf, FILEPATH);
				//break;
			}
			pthread_mutex_unlock(&the_mutex);
			break;
		}
		else {
            		pthread_mutex_lock(&the_mutex);
            		if(write(file, buf, ret) < 0) {
				err = errno;
				perror("write");
				syslog(LOG_ERR, "write error: %s\n", strerror(err));
				//break;
				//rc = -1;
			} else {
				//printf("2writing %s to %s\n", buf, FILEPATH);
				syslog(LOG_DEBUG, "writing %s to %s", buf, FILEPATH);
			}
			pthread_mutex_unlock(&the_mutex);
		}

	}
	
	if (ret == -1) {
		err = errno;
		perror("recv");
		syslog(LOG_ERR, "recv error: %s\n", strerror(err));
		//rc = -1;
	}
	
	#ifndef USE_AESD_CHAR_DEVICE
		lseek(file, 0, SEEK_SET);
	#else
		close(file);
		if((file = open(FILEPATH, O_CREAT | O_APPEND | O_RDWR, 0644)) == -1) {
			err = errno;
			perror("open");
			syslog(LOG_ERR, "open error: %s\n", strerror(err));
			//rc = -1;
		}
	#endif

	while ((ret = read(file, buf, BUFSIZE)) > 0) {
        	//printf("Sending %d\n", ret);
        	//printf("Sending %s to %s\n", buf, FILEPATH);
        
        	if (send(new_fd, buf, ret, 0) < 0) {
			err = errno;
			perror("read");
			syslog(LOG_ERR, "read error: %s\n", strerror(err));
			//rc = -1;
			break;
		}

        	//printf("Sent %d\n", ret);
    	}
	
	close(file);
	threadp->thread_complete = true;
	close(new_fd);
	//return rc;
	return arg;
}

#ifndef USE_AESD_CHAR_DEVICE
void timer_thread() {
	//https://www.geeksforgeeks.org/strftime-function-in-c/
	time_t t ;
	struct tm *tmp ;
	char MY_TIME[50];
	time( &t );
	
	memset(MY_TIME, 0, sizeof(MY_TIME));
	memset(&tmp, 0, sizeof tmp);
	
	//localtime() uses the time pointed by t ,
	// to fill a tm structure with the 
	// values that represent the 
	// corresponding local time.
     
	tmp = localtime( &t );
     
	// using strftime to display time
	strftime(MY_TIME, sizeof(MY_TIME), "timestamp: %Y %B %d %T\n", tmp);
	
	int  err, file;
	
	if((file = open(FILEPATH, O_CREAT | O_APPEND | O_RDWR, 0644)) == -1) {
		err = errno;
		perror("open");
		syslog(LOG_ERR, "open error: %s\n", strerror(err));
	}
	
	pthread_mutex_lock(&the_mutex);
	if(write(file, MY_TIME, strlen(MY_TIME)) < 0) {
		err = errno;
		perror("write");
		syslog(LOG_ERR, "write error: %s\n", strerror(err));
	}
	pthread_mutex_unlock(&the_mutex);
	
	close(file);
	
}
#endif

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

	int err = -1;
	int ret = -1;
	int sockfd = -1;
	struct addrinfo hints, *res;
	bool is_daemon_true = false;
	pid_t pid = -1;
	int new_fd = -1;
	int rc = 0;
	
	openlog(NULL, LOG_PID, LOG_USER);
	
	if(argc == 2) {
		if (strcmp(argv[1], "-d") == 0) {
			is_daemon_true = true;
		}
	}
	
	/*if(open(FILEPATH, O_CREAT | O_APPEND | O_RDWR, 0644) > 0) {
		remove(FILEPATH);
	}*/
	
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET; // use IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	
	if ((ret = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		//fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret));
		syslog(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(ret));
		rc = -1;
	}
	// servinfo now points to a linked list of 1 or more struct addrinfos
	
	
	// make a socket, bind it, and listen on it:
	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		err = errno;
		perror("socket");
		syslog(LOG_ERR, "socket error: %s\n", strerror(err));
		rc = -1;
	}
	
	int yes=1;
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		err = errno;
		perror("setsockopt");
		syslog(LOG_ERR, "vs error: %s\n", strerror(err));
		rc = -1;
	}


	if ((ret = bind(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
		err = errno;
		perror("bind");
		syslog(LOG_ERR, "bind error: %s\n", strerror(err));
		rc = -1;
		is_daemon_true = false;
	}
	
	if(is_daemon_true) {
		// steps discussed in LSP pages 173 to 174
		
		// create a new process
		pid = fork();
		if (pid == -1)
			rc = -1;
		else if (pid != 0) {
			// ... do everything until you don't need servinfo anymore ....
			freeaddrinfo(res); // free the linked-list
			close(sockfd);
			closelog();
			//printf("Parent done\n");
			return rc;
		}
			
		// create a new session and process group
		rc = setsid();
			
		// set the working directory to the root directory
		rc = chdir("/") ;
			
		// skip this
		// close all open files
		//for (int i = 0; i < 1000; i++)
		// 	close(i);
			
		// redirect fd's 0,1,2 to /dev/null
		open("/dev/null", O_RDWR); 	// stdin
		dup(0); 			// stdout
		dup(0);			// stderror
		
		
		/*if( daemon(0, 0) == -1) {
			rc = -1;
		}
		
		perror("daemon");*/
				
	}
	
	#ifndef USE_AESD_CHAR_DEVICE
	//LSP pages 389 to 394
	timer_t timer;
	struct sigevent evp;
	struct itimerspec ts;
	
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread;
	evp.sigev_notify_attributes = NULL;
	
	ts.it_interval.tv_sec = 10;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 10;
	ts.it_value.tv_nsec = 0;
	
	if(timer_create(CLOCK_REALTIME, &evp, &timer) == -1) {
		perror("timer_create");
	}
	
	if(timer_settime(timer, 0, &ts, NULL) == -1) {
		perror("timer_settime");
		timer_delete(timer);
	}
	#endif
	
	
	if ((ret = listen(sockfd, BACKLOG)) == -1) {
		err = errno;
		perror("listen");
		syslog(LOG_ERR, "listen error: %s\n", strerror(err));
		rc = -1;
	}
	
	struct sigaction sig;
	sig.sa_handler = sig_handler;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);

	if (sigaction(SIGINT, &sig, NULL) == -1) {
		err = errno;
		perror("sigaction");
		syslog(LOG_ERR, "sigaction error: %s\n", strerror(err));
		rc = -1;
	}

	if (sigaction(SIGTERM, &sig, NULL) == -1) {
		err = errno;
		perror("sigaction");
		syslog(LOG_ERR, "sigaction error: %s\n", strerror(err));
		rc = -1;
	}

	SLIST_HEAD(slisthead, slist_data_s) head;
    	SLIST_INIT(&head);
	struct slist_data_s *threadp;
	
	while(!is_signal_caught) {
		struct sockaddr_storage their_addr;
		socklen_t addr_size;
		char ipstr[INET_ADDRSTRLEN ];
		memset(ipstr, 0, INET_ADDRSTRLEN);
		
		// now accept an incoming connection
		addr_size = sizeof their_addr;
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			err = errno;
			perror("accept:");
			syslog(LOG_ERR, "accept error: %s\n", strerror(err));
			rc = -1;
		} else {
				
			// convert the IP to a string
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)&their_addr;
			inet_ntop(res->ai_family, &(ipv4->sin_addr), ipstr, sizeof ipstr);
			//inet_ntop(AF_INET, get_in_addr((struct sockaddr *)p->ai_addr), ipstr, sizeof ipstr);
				
			printf("Accepted connection from %s\n", ipstr);
			syslog(LOG_DEBUG, "Accepted connection from %s\n", ipstr);
					
					
			//ret = receive(new_fd);
			
			threadp = malloc(sizeof(struct slist_data_s));
			threadp->thread_complete = false;
			threadp->new_sockfd = new_fd;
			SLIST_INSERT_HEAD(&head, threadp, entries);
			pthread_create(&threadp->thread, NULL, receive_thread, threadp);
			
			//close(new_fd);
			//printf("Closed connection from %s\n", ipstr);
			//syslog(LOG_DEBUG, "Closed connection from %s\n", ipstr);
			
			
			SLIST_FOREACH(threadp, &head, entries) {
				//printf("Thread ID: %d, Completed: %d\n", (int)threadp->thread, threadp->thread_complete);
				if(threadp->thread_complete == true) {
					pthread_join(threadp->thread, NULL);
					close(threadp->new_sockfd);
				}
			}
			
			free(threadp);
					
			if(ret == -1) {
				//printf("receive error\n");
				syslog(LOG_DEBUG, "receive error\n");
				rc = -1;
			}
		}
	}
	
	if(is_signal_caught) rc = 0;

	#ifndef USE_AESD_CHAR_DEVICE
		if(open(FILEPATH, O_CREAT | O_APPEND | O_RDWR, 0644) > 0) {
			remove(FILEPATH);
		}
		timer_delete(timer);		
	#endif

	pthread_mutex_destroy(&the_mutex);
	
	while (!SLIST_EMPTY(&head)) {
		threadp = SLIST_FIRST(&head);
        	SLIST_REMOVE_HEAD(&head, entries);
        	free(threadp);
        }
	
	printf("Caught signal, exiting %d\n", pid);
	syslog(LOG_DEBUG, "Caught signal, exiting\n");
	
	// ... do everything until you don't need servinfo anymore ....
	freeaddrinfo(res); // free the linked-list
	
	
	shutdown(sockfd, SHUT_RDWR);
	//close(new_fd);
	close(sockfd);
	closelog();
	
	if(!SLIST_EMPTY(&head)) printf("SLIST NOT EMPTY\n");
	
	printf("Done %d\n", rc);
	
	return rc;
}
