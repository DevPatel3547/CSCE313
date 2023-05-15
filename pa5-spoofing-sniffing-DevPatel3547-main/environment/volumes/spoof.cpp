#include "common.h"

int main()
{
    char buffer[PACKET_LEN];
    memset(buffer, 0, PACKET_LEN);

    ipheader *ip = (ipheader *)buffer;
    udpheader *udp = (udpheader *)(buffer + sizeof(ipheader));

    // add data
    char *data = (char *)udp + sizeof(udpheader);
    int data_len = strlen(CLIENT_IP);
    strncpy(data, CLIENT_IP, data_len);

    // create udp header
    udp->udp_sport = htons(CLIENT_PORT);
    udp->udp_dport = htons(SERVER_PORT);
    udp->udp_ulen = htons(sizeof(udpheader) + data_len);
    udp->udp_sum = 0; // checksum not calculated for simplicity

    // create ip header
    ip->iph_ihl = 5;
    ip->iph_ver = 4;
    ip->iph_tos = 0;
    ip->iph_len = htons(sizeof(ipheader) + sizeof(udpheader) + data_len);
    ip->iph_ident = htons(54321);
    ip->iph_offset = 0;
    ip->iph_ttl = 255;
    ip->iph_protocol = IPPROTO_UDP;
    ip->iph_chksum = 0; // will be calculated by send_raw_ip_packet

    inet_pton(AF_INET, SPOOF_IP, &(ip->iph_sourceip));
    inet_pton(AF_INET, SERVER_IP, &(ip->iph_destip));

    // send packet
    send_raw_ip_packet(ip);

    return 0;
}
