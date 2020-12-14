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
	fprintf(stderr, "%s SRG-3352x can bus -- write test\n", argv[0]);
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
	if ( 0 > ioctl(s, SIOCGIFINDEX, &ifr)) {
		fprintf(stderr, "%s: check '%s' inteface is correct\n", argv[0], argv[1]);
        return -3;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	printf("%s at index %d\n", argv[1], ifr.ifr_ifindex);
	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -4;
	}

	frame.can_id = 0x123;
	frame.can_dlc = 8;
	frame.data[0] = 0x11;
	frame.data[1] = 0x22;
	frame.data[2] = 0x33;
	frame.data[3] = 0x44;
	frame.data[4] = 0x55;
	frame.data[5] = 0xAA;
	frame.data[6] = 0xBB;
	frame.data[7] = 0xCC;

	nbytes = write(s, &frame, sizeof(struct can_frame));
	printf("Wrote %d bytes\n", nbytes);

	return 0;
}
