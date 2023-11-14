#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BUFFERLENGTH 256

#define THREAD_IN_USE 0
#define THREAD_FINISHED 1
#define THREAD_AVAILABLE 2
#define THREADS_ALLOCATED 10

typedef struct IpPortPairNode
{
  char *ip;
  int port;
  struct IpPortPairNode *next;
} IpPortPairNode;

struct firewallRule_t
{
  int ipaddr1[4];
  int ipaddr2[4];
  int port1;
  int port2;
  IpPortPairNode *matched_queries;
};

struct firewallRules_t
{
  struct firewallRule_t *rule;
  struct firewallRules_t *next;
};

typedef struct threadArgs_t
{
  int newsockfd;
  int threadIndex;
} threadArgs_t;


struct threadInfo_t
{
  pthread_t pthreadInfo;
  pthread_attr_t attributes;
  int status;
};
struct threadInfo_t *serverThreads = NULL;
int noOfThreads = 0;
pthread_rwlock_t threadLock = PTHREAD_RWLOCK_INITIALIZER;
pthread_cond_t threadCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t threadEndLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t rules_mutex;
pthread_mutexattr_t rules_mutexattr;

void
initialize_mutexes ()
{
  pthread_mutexattr_init (&rules_mutexattr);
  pthread_mutexattr_settype (&rules_mutexattr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init (&rules_mutex, &rules_mutexattr);
  pthread_mutex_init (&threadEndLock, NULL);
  pthread_rwlock_init (&threadLock, NULL);
  pthread_cond_init (&threadCond, NULL);
}


struct firewallRules_t *rules_head;
pthread_mutex_t rules_mutex;

int
validate_ip (const char *ip)
{
  struct sockaddr_in sa;
  return inet_pton (AF_INET, ip, &(sa.sin_addr)) != 0;
}

int
validate_port (int port)
{
  return port >= 0 && port <= 65535;
}

void
printIPaddress (FILE * stream, const int *ipaddr)
{
  fprintf (stream, "%d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
}


int
compareIPAddresses (const int *ipaddr1, const int *ipaddr2)
{
  int i;
  for (i = 0; i < 4; i++)
    {
      if (ipaddr1[i] > ipaddr2[i])
  {
    return 1;
  }
      else if (ipaddr1[i] < ipaddr2[i])
  {
    return -1;
  }
    }
  return 0;
}

char *
parseIPaddress (int *ipaddr, char *text)
{
  char *oldPos, *newPos;
  long int addr;
  int i;


  oldPos = text;
  for (i = 0; i < 4; i++)
    {
      if (oldPos == NULL || *oldPos < '0' || *oldPos > '9')
  {
    ipaddr[0] = -1;
    return NULL;
  }
      addr = strtol (oldPos, &newPos, 10);
      if (newPos == oldPos || addr < 0 || addr > 255)
  {
    ipaddr[0] = -1;
    return NULL;
  }
      ipaddr[i] = addr;

      if (i < 3)
  {
    if (*newPos != '.')
      {
        ipaddr[0] = -1;
        return NULL;
      }
    oldPos = newPos + 1;
  }
      else
  {

    if (*newPos != ' ' && *newPos != '-' && *newPos != '\0'
        && *newPos != '\n')
      {
        ipaddr[0] = -1;
        return NULL;
      }
    oldPos = newPos;
  }
    }
  return oldPos;
}



char *
parsePort (int *port, char *text)
{
  char *newPos;


  if ((text == NULL) || (*text < '0') || (*text > '9'))
    {
      return NULL;
    }
  *port = strtol (text, &newPos, 10);
  if (newPos == text)
    {
      *port = -1;
      return NULL;
    }
  if ((*port < 0) || (*port > 65535))
    {
      *port = -1;
      return NULL;
    }


  return newPos;
}


void
printRule (FILE * stream, const struct firewallRule_t *rule)
{
  if (rule->ipaddr1[0] != -1)
    {
      printIPaddress (stream, rule->ipaddr1);
      if (rule->ipaddr2[0] != -1)
  {
    fprintf (stream, "-");
    printIPaddress (stream, rule->ipaddr2);
  }
    }


  if (rule->ipaddr1[0] != -1)
    {
      fprintf (stream, " ");
    }

  if (rule->port1 != -1)
    {
      fprintf (stream, "%d", rule->port1);
      if (rule->port2 != -1 && rule->port2 != rule->port1)
  {
    fprintf (stream, "-%d", rule->port2);
  }
    }
  fprintf (stream, "\n");
}


struct firewallRule_t *
readRule (char *line)
{
  struct firewallRule_t *newRule;
  char *pos;

  newRule = malloc (sizeof (struct firewallRule_t));
  if (!newRule)
    {
      return NULL;
    }

  newRule->matched_queries = NULL;

  pos = parseIPaddress (newRule->ipaddr1, line);
  if ((pos == NULL) || (newRule->ipaddr1[0] == -1))
    {
      free (newRule);
      return NULL;
    }

  if (*pos == '-')
    {
      pos = parseIPaddress (newRule->ipaddr2, pos + 1);
      if ((pos == NULL) || (newRule->ipaddr2[0] == -1))
  {
    free (newRule);
    return NULL;
  }
    }
  else
    {
      memcpy (newRule->ipaddr2, newRule->ipaddr1, sizeof (newRule->ipaddr1));
    }

  while (*pos == ' ' || *pos == '-')
    pos++;

  pos = parsePort (&(newRule->port1), pos);
  if ((pos == NULL) || (newRule->port1 == -1))
    {
      free (newRule);
      return NULL;
    }

  if (*pos == '-')
    {
      pos++;
      pos = parsePort (&(newRule->port2), pos);
      if ((pos == NULL) || (newRule->port2 == -1)
    || (newRule->port2 < newRule->port1))
  {
    free (newRule);
    return NULL;
  }
    }
  else
    {

      newRule->port2 = newRule->port1;
    }

  if ((*pos != '\n') && (*pos != '\0') && (*pos != ' '))
    {
      free (newRule);
      return NULL;
    }

  return newRule;
}


bool
validateRule (const struct firewallRule_t * rule)
{
  if (rule->ipaddr1[0] == -1)
    {
      return false;
    }
  if (rule->ipaddr2[0] != -1
      && compareIPAddresses (rule->ipaddr1, rule->ipaddr2) > 0)
    {
      return false;
    }

  if (rule->port1 < 0 || rule->port1 > 65535)
    {
      return false;
    }
  if (rule->port2 != -1 && (rule->port2 < rule->port1 || rule->port2 > 65535))
    {
      return false;
    }

  return true;
}


const char *
addRule (struct firewallRules_t **rules_head, struct firewallRule_t *rule)
{

  if (!rule)
    {
      return "Invalid rule";
    }
  if (!validateRule (rule))
    {
      return "Invalid rule";
    }


  struct firewallRules_t *newRule = malloc (sizeof (struct firewallRules_t));
  if (!newRule)
    {
      return "Memory allocation failed";
    }

  pthread_mutex_lock (&rules_mutex);

  newRule->rule = rule;
  newRule->next = *rules_head;
  *rules_head = newRule;

  pthread_mutex_unlock (&rules_mutex);

  return "Rule added";
}




bool
areRulesEqual (const struct firewallRule_t * a,
         const struct firewallRule_t * b)
{
  if (memcmp (a->ipaddr1, b->ipaddr1, sizeof (a->ipaddr1)) != 0)
    return false;
  if (memcmp (a->ipaddr2, b->ipaddr2, sizeof (a->ipaddr2)) != 0)
    return false;
  if (a->port1 != b->port1)
    return false;
  if (a->port2 != b->port2)
    return false;
  return true;
}

const char *
deleteRule (struct firewallRules_t **rules_head,
      const struct firewallRule_t *rule_to_delete)
{
  if (!rules_head || !*rules_head || !rule_to_delete)
    {
      return "Rule invalid";
    }

  struct firewallRules_t *current = *rules_head;
  struct firewallRules_t *prev = NULL;

  while (current != NULL)
    {
      if (areRulesEqual (current->rule, rule_to_delete))
  {
    if (prev == NULL)
      {
        *rules_head = current->next;
      }
    else
      {
        prev->next = current->next;
      }

    IpPortPairNode *current_query = current->rule->matched_queries;
    while (current_query != NULL)
      {
        IpPortPairNode *next_query = current_query->next;
        free (current_query->ip);
        free (current_query);
        current_query = next_query;
      }

    free (current->rule);
    free (current);
    return "Rule deleted";
  }
      prev = current;
      current = current->next;
    }

  return "Rule not found";
}

bool
checkIPAddress (int *ipaddr1, int *ipaddr2, int *ipaddr)
{
  int res;

  res = compareIPAddresses (ipaddr, ipaddr1);
  if (compareIPAddresses (ipaddr, ipaddr1) == 0)
    {
      return true;
    }
  else if (ipaddr2[0] == -1)
    {
      return false;
    }
  else if (res == -1)
    {
      return false;
    }
  else if (compareIPAddresses (ipaddr, ipaddr2) <= 0)
    {
      return true;
    }
  else
    {
      return false;
    }
}

bool
checkPort (int port1, int port2, int port)
{
  if (port == port1)
    {
      return true;
    }
  else if (port < port1)
    {
      return false;
    }
  else if (port2 == -1 || port > port2)
    {
      return false;
    }
  else
    {
      return true;
    }
}

const char *
checkAndRecordQuery (struct firewallRules_t **rules_head, const char *ip,
         int port)
{
  if (!validate_ip (ip))
    {
      return "Illegal IP address specified";
    }

  if (!validate_port (port))
    {
      return "Illegal port specified";
    }

  int ipaddr[4] = { 0 };
  if (parseIPaddress (ipaddr, (char *) ip) == NULL)
    {
      return "Illegal IP address format";
    }

  bool added = false;
  struct firewallRules_t *current = *rules_head;
  while (current != NULL)
    {
      if (checkIPAddress
    (current->rule->ipaddr1, current->rule->ipaddr2, ipaddr)
    && checkPort (current->rule->port1, current->rule->port2, port))
  {
    IpPortPairNode *query = current->rule->matched_queries;
    while (query != NULL)
      {
        query = query->next;
      }

    if (!added)
      {
        IpPortPairNode *newQuery =
    (IpPortPairNode *) malloc (sizeof (IpPortPairNode));
        if (!newQuery)
    {
      return "Memory allocation failed";
    }

        newQuery->ip = strdup (ip);
        if (!newQuery->ip)
    {
      free (newQuery);
      return "Memory allocation failed for IP";
    }

        newQuery->port = port;

        newQuery->next = current->rule->matched_queries;
        current->rule->matched_queries = newQuery;
        added = true;
      }
  }
      current = current->next;
    }

  if (added)
    {
      return "Connection accepted";
    }
  else
    {
      return "Connection rejected";
    }
}



char *
listAllRulesAndQueries (const struct firewallRules_t *rules_head)
{
  const struct firewallRules_t *current_rule = rules_head;
  char *output = NULL;
  size_t output_size = 0;
  FILE *stream = open_memstream (&output, &output_size);
  if (stream == NULL)
    {
      fprintf (stderr, "Error: open_memstream failed\n");
      return NULL;
    }

  while (current_rule != NULL)
    {
      fprintf (stream, "Rule: ");
      if (current_rule->rule->ipaddr1[0] != -1)
  {
    printIPaddress (stream, current_rule->rule->ipaddr1);

    if (compareIPAddresses
        (current_rule->rule->ipaddr1, current_rule->rule->ipaddr2) != 0)
      {
        fprintf (stream, "-");
        printIPaddress (stream, current_rule->rule->ipaddr2);
      }
  }

      fprintf (stream, " %d", current_rule->rule->port1);
      if (current_rule->rule->port1 != current_rule->rule->port2)
  {
    fprintf (stream, "-%d", current_rule->rule->port2);
  }

      fprintf (stream, "\n");

      const IpPortPairNode *current_query =
  current_rule->rule->matched_queries;
      while (current_query != NULL)
  {
    fprintf (stream, "Query: %s %d\n", current_query->ip,
       current_query->port);
    current_query = current_query->next;
  }

      current_rule = current_rule->next;
    }

  fclose (stream);
  return output;
}



void
cleanup_resources ()
{
  pthread_mutex_destroy (&rules_mutex);
  pthread_mutexattr_destroy (&rules_mutexattr);
  pthread_mutex_destroy (&threadEndLock);
  pthread_rwlock_destroy (&threadLock);
  pthread_cond_destroy (&threadCond);
}

void
error (const char *msg)
{
  perror (msg);
  exit (1);
}

void *
processRequest (void *arg)
{
  threadArgs_t *threadArgs = (threadArgs_t *) arg;
  int client_socket = threadArgs->newsockfd;
  int threadIndex = threadArgs->threadIndex;
  char buffer[BUFFERLENGTH];
  bzero (buffer, BUFFERLENGTH);
  char *response = NULL;

  ssize_t bytes_read = read (client_socket, buffer, BUFFERLENGTH - 1);

  if (bytes_read > 0)
    {
      buffer[bytes_read] = '\0';

      char *command = strtok (buffer, " ");
      char *restOfTheCommand = strtok (NULL, "\n");
      if (command)
  {
    if ((strcmp (command, "A") == 0 || strcmp (command, "D") == 0)
        && restOfTheCommand)
      {
        struct firewallRule_t *rule = readRule (restOfTheCommand);
        if (rule)
    {
      pthread_mutex_lock (&rules_mutex);
      const char *result = NULL;
      if (strcmp (command, "A") == 0)
        {
          result = addRule (&rules_head, rule);
        }
      else if (strcmp (command, "D") == 0)
        {
          result = deleteRule (&rules_head, rule);
        }
      pthread_mutex_unlock (&rules_mutex);
      response = strdup (result);
    }
        else
    {
      response = strdup ("Invalid rule format");
    }
      }
    else if (strcmp (command, "C") == 0 && restOfTheCommand)
      {

        char *ipAddress = strtok (restOfTheCommand, " ");
        char *portString = strtok (NULL, " ");
        int port;

        if (!ipAddress || !validate_ip (ipAddress))
    {
      response = strdup ("Illegal IP address specified");
    }
        else if (!portString || !(port = atoi (portString))
           || !validate_port (port))
    {
      response = strdup ("Illegal port specified");
    }
        else
    {

      pthread_mutex_lock (&rules_mutex);
      const char *checkResult =
        checkAndRecordQuery (&rules_head, ipAddress, port);
      pthread_mutex_unlock (&rules_mutex);
      response = strdup (checkResult);
    }
      }

    else if (strcmp (command, "L") == 0)
      {

        if (restOfTheCommand == NULL
      || strcmp (restOfTheCommand, "") == 0)
    {
      pthread_mutex_lock (&rules_mutex);
      response = listAllRulesAndQueries (rules_head);
      pthread_mutex_unlock (&rules_mutex);
      if (!response)
        {
          response =
      strdup ("Error: Could not retrieve rules list");
        }
    }
        else
    {
      response = strdup ("Illegal list request");
    }
      }
    else
      {
        response = strdup ("Illegal request");
      }
  }
      else
  {
    response = strdup ("Invalid request format");
  }

      if (response)
  {
    send (client_socket, response, strlen (response), 0);
    free (response);
  }
    }
  else if (bytes_read == 0)
    {
      printf ("Client disconnected\n");
    }
  else
    {
      perror ("Error reading from socket");
    }

  close (client_socket);

  pthread_rwlock_wrlock (&threadLock);
  serverThreads[threadIndex].status = THREAD_FINISHED;
  pthread_rwlock_unlock (&threadLock);

  pthread_cond_signal (&threadCond);

  free (threadArgs);

  return NULL;
}


int
findThreadIndex ()
{
  int i, tmp;

  for (i = 0; i < noOfThreads; i++)
    {
      if (serverThreads[i].status == THREAD_AVAILABLE)
  {
    serverThreads[i].status = THREAD_IN_USE;
    return i;
  }
    }

  pthread_rwlock_wrlock (&threadLock);
  serverThreads =
    realloc (serverThreads,
       ((noOfThreads +
         THREADS_ALLOCATED) * sizeof (struct threadInfo_t)));
  noOfThreads = noOfThreads + THREADS_ALLOCATED;
  pthread_rwlock_unlock (&threadLock);
  if (serverThreads == NULL)
    {
      fprintf (stderr, "Memory allocation failed\n");
      exit (1);
    }
  for (tmp = i + 1; tmp < noOfThreads; tmp++)
    {
      serverThreads[tmp].status = THREAD_AVAILABLE;
    }
  serverThreads[i].status = THREAD_IN_USE;
  return i;
}

void *
waitForThreads (void *args)
{
  int i;
  int res;
  while (1)
    {
      pthread_mutex_lock (&threadEndLock);
      pthread_cond_wait (&threadCond, &threadEndLock);
      pthread_mutex_unlock (&threadEndLock);
      pthread_rwlock_rdlock (&threadLock);
      for (i = 0; i < noOfThreads; i++)
  {
    if (serverThreads[i].status == THREAD_FINISHED)
      {
        res = pthread_join (serverThreads[i].pthreadInfo, NULL);
        if (res != 0)
    {
      fprintf (stderr, "thread joining failed, exiting\n");
      exit (1);
    }
        serverThreads[i].status = THREAD_AVAILABLE;
      }
  }
      pthread_rwlock_unlock (&threadLock);
    }
  return NULL;
}


int
main (int argc, char *argv[])
{
  socklen_t clilen;
  int sockfd, portno;
  struct sockaddr_in6 serv_addr, cli_addr;
  int result;
  initialize_mutexes ();
  pthread_t waitInfo;
  pthread_attr_t waitAttributes;

  pthread_mutex_init (&threadEndLock, NULL);
  pthread_rwlock_init (&threadLock, NULL);
  pthread_cond_init (&threadCond, NULL);

  if (argc < 2)
    {
      fprintf (stderr, "ERROR, no port provided\n");
      exit (1);
    }

  sockfd = socket (AF_INET6, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      error ("ERROR opening socket");
    }

  bzero ((char *) &serv_addr, sizeof (serv_addr));
  portno = atoi (argv[1]);
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_addr = in6addr_any;
  serv_addr.sin6_port = htons (portno);

  if (bind (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
    {
      error ("ERROR on binding");
    }

  listen (sockfd, 5);
  clilen = sizeof (cli_addr);

  if (pthread_attr_init (&waitAttributes))
    {
      fprintf (stderr, "Creating initial thread attributes failed!\n");
      close (sockfd);
      cleanup_resources ();
      exit (1);
    }

  result = pthread_create (&waitInfo, &waitAttributes, waitForThreads, NULL);
  if (result != 0)
    {
      fprintf (stderr, "Initial Thread creation failed!\n");
      close (sockfd);
      pthread_attr_destroy (&waitAttributes);
      cleanup_resources ();
      exit (1);
    }

  while (1)
    {
      struct threadArgs_t *threadArgs = malloc (sizeof (struct threadArgs_t));
      if (!threadArgs)
  {
    fprintf (stderr, "Memory allocation failed!\n");
    continue;
  }

      threadArgs->newsockfd =
  accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (threadArgs->newsockfd < 0)
  {
    free (threadArgs);
    perror ("ERROR on accept");
    continue;
  }

      int threadIndex = findThreadIndex ();
      threadArgs->threadIndex = threadIndex;
      if (pthread_attr_init (&(serverThreads[threadIndex].attributes)))
  {
    fprintf (stderr, "Creating thread attributes failed!\n");
    close (threadArgs->newsockfd);
    free (threadArgs);
    continue;
  }

      result = pthread_create (&(serverThreads[threadIndex].pthreadInfo),
             &(serverThreads[threadIndex].attributes),
             processRequest, (void *) threadArgs);
      if (result != 0)
  {
    fprintf (stderr, "Thread creation failed!\n");
    pthread_attr_destroy (&(serverThreads[threadIndex].attributes));
    close (threadArgs->newsockfd);
    free (threadArgs);
    continue;
  }
    }

  cleanup_resources ();
  close (sockfd);
  return 0;
}
