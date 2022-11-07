#include <netinet/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#define IP_HDRLEN 20
#define ICMP_HDRLEN 8
#define Ether_Hdr_Len 14
#define Packet_Len Ether_Hdr_Len + IP_HDRLEN + ICMP_HDRLEN
#define IP_ADDR_LEN 4
#define DEVICE_NAME "enp0s3"


unsigned short checksum(unsigned short *b,int len){
    unsigned long sum = 0xffff;

    while(len>1){
        sum+= *b;
        b++;
        len-=2;
    }
    if(len==1){
        sum += *(unsigned char*)b;
    }

    sum = (sum & 0xffff) + (sum >>16);
    sum = (sum & 0xffff) + (sum >>16);

    return ~sum;
}

/*void fill_eth_h(struct ether_header* eth_hdr,unsigned char* src_mac,unsigned char* dst_mac){
    memcpy(eth_hdr->ether_shost,&src_mac,ETH_ALEN);
    memcpy(eth_hdr->ether_dhost,&dst_mac,ETH_ALEN);
    eth_hdr->ether_type = htons(ETH_P_IP);
}*/
void fill_ip_h(struct ip *ip_hdr,char *dst ,int ttl){
    struct hostent *src_hp,*dst_hp;
    struct in_addr dst_ip;
    char name[256];
    if(gethostname(name,sizeof(name))<0){
        perror("get host name error");
        exit(EXIT_FAILURE);
    }else{
        src_hp = gethostbyname(name);
        ip_hdr->ip_src = (*(struct in_addr *)src_hp->h_addr);
    }
    inet_pton(AF_INET,dst,&dst_ip);
    ip_hdr->ip_dst = dst_ip;
    //ip_hdr->ip_src.s_addr = (struct in_addr_t)src_inaddr;
    //ip_hdr->ip_dst.s_addr = dst_inaddr;
    ip_hdr->ip_v = 4;
    ip_hdr->ip_hl = 20;
    ip_hdr->ip_len = IP_HDRLEN+ICMP_HDRLEN; //not sure
    ip_hdr->ip_id = htons(321);
    ip_hdr->ip_off = htons(0);
    ip_hdr->ip_ttl = (unsigned char)ttl;
    ip_hdr->ip_p = IPPROTO_ICMP;
    ip_hdr->ip_sum = 0;
}
void fill_icmp_h(struct icmp* icmp_hdr,int *msg_count){
    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = 123;
    icmp_hdr->icmp_seq = 0;
}
int main(int argc ,char* argv[]){
    int sockfd_recv = 0, sockfd_send = 0;
    char send_buf[98],recv_buf[98];
    struct sockaddr_in sa;
    struct ifreq req;
    unsigned char src_mac[ETH_ALEN],dst_mac[ETH_ALEN];
    char *src_ip;
    if(argc!=3){
        printf("usage : ./ERS hop destination_ip\n");
        return 0;
    }
    
    if(sockfd_send = socket(PF_INET, SOCK_RAW ,htons(ETH_P_IP)) < 0)
	{
		perror("open send socket error\n");
		exit(sockfd_send);
	}
    if(sockfd_recv = socket(PF_INET, SOCK_RAW ,htons(ETH_P_IP)) < 0) //open receive socket
	{
		perror("open recv socket error");
		exit(1);
	}
    /*memset(&req,'\0',sizeof(req));
	strncpy(req.ifr_name, DEVICE_NAME,IF_NAMESIZE-1);
    if(ioctl(sockfd_send,SIOGIFINDEX,&req) < 0){//get interface index and store in req
			perror("get ifindex failed\n");
	}
	if(ioctl(sockfd_send,SIOCGIFADDR,&req) < 0){//get interface ip and store in req
		perror("get ip failed\n");
	}
	if(ioctl(sockfd_send,SIOCGIFHWADDR,&req) < 0){//get interface MAC and store in req
		perror("get MAC failed\n");
	}*/
    memset(&sa,'\0',sizeof(sa));
    sa.sin_family = PF_INET;
    inet_pton(PF_INET,argv[2],&sa.sin_addr);


    return 0;
}