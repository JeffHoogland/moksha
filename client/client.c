#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

struct _coords {
  int xid;
  int x;
  int y;
};

typedef struct _coords coords;

int main(void)
{
 struct sockaddr_un    cli_sun;
 int fd, opcode, retval, size;
 char buf[100];
 coords p;

  if((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
      perror("socket() error");
      exit(1);
  }

  bzero(&cli_sun, sizeof(cli_sun));
  cli_sun.sun_family = AF_UNIX;
  strncpy(cli_sun.sun_path, ".e/ecom", sizeof(cli_sun.sun_path));

  if (connect(fd, (struct sockaddr*)&cli_sun, sizeof(cli_sun)) < 0)
    {
      fprintf(stderr, "EIPC Connect Error.\n"); exit(1);      
    }

  /* move a window */
  opcode = 1;
  p.xid = 5;
  p.x = 100;
  p.y = 300;

  if(write(fd, &opcode, sizeof(opcode)) == -1)
    perror("Cannot write data");

  size = sizeof(p);

  if(write(fd, &size, sizeof(size)) == -1)
    perror("Cannot write data");

  if(write(fd, &p, sizeof(p)) == -1)
    perror("Cannot write data");

  read(fd, &size, sizeof(size));
  read(fd, &retval, size);

  printf("Window move successful? %d\n", retval);

  /* get e version */
  opcode = 0;

  if(write(fd, &opcode, sizeof(opcode)) == -1)
    perror("Cannot write data");

  read(fd, &size, sizeof(size));
  read(fd, &buf, size);

  printf("Enlightenment Version: %s\n", buf);

  printf("Closing socket.\n");
  close(fd);

  return(0);
}
