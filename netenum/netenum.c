/*
  Example code to obtain IP and MAC for all available interfaces on Linux.
  by Adam Pierce <adam@doctort.org>

http://www.doctort.org/adam/

*/

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>

int main(void)
{
	char          buf[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int           sck;
	int           nInterfaces;
	int           i;

/* Get a socket handle. */
	sck = socket(AF_INET, SOCK_DGRAM, 0);
	if(sck < 0)
	{
		perror("socket");
		return 1;
	}

/* Query available interfaces. */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl(SIOCGIFCONF)");
		return 1;
	}

/* Iterate through the list of interfaces. */
	ifr         = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)
	{
		struct ifreq *item = &ifr[i];

	/* Show the device name and IP address */
		printf("%s:\n", item->ifr_name);

		printf("\tIP %s\n",
		       inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));

	/* Get the MAC address */
		if(ioctl(sck, SIOCGIFHWADDR, item) < 0)
		{
			perror("ioctl(SIOCGIFHWADDR)");
			return 1;
		}

		printf("\tMAC %s\n",
		       inet_ntoa(((struct sockaddr_in *)&item->ifr_hwaddr)->sin_addr));

	/* Get the broadcast address (added by Eric) */
		if(ioctl(sck, SIOCGIFBRDADDR, item) >= 0)
			printf("\tBROADCAST %s\n", 
				inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr));

		printf("\n\n");
	}

        return 0;
}
