// Station Process

#include "unp.h"

const int MAXREQ = 3;       // max # to retransmit the request frame
const int MAXSPNO = 100;    // max SP_id value

void
str_cli_(FILE *fp, int sockfd, int sp_id);

int
main(int argc, char **argv)
{
   int                  sockfd;
   struct sockaddr_in   servaddr;
   int                  sp_id;         //SP_Number, e.g. 1,2,3...100
   
   if (argc != 3)
      err_quit("usage: sp <IPaddress> <SP_Number>\n");

   sp_id = atoi(argv[2]);
   if (sp_id < 1 || sp_id > MAXSPNO)   // verify sp_id range
      err_quit("error: 1 <= SP_Number <= %d\n", MAXSPNO);
   printf("This Station Process is SP%d\n", sp_id);
   
   sockfd = Socket(AF_INET, SOCK_STREAM, 0);

   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(SERV_PORT);
   Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

   Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
   Writen(sockfd, &sp_id, sizeof(sp_id));
   
   str_cli_(stdin, sockfd, sp_id);

   exit(0);
}

// argument "sp_id" is the current index of Station Process
// e.g. for SP3, sp_id = 3
void
str_cli_(FILE *fp, int sockfd, int sp_id)
{
   int         maxfdp1, stdineof;
   fd_set      rset;
   char        buf[MAXLINE];
   char        temp1[20], temp2[20], temp3[20]; // space holder in sscanf
   int         n;
   int         curr;          // retransmit request frame failure counter
   
   // the frame has fixed size (4 * 3 + 10 = 22 bytes)
   struct frame {
      int   seq;              // sequence number
      int   src;              // source address
      int   dest;             // destination address
      char  data[10];         // data section
   };
   struct frame send, rcev;   // frame for send, frame for receive
   
   stdineof = 0;
   send.src = sp_id;          // source address is fixed for a certain SP
   curr = 0;
   FD_ZERO(&rset);
   
   for ( ; ; ) {
      if (stdineof == 0)
         FD_SET(fileno(fp), &rset);
      FD_SET(sockfd, &rset);
      maxfdp1 = max(fileno(fp), sockfd) + 1;
      Select(maxfdp1, &rset, NULL, NULL, NULL);

      if (FD_ISSET(sockfd, &rset)) {
         if ( (n = Read(sockfd, &rcev, MAXLINE)) == 0) {
            if (stdineof == 1)
               return;
            else
               err_quit("str_cli: server terminated prematurely");
         }
         if (rcev.src > 0)                   // data frame from CSP
            printf("Receive (via CSP) a data frame %d from SP %d\n", 
                   rcev.seq, rcev.src);
         else if (rcev.dest > 0) {           // Pos reply frame from CSP
            printf("Receive positive reply (permission) from CSP ");
            printf("to send data frame %d to SP %d\n", 
                   rcev.seq, rcev.dest);
            send.seq = rcev.seq;
            send.src = -rcev.src;
            send.dest = -rcev.dest;
            Writen(sockfd, &send, sizeof(send));
            printf("Send (via CSP) data frame %d to SP %d\n",
                   send.seq, -send.dest);
         } else {                            // Neg reply frame from CSP
            if (curr++ < MAXREQ) {           // retransmit 3 times
               printf("Receive reject reply from CSP ");
               printf("to send data frame %d to SP %d\n", 
                      rcev.seq, -rcev.dest);
               send.seq = rcev.seq;
               send.src = -rcev.src;
               send.dest = -rcev.dest;
               Writen(sockfd, &send, sizeof(send));
               printf("Retransmit request to CSP to send data ");
               printf("frame %d to SP %d ", send.seq, send.dest);
               printf("(count: %d)\n", curr);
            } else
               curr = 0;
         }
      }
      
      if (FD_ISSET(fileno(fp), &rset)) {     // (stdin) input is readable 
         if ( (n = Read(fileno(fp), buf, MAXLINE)) == 0) {
            stdineof = 1;
            Shutdown(sockfd, SHUT_WR);
            FD_CLR(fileno(fp), &rset);
            continue;
         }
         if (sscanf(buf, "%s %d, %s %s %d",  // scan from input string
             temp1, &send.seq, temp2, temp3, &send.dest) != 5) {
            printf("usage: Frame <number>, To SP <number>\n");
            printf("Invalid input: %s", buf);
            continue;
         }
         
         Writen(sockfd, &send, sizeof(send));
         printf("Send request to CSP to send data frame %d to SP %d\n", 
                send.seq, send.dest);
      }
      
   }
}
