#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFERLENGTH 256

/* displays error messages from system calls */
void
error (char *msg)
{
  perror (msg);
  exit (0);
}

int
main (int argc, char *argv[])
{
  int sockfd, n;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int res;
  char buffer[BUFFERLENGTH];

  if (argc < 3)
    {
      fprintf (stderr, "usage %s hostname port\n", argv[0]);
      exit (1);
    }

  /* Obtain address(es) matching host/port */
  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;  /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;  /* Stream socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;  /* Any protocol */

  res = getaddrinfo (argv[1], argv[2], &hints, &result);
  if (res != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (res));
      exit (EXIT_FAILURE);
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sockfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sockfd == -1)
  continue;

      if (connect (sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
  break;      /* Success */

      close (sockfd);
    }

  if (rp == NULL)
    {       /* No address succeeded */
      fprintf (stderr, "Could not connect\n");
      exit (EXIT_FAILURE);
    }

  freeaddrinfo (result);  /* No longer needed */

  /* Construct and send the command */
  memset (buffer, 0, BUFFERLENGTH);
  for (int i = 3; i < argc; i++)
    {
      strcat (buffer, argv[i]);
      if (i < argc - 1)
  {
    strcat (buffer, " "); // Add a space between arguments
  }
    }

  n = write (sockfd, buffer, strlen (buffer));
  if (n < 0)
    error ("ERROR writing to socket");

  memset (buffer, 0, BUFFERLENGTH);

  /* Wait for reply */
  n = read (sockfd, buffer, BUFFERLENGTH - 1);
  if (n < 0)
    error ("ERROR reading from socket");
  printf ("%s\n", buffer);
  close (sockfd);
  return 0;
}
