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


unsigned short checksum(unsigned short *addr,int len){
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short result = 0;

    while(nleft>1){
        sum += *w++;
        nleft-=2;
    }

    if(nleft == 1){
        *(unsigned *)(&result) = *(unsigned char *)w;
        sum+=result;
    }
    sum =(sum >> 16)+(sum & 0xffff);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

void fill_ip_h(struct ip* ip_hdr,struct sockaddr src,char *dst ,int ttl){
    //struct hostent *src_hp,*dst_hp;
    struct in_addr dst_ip,src_ip;
    char name[256];
    //src_ip = ((struct sockaddr_in *)&src)->sin_addr;
    //ip_hdr->ip_src = src_ip;
    inet_pton(AF_INET,dst,&dst_ip);
    ip_hdr->ip_dst = dst_ip;
    //ip_hdr->ip_src.s_addr = (struct in_addr_t)src_inaddr;
    //ip_hdr->ip_dst.s_addr = dst_inaddr;
    ip_hdr->ip_v = 4;
    ip_hdr->ip_hl = 5;
    ip_hdr->ip_len = IP_HDRLEN+ICMP_HDRLEN; //not sure
    ip_hdr->ip_id = htons(321);
    ip_hdr->ip_off = htons(0);
    ip_hdr->ip_ttl = (unsigned char)ttl;
    ip_hdr->ip_p = IPPROTO_ICMP;
    ip_hdr->ip_sum = 0;
}
void fill_icmp_h(struct icmp* icmp_hdr){
    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = 123;
    icmp_hdr->icmp_seq = 0;
}

int main(int argc ,char* argv[]){
    int sockfd = 0, sockfd_recv = 0;
    int ttl = 0;
    int send_result=0,recv_result=0;
    char send_buf[100],recv_buf[70];
    struct ip *ip_hdr;
    struct icmp *icmp_hdr;
    struct sockaddr_in sa;
    struct in_addr dst,*src;
    struct ifreq req;
    char src_ip[15];
    char *my_ip;
    fd_set sockfds;
    if(argc!=3){
        printf("usage : ./ERS hop destination_ip\n");
        return 0;
    }
    
    if((sockfd = socket(AF_INET, SOCK_RAW ,IPPROTO_RAW)) < 0)
	{
		perror("open socket error\n");
		exit(sockfd);
	}
    if((sockfd_recv = socket(PF_PACKET, SOCK_RAW ,htons(ETH_P_ALL))) < 0)
	{
		perror("open recv socket error\n");
		exit(sockfd_recv);
	}
    memset(&sa,'\0',sizeof(sa));
    memset(send_buf,'\0',sizeof(send_buf));
    memset(&req,'\0',sizeof(req));
	strncpy(req.ifr_name, DEVICE_NAME,IF_NAMESIZE-1);
	if(ioctl(sockfd,SIOCGIFADDR,&req) < 0){//get interface ip and store in req
		perror("get ip failed\n");
	}
    my_ip = inet_ntoa(((struct sockaddr_in *)&(req.ifr_addr))->sin_addr);
    inet_pton(AF_INET,argv[2],&dst);
    sa.sin_family = AF_INET;
    sa.sin_addr = dst;
    char *p;
    ttl = strtol(argv[1],&p,10);// change number string to int 

    struct sockaddr from;
    int from_addr_len = sizeof(from);
    for(int i=1;i<=ttl;i++){{
        memset(send_buf,'\0',sizeof(send_buf));
        ip_hdr =(struct ip *)send_buf;
        icmp_hdr =(struct icmp *) (send_buf+IP_HDRLEN);
        // fill packet
        fill_ip_h(ip_hdr,req.ifr_addr,argv[2],i);
        ip_hdr->ip_sum = checksum((unsigned short *)send_buf, ip_hdr->ip_hl);
        fill_icmp_h(icmp_hdr);
        icmp_hdr->icmp_cksum = checksum((unsigned short *)icmp_hdr,sizeof(send_buf)-sizeof(struct icmp));
        //printf("header length : %d\n",ip_hdr->ip_hl);
        // send 
        if(send_result = sendto(sockfd,send_buf,sizeof(send_buf),0,(struct sockaddr *)&sa,sizeof(struct sockaddr_in))<0){
            perror("send packet failed\n");
        }
        while(1){
            memset(recv_buf,'\0',sizeof(recv_buf));
            recvfrom(sockfd_recv,recv_buf,sizeof(recv_buf),0,&from,(socklen_t *)from_addr_len);
            ip_hdr =(struct ip *)(recv_buf + ETHER_HDR_LEN);
            struct in_addr inaddr = ip_hdr->ip_src;
            inet_ntop(AF_INET,&inaddr,src_ip,INET_ADDRSTRLEN);
            if(ip_hdr->ip_p==IPPROTO_ICMP && strcmp(src_ip,my_ip)!=0){ //icmp packet
                icmp_hdr = (struct icmp *)(recv_buf + ETHER_HDR_LEN + IP_HDRLEN);
                if(icmp_hdr->icmp_type==ICMP_TIMXCEED){ // if icmp packet is time exceed
                    printf("%d hop Src ip : %s  (time exceed)\n",i,src_ip);
                    break;
                }else if(icmp_hdr->icmp_type==ICMP_ECHOREPLY){ // if icmp packet is reply echo
                    printf("%d hop Src ip : %s  (icmp reply)\n",i,src_ip);
                    return 0;
                }else{
                    printf("unhandle icmp type :%d code :%d by %s  \n",icmp_hdr->icmp_type,icmp_hdr->icmp_code,src_ip);
                    break;
                }
            }
        }
    }

    return 0;
}