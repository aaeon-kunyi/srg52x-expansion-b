#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

static void usage(char* argv[]) {
	fprintf(stderr, "%s SRG-3352x can bus -- read test\n", argv[0]);
	fprintf(stderr, "\n  usage:\n");
	fprintf(stderr, "\t %s interface -- interface name\n", argv[0]);
	fprintf(stderr, "\t for example: %s can0\n", argv[0]);
}

int main(int argc, char* argv[]) {
	int s;
	int nbytes;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;
	
	if (argc < 2) {
		fprintf(stderr, "%s: missed canbus interface\n", argv[0]);
		usage(argv);
		return -1;
	}
	
	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -2;
	}

	strcpy(ifr.ifr_name, argv[1]);
	if ( 0 > ioctl(s, SIOCGIFINDEX, &ifr))
        return -3;

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	printf("%s at index %d\n", argv[1], ifr.ifr_ifindex);
	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -4;
	}

	nbytes = read(s, &frame, sizeof(struct can_frame));
	if (nbytes < 0) {
		perror("Error in can raw socket read");    
	}
	
	if (nbytes < sizeof(struct can_frame)) {
		fprintf(stderr, "read: incomplete CAN frame\n");
		return -5;
	}

	printf(" %5s %03x [%d] ", argv[1], frame.can_id, frame.can_dlc);
	for (int i = 0; i < frame.can_dlc; i++)
		printf(" %02x", frame.data[i]);
	printf("\n");
 
	return 0;
}
