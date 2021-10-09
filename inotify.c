#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <asm/unistd.h>
#include <string.h>
#include <time.h>


/* Read all available inotify events from the file descriptor 'fd'.
  wd is the table of watch descriptors for the directories in argv.
  argc is the length of wd and argv.
  argv is the list of watched directories.
  Entry 0 of wd and argv is unused. */

static void
handle_events(int fd, int *wd, int argc, char* argv[])
{
	/* Some systems cannot read integer variables if they are not
	properly aligned. On other systems, incorrect alignment may
	decrease performance. Hence, the buffer used for reading from
	the inotify file descriptor should have the same alignment as
	struct inotify_event. */

	char buf[4096]
	__attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	int i;
	ssize_t len;
	char *ptr;
	struct timeval timev;
	

	/* Loop while events can be read from inotify file descriptor. */

	for (;;) {

	/* Read some events. */

	len = read(fd, buf, sizeof buf);
	if (len == -1 && errno != EAGAIN) {
	   perror("read");
	   exit(EXIT_FAILURE);
	}

	 /* If the nonblocking read() found no events to read, then
			  it returns -1 with errno set to EAGAIN. In that case,
			  we exit the loop. */

	if (len <= 0)
	   break;

	/* Loop over all events in the buffer */

	for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {

		event = (const struct inotify_event *) ptr;

		/* Print event type */

		syscall(__NR_gettimeofday, &timev, NULL);
		
		//time_t curtime=timev.tv_sec;
		char buffer[30];
		strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&timev.tv_sec));
		syscall(__NR_write, 1, buffer, 30);
		char buffer2[8] = "\x0\x0\x0\x0\x0\x0";
		sprintf(buffer2,"%d-\n",timev.tv_usec);
		syscall(__NR_write, 1, buffer2, 8);
		//syscall(__NR_write, 1, "\n", 1);
		//printf("%s", buffer2);

  		//printf("time = %ld:%ld\n", (timev.tv_sec, timev.tv_usec));

		if (event->mask & IN_OPEN)
		syscall(__NR_write, 1, "IN_OPEN:             ", 20);
		if (event->mask & IN_CLOSE_NOWRITE)
		syscall(__NR_write, 1, "IN_CLOSE_NOWRITE:    ", 20);
		if (event->mask & IN_CLOSE_WRITE)
		syscall(__NR_write, 1, "IN_CLOSE_WRITE:      ", 20);
		if (event->mask & IN_ACCESS)
		syscall(__NR_write, 1, "IN_ACCESS:           ", 20);
		if (event->mask & IN_ATTRIB)
		syscall(__NR_write, 1, "IN_ATTRIB:           ", 20);
		if (event->mask & IN_CREATE)
		syscall(__NR_write, 1, "IN_CREATE:           ", 20);
		if (event->mask & IN_DELETE)
		syscall(__NR_write, 1, "----!!!---IN_DELETE: ", 20);
		if (event->mask & IN_DELETE_SELF)
		syscall(__NR_write, 1, "IN_DELETE_SELF:      ", 20);
		if (event->mask & IN_MODIFY)
		syscall(__NR_write, 1, "IN_MODIFY:           ", 20);
		if (event->mask & IN_MOVED_FROM)
		syscall(__NR_write, 1, "IN_MOVED_FROM:       ", 20);
		if (event->mask & IN_MOVE_SELF)
		syscall(__NR_write, 1, "IN_MOVE_SELF:        ", 20);
		if (event->mask & IN_MOVED_TO)
		syscall(__NR_write, 1, "IN_MOVED_TO:         ", 20);
		if (event->mask & IN_IGNORED)
		syscall(__NR_write, 1, "IN_IGNORED :         ", 20);
		if (event->mask & IN_EXCL_UNLINK)
		syscall(__NR_write, 1, "IN_EXCL_UNLINK :     ", 20);
				

	   /* Print the name of the watched directory */

		for (i = 1; i < argc; ++i) {
			if (wd[i] == event->wd) {
			   //printf("%s/ ", argv[i]);
			   syscall(__NR_write, 1, argv[i], strlen(argv[i])); 
			   syscall(__NR_write, 1, "/", 1); 
			   syscall(__NR_write, 1, event->name, event->len); 
			   break;
			}

		//printf(", name = %s , ", event->name);
		

		}


/* Print type of filesystem object */

           if (event->mask & IN_ISDIR)
               	syscall(__NR_write, 1, " [directory]\n", 13); 
           else
		syscall(__NR_write, 1, " [file]\n", 8); 
       }
   }
}



 int main(int argc, char* argv[])
{
	char buf;
	int fd, i, poll_num;
	int *wd;
	nfds_t nfds;
	struct pollfd fds[2];

	if (argc < 2) {
	printf("Usage: %s PATH [PATH ...]\n", argv[0]);
	exit(EXIT_FAILURE);
	}



	/* set syscall */
	int p = syscall(__NR_getpriority, PRIO_PROCESS, syscall(__NR_getpid));
	if(p > 20)p = -(p-20);
	else p = 20 - p;
	printf("start priority = %d\n", p);

	int r = syscall(__NR_setpriority, PRIO_PROCESS, syscall(__NR_getpid), -20);
	if(!r){
		p = syscall(__NR_getpriority, PRIO_PROCESS, syscall(__NR_getpid));
		if(p > 20)p = -(p-20);
		else p = 20 - p;
		printf("defined priority = %d\n", p);
	}
	/* end              */

	printf("Press ENTER key to terminate.\n");

	/* Create the file descriptor for accessing the inotify API */

	fd = syscall(__NR_inotify_init1, IN_NONBLOCK);
	if (fd == -1) {
	perror("inotify_init1");
	exit(EXIT_FAILURE);
	}

	/* Allocate memory for watch descriptors */

	wd = calloc(argc, sizeof(int));
	if (wd == NULL) {
	perror("calloc");
	exit(EXIT_FAILURE);
	}


	/* Mark directories for events
	- file was opened
	- file was closed */

	for (i = 1; i < argc; i++) {
	wd[i] = syscall(__NR_inotify_add_watch, fd, argv[i],
		                 IN_OPEN | IN_CLOSE | IN_ACCESS | IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF |
				 IN_MODIFY | IN_MOVE_SELF | IN_MOVE | IN_IGNORED | IN_EXCL_UNLINK);
	if (wd[i] == -1) {
	   fprintf(stderr, "Cannot watch '%s'\n", argv[i]);
	   perror("inotify_add_watch");
	   exit(EXIT_FAILURE);
	}
	}

	/* Prepare for polling */

	nfds = 2;

	/* Console input */

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	/* Inotify input */

	fds[1].fd = fd;
	fds[1].events = POLLIN;


	/* Wait for events and/or terminal input */

	printf("Listening for events.\n");
	while (1) {
		poll_num = syscall(__NR_poll, fds, nfds, -1);
		if (poll_num == -1) {
		   if (errno == EINTR)
		       continue;
		   perror("poll");
		   exit(EXIT_FAILURE);
		}

		if (poll_num > 0) {

		   if (fds[0].revents & POLLIN) {

		       /* Console input is available. Empty stdin and quit */

		       while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
			   continue;
		       break;
		   }

		   if (fds[1].revents & POLLIN) {

		       /* Inotify events are available */

		       handle_events(fd, wd, argc, argv);
		   }
		}
	}

	printf("Listening for events stopped.\n");

	/* Close inotify file descriptor */

	close(fd);

	free(wd);
	exit(EXIT_SUCCESS);
}








