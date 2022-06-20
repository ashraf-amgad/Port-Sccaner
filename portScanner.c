#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#define TCP_PROTOCOL 1
#define UDP_PROTOCOL 2

int rx_packet(int recvfd)
{
    struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    char buf[256];
    fd_set fds;
    struct ip *iphdr;
    int iplen;
    struct icmp *icmp;

    while (1)
    {

        FD_ZERO(&fds);
        FD_SET(recvfd, &fds);

        if (select(recvfd + 1, &fds, NULL, NULL, &tv_out) > 0)
        {
            recvfrom(recvfd, &buf, sizeof(buf), 0x0, NULL, NULL);
        }
        else if (!FD_ISSET(recvfd, &fds))
            return 1;
        else
            perror("*** recvfrom() failed ***");

        iphdr = (struct ip *)buf;
        iplen = iphdr->ip_hl << 2;

        icmp = (struct icmp *)(buf + iplen);

        if ((icmp->icmp_type == ICMP_UNREACH) && (icmp->icmp_code == ICMP_UNREACH_PORT))
            return 0;
    }

}


// Driver Code
int main(int argc, char *argv[])
{
    int sendfd, recvfd;
    char ip_addr[100];
    struct sockaddr_in addr_con;
    struct servent *srvport;
    char UDP_txMsg[5] = "PING";
    int port_counter, target_Protocol;
    struct hostent *host_entity;

    int Port_Low = atoi(argv[3]);
    int Port_High = atoi(argv[4]);

    if (argc != 5)
    {
        perror("Please try again with :: <hostname>  <protocol>  <portlow>  <porthigh>  ....\n");
        exit(-1);
    }

    argv[2][3] = '\0';
    if ((strcmp(argv[2], "TCP") == 0) || (strcmp(argv[2], "tcp") == 0))
    {
        target_Protocol = TCP_PROTOCOL;
    }
    else if ((strcmp(argv[2], "UDP") == 0) || (strcmp(argv[2], "udp") == 0))
    {
        target_Protocol = UDP_PROTOCOL;
    }
    else
    {
        perror("Error ::: Please Enter a valid protocol <TCP> or <UDP> ....\n");
        exit(-1);
    }

    if (isdigit(argv[1][0]) != 0)
    {
        strcpy(ip_addr, argv[1]);
    }
    else
    {

        if ((host_entity = gethostbyname(argv[1])) == NULL)
        {
            herror("n *** gethostbyname() failed ***n");
            exit(-1);
        }

        //filling up address structure
        strcpy(ip_addr, inet_ntoa(*(struct in_addr *)
                                       host_entity->h_addr));
    }

    printf("-------------------------------------------\n");
    printf("Scanning ... \n");
    printf("\thost = %s , protocol = %s , ports = %d -> %d \n", ip_addr, argv[2], Port_Low, Port_High);
    printf("-------------------------------------------\n");

    if (target_Protocol == TCP_PROTOCOL) // tcp scan
    {
        for (port_counter = Port_Low; port_counter <= Port_High; port_counter++)
        {
            // open stream socket
            if ((sendfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("*** socket(,SOCK_STREAM,) failed ***n");
                exit(-1);
            }

            bzero(&addr_con, sizeof(addr_con));
            addr_con.sin_family = AF_INET;
            addr_con.sin_port = htons(port_counter);
            addr_con.sin_addr.s_addr = inet_addr(ip_addr);

            if ( (connect(sendfd, (struct sockaddr *)&addr_con, sizeof(addr_con))) >= 0)
            {
                srvport = getservbyport(htons(port_counter), "tcp");

                if (srvport != NULL)
                    printf("port <%d>   open : \t%s\n", port_counter, srvport->s_name);

                fflush(stdout);
            }
            close(sendfd);
        } //end of for()
    }

    else // udp scan
    {
        // open send UDP socket
        if ((sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            perror("*** socket(,,IPPROTO_UDP) failed ***n");
            exit(-1);
        }
        // open receive ICMP socket
        if ((recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        {
            perror("*** socket(,,IPPROTO_ICMP) failed ***n");
            exit(-1);
        }

        for (port_counter = Port_Low; port_counter <= Port_High; port_counter++)
        {

            bzero(&addr_con, sizeof(addr_con));
            addr_con.sin_family = AF_INET;
            addr_con.sin_port = htons(port_counter);
            addr_con.sin_addr.s_addr = inet_addr(ip_addr);

            if (sendto(sendfd, UDP_txMsg, sizeof(UDP_txMsg), 0, (struct sockaddr *)&addr_con, sizeof(addr_con)) < 0)
            {
                perror("*** sendto() failed ***");
                exit(-1);
            }

            if (rx_packet(recvfd) == 1)
            {
                srvport = getservbyport(htons(port_counter), "udp");

                if (srvport != NULL)
                    printf("port <%d>   open : \t%s\n", port_counter, srvport->s_name);

                fflush(stdout);
            }
        }

        close(sendfd);
        close(recvfd);
    }
}
