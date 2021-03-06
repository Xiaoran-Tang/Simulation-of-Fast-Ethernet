// Communication Switch Process
/* source: tcpcliserv/tcpservselect01.c */

#include "unp.h"

const int SETSIZE = 10;          // assuming at most 10 SP's
const int MAXBUF = 10;           // buffer length for request queue in CSP
const int MAXSPNO = 100;         // max SP_id value

int
main(int argc, char **argv)
{
   int               i, j, maxi, maxfd, listenfd, connfd, sockfd;
   int               nready, client[SETSIZE], sp[MAXSPNO + 1];
   ssize_t           n;
   int               sp_id;      // index of SP
   int               src;        // source sp_id
   int               curr;       // CSP request frame queue counter 
   fd_set            rset, allset;
   char              ip[46];
   socklen_t         clilen;
   struct sockaddr_in   cliaddr, servaddr;
   int               sp_count;   // to be used for exit CSP
   typedef enum {false = 0, true = 1} bool;
   bool              has_sp;     // to be used for exit CSP

   // the frame has fixed size (4 * 3 + 10 = 22 bytes)
   struct frame {
      int   seq;                 // sequence number
      int   src;                 // source address
      int   dest;                // destination address
      char  data[10];            // data section
   } frame;                      // frame for receive and send

   listenfd = Socket(AF_INET, SOCK_STREAM, 0);

   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family      = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port        = htons(SERV_PORT);

   Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

   Listen(listenfd, LISTENQ);

   maxfd = listenfd;             // initialization
   maxi = -1;                    // index into client[] array
   for (i = 0; i < SETSIZE; i++) 
      client[i] = -1;            // -1 indicates available entry
   for (i = 0; i < MAXSPNO; i++) 
      sp[i + 1] = -1;            // -1 indicates SP not connected
   curr = 0;
   sp_count = 0;
   has_sp = false;
   FD_ZERO(&allset);
   FD_SET(listenfd, &allset);

   for ( ; ; ) {
      rset = allset;       
      nready = Select(maxfd+1, &rset, NULL, NULL, NULL);

      if (FD_ISSET(listenfd, &rset)) {          // new client connection
         clilen = sizeof(cliaddr);
         connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
         Inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
         
         Read(connfd, &sp_id, sizeof(sp_id));
         printf("new connection from SP%d: %s, port %d\n", sp_id, ip,
               ntohs(cliaddr.sin_port));

         sp[sp_id] = connfd;                    // save sp_id - connfd pairs
         for (i = 0; i < SETSIZE; i++)
            if (client[i] < 0) {
               client[i] = connfd;              // save client_i - connfd pairs
               break;
            }
         if (i >= SETSIZE)
            err_quit("too many clients");

         FD_SET(connfd, &allset);
         if (connfd > maxfd)
            maxfd = connfd;
         if (i > maxi)
            maxi = i;
         
         sp_count++;                            // to be used for exit CSP
         if (sp_count == 1)
            has_sp = true;                      // to be used for exit CSP
            
         if (--nready <= 0)
            continue;
      }

      for (i = 0; i <= maxi; i++) {             // check all clients
         if ( (sockfd = client[i]) < 0)
            continue;
         if (FD_ISSET(sockfd, &rset)) {
            if ( (n = Read(sockfd, &frame, MAXLINE)) == 0) {
               for (j = 1; j < MAXSPNO; j++)
                  if (sp[j] == sockfd) {
                     sp[j] = -1;                // reset fd to -1 in sp array
                     break;
                  }
               Close(sockfd);
               FD_CLR(sockfd, &allset);
               client[i] = -1;
               if (has_sp && --sp_count == 0)   // if sp_count reduce to 0
                  exit(0);                      // then CSP exit
            } else {
               if (frame.dest > 0) {            // request frame from SP
                  // use sockfd to get the source sp_id value: src
                  for (j = 1; j < MAXSPNO; j++)
                     if (sp[j] == sockfd) {
                        src = j;
                        break;
                     }
                  // alternatively, we can also get src from frame.src
                  src = frame.src;
                  printf("Receive request from SP %d\n", frame.src);
                  if (sp[frame.dest] == -1) {   // destination SP not exist
                     printf("Send negative reply to SP %d\n", frame.dest);
                     frame.src = -frame.src;    // indicate Neg
                     frame.dest = -frame.dest;  // indicate Neg
                     Writen(sp[src], &frame, sizeof(frame));
                     continue;
                  }
                  frame.src = -frame.src;       // indicate Pos/Neg frame
                  if (curr < MAXBUF) {          // request frame queue not full
                     printf("Send postive reply to SP %d\n", frame.dest);
                     curr++;                    // add to request frame queue
                     Writen(sp[src], &frame, sizeof(frame)); // Pos reply
                  } else {                      // request frame queue full
                     printf("Send negative reply to SP %d\n", frame.dest);
                     frame.src = -frame.src;    // indicate Neg
                     frame.dest = -frame.dest;  // indicate Neg
                     Writen(sp[src], &frame, sizeof(frame)); // Neg reply
                  }
               } else {                         // data frame from SP
                  printf("Receive data frame %d from SP %d (to SP %d)\n", 
                         frame.seq, frame.src, -frame.dest);
                  frame.dest = -frame.dest;
                  Writen(sp[frame.dest], &frame, sizeof(frame));
                  printf("Forward data frame %d (from SP %d) to SP %d\n", 
                         frame.seq, frame.src, frame.dest);
                  curr--;                       // remove from request queue
               }
            }

            if (--nready <= 0)
               break;
         }
      }
   }
}
