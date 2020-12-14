#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <toml.h>
#include <getopt.h>

#define VERSION "1.0.0"

#define DEFAULT_PORT	(-1)
#define DEFAULT_MODE	(-1)
#define MAX_SUPPORT_PORTS (2)

#define  sysfs_gpio	"/sys/class/gpio"
#define  gpio_export	"export"
#define  MAX_PORTS	(6)	

const char pathConf[] = "/etc/srg52/conf.d";
const char fileConf[] = "uartmode.toml";

#define gpioNum(controller, index) \
	((controller*32)+index)

/* ------------------------------------
 * /dev/ttyMU0, gpio2_6, gpio2_7
 * /dev/ttyMU1, gpio2_10, gpio2_11
 * ====================================
 *  MODE    	gpio2_6,	gpio2_7
 * 		gpio2_10,	gpio2_11
 *  RS232	   1		   0
 *  RS485-HALF	   0		   1
 *  RS485-FULL	   1		   1
 * ------------------------------------
 */

enum SERIAL_MODE {
	MODE_RS232 = 0,
	MODE_RS485_HALF = 1,
	MODE_RS485_FULL = 2,
	MODE_MAX_SUPPORT = MODE_RS485_FULL,
};

struct CONF_UART {
	long portnums;
	enum SERIAL_MODE mode[MAX_PORTS];
};

enum OPTION_ACTION {
	USAGE_HELP = -1,
	GET_PORT_STATUS = 0,
	SET_PORT_STATUS,
	INIT_PORT_STATUS
};

/*
 * Declaring Variables
 */
uint32_t optflags = USAGE_HELP;
static int argPort = DEFAULT_PORT;
static int argMode = DEFAULT_MODE;
static struct CONF_UART conf = { .portnums = 0, .mode = { 0 } };


static int portGpios[MAX_SUPPORT_PORTS][2] = { 
	{ gpioNum(2, 6), gpioNum(2, 7)},
	{ gpioNum(2, 10), gpioNum(2, 11)},
};

static int portMode[MODE_MAX_SUPPORT+1][2] = {
	{1, 0},
	{0, 1},
	{1, 1},
};

static int isHelp(void) {
	return (optflags == USAGE_HELP);
}

static int writeFile(char* file, char* data, size_t len)
{
	int fd = open(file, O_WRONLY);
	int ret = 0;
	if (fd < 0) {
		perror("open file failed\n");
		return fd;
	}

	ret = write(fd, data, len);
	if (ret < 0) {
		perror("write file failed\n");
		return ret;
	}
	close(fd);
	return 0;
}

static void export_gpio(int i)
{
	char path[PATH_MAX] = { 0 };
	char data[32] = { 0 };

	if (i < 0)
		return;

	snprintf(path, sizeof(path), "%s/export", sysfs_gpio);
	snprintf(data, sizeof(data), "%d", i);
	writeFile(path, data, strlen(data));
}

static void setgpio_output_mode(int i)
{
	char path[PATH_MAX] = { 0 };
	char data[] = "out";
	snprintf(path, sizeof(path), "%s/gpio%d/direction", sysfs_gpio, i);
	writeFile(path, data, strlen(data));
}

static void setgpio_output_model_initvalue(int i, int v)
{
	char path[PATH_MAX] = { 0 };
	char data[16] = { 0 };

	strncpy(data, "low", sizeof(data));
	if (v > 0)
		strncpy(data, "high", sizeof(data));

	snprintf(path, sizeof(path), "%s/gpio%d/direction", sysfs_gpio, i);
	writeFile(path, data, strlen(data));
}

static void setgpio_output_value(int i, int value)
{
	char path[PATH_MAX] = { 0 };
	char data[] = "out";
	snprintf(path, sizeof(path), "%s/gpio%d/value", sysfs_gpio, i);
	snprintf(data, sizeof(data), "%d", value);
	writeFile(path, data, strlen(data));
}

static int isgpio_exist(int i)
{
	int ret;
	char path[PATH_MAX] = { 0 };
	snprintf(path, sizeof(path), "%s/gpio%d", sysfs_gpio, i);
	return (0 == access(path, F_OK)) ? 1 : 0;
}

/* for update uart mode */
static int updateMode(int port, int mode) {
	char cmd[256] = { 0 };
	snprintf(cmd, sizeof(cmd) - 1,
		"sed -i 's/port%dMode.*/port%dMode=%d/g' %s/%s",
			port, port, mode,
			pathConf, fileConf);
	return system(cmd);
}

static void setUartMode(int port, int mode) {
	if (port >= 0 && port <= 1) {
		int bExport = 0;
		if (0 == isgpio_exist(portGpios[port][0])) {
			export_gpio(portGpios[port][0]);
			bExport = 1;
		}

		if (0 == isgpio_exist(portGpios[port][1])) {
			export_gpio(portGpios[port][1]);
			bExport = 1;
		}

		if (bExport = 1)
			usleep(1000 * 300);

		if (mode >= 0 && mode <= MODE_MAX_SUPPORT) {
			setgpio_output_model_initvalue(portGpios[port][0], portMode[mode][0]);
			setgpio_output_model_initvalue(portGpios[port][1], portMode[mode][1]);
		}
	}
}

static int IsRoot(void)
{
	uid_t uid=getuid(), euid=geteuid();
	return (uid==0 && uid==euid) ? 0 : -1;
}

static int procUARTTable(const TomlTable *table) 
{
	TomlTable *uart_table = NULL;

	uart_table = toml_table_get_as_table(table, "UART");
	if (uart_table == NULL)
		return -2; /* can't find UART section */

	conf.portnums = toml_table_get_as_integer(uart_table, "portnums");
	return 0;
}

static int procModeTable(const TomlTable *table)
{
	char keybuff[32];
	TomlTable *mode_table = NULL;

	mode_table = toml_table_get_as_table(table, "MODE");
	if (mode_table == NULL)
		return -3; /* can't find Mode section */

	for (int i = 0; i < MAX_PORTS && i < conf.portnums; i++) {
		snprintf(keybuff, sizeof(keybuff) - 1, "port%dMode", i);
		conf.mode[i] = toml_table_get_as_integer(mode_table, keybuff);
	}
	return 0;
}

static int loadconf(void) {
	const char filename[256] = { 0 };
	int ret = 0;
	TomlTable *table = NULL;	
	snprintf((char *)filename, sizeof(filename), "%s/%s", pathConf, fileConf);

	table =toml_load_filename(filename);
	if (table == NULL)
		return -1; /* load conf file failed */

	ret = procUARTTable(table);
	if (ret == 0)
		ret = procModeTable(table);

	toml_table_free(table);
	return ret;
}


static void sTitle(void) {
	fprintf(stderr, "SRG-3352x uartmode tool, " VERSION "\n");
}

static void usage(void) {
	sTitle();
	fprintf(stderr, "\n  options:\n");
	fprintf(stderr, "\t-p, --port=[value] -- uart index, supprt 0 and 1\n");
	fprintf(stderr, "\t-m, --mode=[value] -- uart mode, 0 for RS232\n");
	fprintf(stderr, "\t                                 1 for RS485(half duplex)\n");
	fprintf(stderr, "\t                                 2 for RS485(full duplex)/RS422\n");
	fprintf(stderr, "\t-h, --help         -- show this message\n");
	fprintf(stderr, "\n");
}

static const char *short_options = "ihp:m:";
static const struct option long_options[] = {
	{"init", no_argument, NULL, 'i'},
	{"help", no_argument, NULL, 'h'},
	{"port", required_argument, NULL, 'p'},
	{"mode", required_argument, NULL, 'm'},
};

static int preprocess_cmd_option(int argc, char *argv[]) {
	int result = 0;
	int optCount = 0;

	while (1) {
		int c = -1;
		int opt_idx = 0;
		c = getopt_long(argc, argv, short_options, long_options, &opt_idx);
		optCount++;
		if (c == -1) {
			break;
		}
		switch(c) {
		case 'i':
			optflags = INIT_PORT_STATUS;
			break;
		case 'h':
			optflags = USAGE_HELP;
			break;
		case 'p':
			argPort = atoi(optarg);
			if ((argPort == 0) && (*optarg != '0')) {
				argPort = DEFAULT_PORT;
				break;
			}

			if(optflags != SET_PORT_STATUS)
				optflags = GET_PORT_STATUS;
			break;
		case 'm':
			argMode = atoi(optarg);
			if ((argMode == 0) && (*optarg != '0')) {
				argMode = DEFAULT_MODE;
				break;
			}

			optflags = SET_PORT_STATUS;
			break;
		}
	}

	if (optflags == SET_PORT_STATUS)  {
		if (argPort == DEFAULT_PORT) {
			fprintf(stderr, "%s: missed/incorrect argument. '-p/--port'\n", argv[0]);
			optflags = USAGE_HELP;
			return -1;
		}

		if (argMode == DEFAULT_MODE) {
			fprintf(stderr, "%s: missed/incorrect argument. '-m/--mode'\n", argv[0]);
			optflags = USAGE_HELP;
			return -2;
		}

		if ((argPort > (conf.portnums - 1)) || (argPort >= MAX_PORTS) ||
			(argMode > MODE_MAX_SUPPORT)) {
			fprintf(stderr, "%s: incorrect argument. over range\n", argv[0]);
			optflags = USAGE_HELP;
			return -3;
		}
		
		if (IsRoot() < 0) {
			fprintf(stderr, "%s: change uart mode need privilege\n", argv[0]);
			return -4;
		}
	}

	if (optflags == GET_PORT_STATUS) {
		if ((argPort > (conf.portnums - 1)) || (argPort >= MAX_PORTS)) {
			fprintf(stderr, "%s: over support range: 'port'\n", argv[0]);
			optflags = USAGE_HELP;
			return -5;
		}
	}
	
	return 0;
}

static int procGetPortStatus(void) {
	if (optflags == GET_PORT_STATUS) {
		fprintf(stdout, "%d\n", conf.mode[argPort]);
		return 1;
	}
	return 0;
}

static int procSetPortStatus(void) {
	if (optflags == SET_PORT_STATUS) {
		updateMode(argPort, argMode);
		conf.mode[argPort] = argMode;
		setUartMode(argPort, argMode);
		return 1;
	}
	return 0;
}

static int procInitPortsStatus(void) {
	if (optflags == INIT_PORT_STATUS) {
		for (int i = 0; i < conf.portnums; i++)
			setUartMode(i, conf.mode[i]);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int cmd_opt = 0;

	loadconf();

	cmd_opt = preprocess_cmd_option(argc, argv);
	if (cmd_opt != 0 || isHelp()) {
		usage();
		exit(0);
	}
	
	if (procGetPortStatus())
		return 0;
	if (procSetPortStatus())
		return 0;
	if (procInitPortsStatus())
		return 0;

	return 0;
}
