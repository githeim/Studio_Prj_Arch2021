#include <string>
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
#include <signal.h>
#include <time.h>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
// Define the Packet Constants
// ping packet size
#define PING_PKT_S 64

// Automatic port number
#define PORT_NO 0

// Automatic port number
#define PING_SLEEP_RATE 400000 

// Gives the timeout delay for receiving packets
// in seconds
#define RECV_TIMEOUT 1

// Define the Ping Loop
int pingloop=1;


// ping packet structure
struct ping_pkt
{
	struct icmphdr hdr;
	char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

// Calculating the Check Sum
unsigned short checksum(void *b, int len)
{ 
  unsigned short *buf =(unsigned short *) b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}


// Interrupt handler
void intHandler(int dummy)
{
	pingloop=0;
}

// Performs a DNS lookup
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con)
{
	printf("\nResolving DNS..\n");
	struct hostent *host_entity;
	char *ip=(char*)malloc(NI_MAXHOST*sizeof(char));
	int i;

	if ((host_entity = gethostbyname(addr_host)) == NULL)
	{
		// No ip found for hostname
		return NULL;
	}
	
	//filling up address structure
	strcpy(ip, inet_ntoa(*(struct in_addr *)
						host_entity->h_addr));

	(*addr_con).sin_family = host_entity->h_addrtype;
	(*addr_con).sin_port = htons (PORT_NO);
	(*addr_con).sin_addr.s_addr = *(long*)host_entity->h_addr;

	return ip;
	
}

// Resolves the reverse lookup of the hostname
char* reverse_dns_lookup(char *ip_addr)
{
	struct sockaddr_in temp_addr;	
	socklen_t len;
	char buf[NI_MAXHOST], *ret_buf;

	temp_addr.sin_family = AF_INET;
	temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
	len = sizeof(struct sockaddr_in);

	if (getnameinfo((struct sockaddr *) &temp_addr, len, buf,
					sizeof(buf), NULL, 0, NI_NAMEREQD))
	{
		printf("Could not resolve reverse lookup of hostname\n");
		return NULL;
	}
	ret_buf = (char*)malloc((strlen(buf) +1)*sizeof(char) );
	strcpy(ret_buf, buf);
	return ret_buf;
}

// make a ping request
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr,
				char *ping_dom, char *ping_ip, char *rev_host)
{
	int ttl_val=64, msg_count=0, i, addr_len, flag=1,
			msg_received_count=0;
	
	struct ping_pkt pckt;
	struct sockaddr_in r_addr;
	struct timespec time_start, time_end, tfs, tfe;
	long double rtt_msec=0, total_msec=0;
	struct timeval tv_out;
	tv_out.tv_sec = RECV_TIMEOUT;
	tv_out.tv_usec = 0;

	clock_gettime(CLOCK_MONOTONIC, &tfs);

	
	// set socket options at ip to TTL and value to 64,
	// change to what you want by setting ttl_val
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,
			&ttl_val, sizeof(ttl_val)) != 0)
	{
		printf("\nSetting socket options to TTL failed!\n");
		return;
	}

	else
	{
		printf("\nSocket set to TTL..\n");
	}

	// setting timeout of recv setting
	setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,
				(const char*)&tv_out, sizeof tv_out);

	// send icmp packet in an infinite loop
	while(pingloop)
	{
		// flag is whether packet was sent or not
		flag=1;
	
		//filling packet
		bzero(&pckt, sizeof(pckt));
		
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = getpid();
		
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = msg_count++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));


		usleep(PING_SLEEP_RATE);

		//send packet
		clock_gettime(CLOCK_MONOTONIC, &time_start);
		if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0,
		(struct sockaddr*) ping_addr,
			sizeof(*ping_addr)) <= 0)
		{
			printf("\nPacket Sending Failed!\n");
			flag=0;
		}

		//receive packet
		addr_len=sizeof(r_addr);

		if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0,
			(struct sockaddr*)&r_addr, (socklen_t*)&addr_len) <= 0
			&& msg_count>1)
		{
			printf("\nPacket receive failed!\n");
		}

		else
		{
			clock_gettime(CLOCK_MONOTONIC, &time_end);
			
			double timeElapsed = ((double)(time_end.tv_nsec -
								time_start.tv_nsec))/1000000.0;
			rtt_msec = (time_end.tv_sec-
						time_start.tv_sec) * 1000.0
						+ timeElapsed;
			
			// if packet was not sent, don't receive
			if(flag)
			{
				if(!(pckt.hdr.type ==69 && pckt.hdr.code==0))
				{
					printf("Error..Packet received with ICMP type %d code %d\n",
						pckt.hdr.type, pckt.hdr.code);
				}
				else
				{
					printf("%d bytes from %s (h: %s) (%s) msg_seq=%d ttl=%d"
                 " rtt = %Lf ms.\n",
						PING_PKT_S, ping_dom, rev_host,
						ping_ip, msg_count,
						ttl_val, rtt_msec);

					msg_received_count++;
				}
			}
		}	
	}
	clock_gettime(CLOCK_MONOTONIC, &tfe);
	double timeElapsed = ((double)(tfe.tv_nsec -
						tfs.tv_nsec))/1000000.0;
	
	total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+timeElapsed;
					
	printf("\n===%s ping statistics===\n", ping_ip);
	printf("\n%d packets sent, %d packets received, %f percent"
		"packet loss. Total time: %Lf ms.\n\n",
		msg_count, msg_received_count,
		((msg_count - msg_received_count)/msg_count) * 100.0,
		total_msec);
}
// make a ping request
int Send_SinglePing(int ping_sockfd, struct sockaddr_in *ping_addr,
				char *ping_dom, char *ping_ip, char *rev_host, long double& ldRTT_Msec )
{
	int ttl_val=64, msg_count=0, i, addr_len, flag=1,
			msg_received_count=0;
	
	struct ping_pkt pckt;
	struct sockaddr_in r_addr;
	struct timespec time_start, time_end, tfs, tfe;
	long double rtt_msec=0, total_msec=0;
	struct timeval tv_out;
	tv_out.tv_sec = RECV_TIMEOUT;
	tv_out.tv_usec = 0;

	clock_gettime(CLOCK_MONOTONIC, &tfs);

	
	// set socket options at ip to TTL and value to 64,
	// change to what you want by setting ttl_val
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,
			&ttl_val, sizeof(ttl_val)) != 0)
	{
		printf("\nSetting socket options to TTL failed!\n");
		return -1;
	}

	else
	{
		printf("\nSocket set to TTL..\n");
	}

	// setting timeout of recv setting
	setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,
				(const char*)&tv_out, sizeof tv_out);



  // send icmp packet single time
  // flag is whether packet was sent or not
  flag=1;

  //filling packet
  bzero(&pckt, sizeof(pckt));

  pckt.hdr.type = ICMP_ECHO;
  pckt.hdr.un.echo.id = getpid();

  for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
    pckt.msg[i] = i+'0';

  pckt.msg[i] = 0;
  pckt.hdr.un.echo.sequence = msg_count++;
  pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));


  usleep(PING_SLEEP_RATE);

  //send packet
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0,
        (struct sockaddr*) ping_addr,
        sizeof(*ping_addr)) <= 0)
  {
    printf("\nPacket Sending Failed!\n");
    flag=0;
  }

  //receive packet
  addr_len=sizeof(r_addr);

  if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0,
        (struct sockaddr*)&r_addr, (socklen_t*)&addr_len) <= 0
      && msg_count>1)
  {
    printf("\nPacket receive failed!\n");
  }

  else
  {
    clock_gettime(CLOCK_MONOTONIC, &time_end);

    double timeElapsed = ((double)(time_end.tv_nsec -
          time_start.tv_nsec))/1000000.0;
    rtt_msec = (time_end.tv_sec-
        time_start.tv_sec) * 1000.0
      + timeElapsed;

    // if packet was not sent, don't receive
    if(flag)
    {
      if(!(pckt.hdr.type ==69 && pckt.hdr.code==0))
      {
        printf("Error..Packet received with ICMP type %d code %d\n",
            pckt.hdr.type, pckt.hdr.code);
      }
      else
      {
        printf("\033[1;32m%d bytes from %s (h: %s) (%s) msg_seq=%d ttl=%d"
            " rtt = %Lf ms.\n \033[m",
            PING_PKT_S, ping_dom, rev_host,
            ping_ip, msg_count,
            ttl_val, rtt_msec);
        msg_received_count++;
      }
    }
  }	


  ldRTT_Msec = rtt_msec;

  return 0;
}

static char *g_pIP_addr, *g_pReverse_hostname;
static struct sockaddr_in g_addr_con;
static int g_iSockfd;
std::string g_strAddr;
int SetupJitterMeter(std::string strAddr) {
	g_pIP_addr = dns_lookup((char*)strAddr.c_str(), &g_addr_con);
	if(g_pIP_addr == NULL)
	{
		printf("\nDNS lookup failed! Could"
			"	not resolve hostname!\n");
    printf("\033[1;31m[%s][%d] :x: DNS lookup failed![%s] \033[m\n",
        __FUNCTION__,__LINE__,strAddr.c_str());
		return -1;
	}
	g_pReverse_hostname = reverse_dns_lookup(g_pIP_addr);
	printf("\nTrying to connect to '%s' IP: %s\n",
									strAddr.c_str(), g_pIP_addr);
	printf("\nReverse Lookup domain: %s",
						g_pReverse_hostname);

	g_iSockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(g_iSockfd<0)
	{
		printf("\nSocket file descriptor not received!!\n");
		return -1;
	}
	else
		printf("\nSocket file descriptor %d received\n", g_iSockfd);

  g_strAddr = strAddr;

  return 0;
}

int GetJitter(long double &ldJitterTime_ms) {

  if (Send_SinglePing 	(g_iSockfd, &g_addr_con, g_pReverse_hostname,
								g_pIP_addr, (char*)g_strAddr.c_str(),ldJitterTime_ms))
    return -1;
	
  return 0;
}

static std::thread* g_pThrUpdateJitter = nullptr;
static bool g_bUpdateJitter = false;
std::vector<long double> g_vecJitters;
long double g_ldCurrentJitter_ms;
std::mutex g_mtxCurrentJitter_ms;

void LoopUpdateJitter(std::string strAddr) {
  long double ldJitterTime_ms;
  while (g_bUpdateJitter) {
    GetJitter(ldJitterTime_ms);
    g_mtxCurrentJitter_ms.lock();
    g_ldCurrentJitter_ms = ldJitterTime_ms;
    g_mtxCurrentJitter_ms.unlock();
  }

}
int StartUpdateJitter(std::string strAddr) {

  if (SetupJitterMeter(strAddr) ) {
    return -1;
  }
  printf("\033[1;33m[%s][%d] :x: Jitter Update Start \033[m\n",__FUNCTION__,__LINE__);
  g_bUpdateJitter = true;
  g_pThrUpdateJitter = new std::thread(LoopUpdateJitter,strAddr);
  return 0;
}

void StopUpdateJitter() {
  g_bUpdateJitter = false;
  g_pThrUpdateJitter->join();
  printf("\033[1;33m[%s][%d] :x: Jitter Update Finished \033[m\n",__FUNCTION__,__LINE__);
}

int GetCurrentJitter(long double& ldJitterTime_ms) {
  g_mtxCurrentJitter_ms.lock();
  ldJitterTime_ms = g_ldCurrentJitter_ms;
  g_mtxCurrentJitter_ms.unlock();
  return 0;
}
