#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/wait.h>

#include "scull.h"

#define CDEV_NAME "/dev/scull"


/* Quantum command line option */
static int g_quantum;
static int g_process;
static int g_thread;

static void usage(const char *cmd)
{
	printf("Usage: %s <command>\n"
	       "Commands:\n"
	       "  R          Reset quantum\n"
	       "  S <int>    Set quantum\n"
	       "  T <int>    Tell quantum\n"
	       "  G          Get quantum\n"
	       "  Q          Qeuery quantum\n"
	       "  X <int>    Exchange quantum\n"
	       "  H <int>    Shift quantum\n"
	       "  h          Print this message\n",
	       cmd);
}

static void print_info(struct task_info* info) {

	printf("state %ld, stack %p, cpu %d, prio %d, sprio %d, nprio %d, rtprio %d, pid %d, tgid %d, nv %ld, niv %ld\n", info->state, info->stack, info->cpu, info->prio, info->static_prio, info->normal_prio, info->rt_priority, info->pid, info->tgid, info->nvcsw, info->nivcsw);

}

typedef int cmd_t;

static cmd_t parse_arguments(int argc, const char **argv)
{
	cmd_t cmd;

	if (argc < 2) {
		fprintf(stderr, "%s: Invalid number of arguments\n", argv[0]);
		cmd = -1;
		goto ret;
	}

	/* Parse command and optional int argument */
	cmd = argv[1][0];
	switch (cmd) {
	case 'S':
	case 'T':
	case 'H':
	case 'X':
		if (argc < 3) {
			fprintf(stderr, "%s: Missing quantum\n", argv[0]);
			cmd = -1;
			break;
		}
		g_quantum = atoi(argv[2]);
		break;
	case 'R':
	case 'G':
	case 'Q':
	case 'h':
		break;
	case 'p':
		if (argc < 3) {
			fprintf(stderr, "%s: Missing process number\n", argv[0]);
			cmd = -1;
			break;
		}
		g_process = atoi(argv[2]);
		break;
	case 't':
		if (argc < 3) {
			fprintf(stderr, "%s: Missing thread number\n", argv[0]);
			cmd = -1;
			break;
		}
		g_thread = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "%s: Invalid command\n", argv[0]);
		cmd = -1;
	}

ret:
	if (cmd < 0 || cmd == 'h') {
		usage(argv[0]);
		exit((cmd == 'h')? EXIT_SUCCESS : EXIT_FAILURE);
	}
	return cmd;
}

void* thread_op(void* arg) {
	struct task_info info;
	int* fd = (int*)arg;
	ioctl(*fd, SCULL_IOCVQUANTUM, &info);
	print_info(&info);
	return NULL;

}


static int do_op(int fd, cmd_t cmd)
{
	int ret, q;
	pid_t pid;
	int n = 0;
	struct task_info info;
	pthread_t ts[11];

	switch (cmd) {
	case 'R':
		ret = ioctl(fd, SCULL_IOCRESET);
		if (ret == 0)
			printf("Quantum reset\n");
		break;
	case 'Q':
		q = ioctl(fd, SCULL_IOCQQUANTUM);
		printf("Quantum: %d\n", q);
		ret = 0;
		break;
	case 'G':
		ret = ioctl(fd, SCULL_IOCGQUANTUM, &q);
		if (ret == 0)
			printf("Quantum: %d\n", q);
		break;
	case 'T':
		ret = ioctl(fd, SCULL_IOCTQUANTUM, g_quantum);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'S':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCSQUANTUM, &q);
		if (ret == 0)
			printf("Quantum set\n");
		break;
	case 'X':
		q = g_quantum;
		ret = ioctl(fd, SCULL_IOCXQUANTUM, &q);
		if (ret == 0)
			printf("Quantum exchanged, old quantum: %d\n", q);
		break;
	case 'H':
		q = ioctl(fd, SCULL_IOCHQUANTUM, g_quantum);
		printf("Quantum shifted, old quantum: %d\n", q);
		ret = 0;
		break;
	case 'p': 
		n = 0;
		while (n < g_process) {
			pid = fork();
			if (pid == 0) {
				break;	
			}
			n++;
		
		}
		if (pid == 0) {
			ioctl(fd, SCULL_IOCVQUANTUM, &info);
			print_info(&info);
			exit(0);
		} else {
			wait(NULL);	
		}
		ret = 0;
		break;
	case 't':
		n = 0;
		while (n < g_thread) {
			pthread_create(&(ts[n]), NULL, thread_op, &fd);
			n++;
		
		}
		for(n = 0; n <g_thread; n++) {
			pthread_join(ts[n], NULL);	
		}

		ret = 0;
		break;

		
	default:
		/* Should never occur */
		abort();
		ret = -1; /* Keep the compiler happy */
	}

	if (ret != 0)
		perror("ioctl");
	return ret;
}

int main(int argc, const char **argv)
{
	int fd, ret;
	cmd_t cmd;

	cmd = parse_arguments(argc, argv);

	fd = open(CDEV_NAME, O_RDONLY);
	if (fd < 0) {
		perror("cdev open");
		return EXIT_FAILURE;
	}

	printf("Device (%s) opened\n", CDEV_NAME);

	ret = do_op(fd, cmd);

	if (close(fd) != 0) {
		perror("cdev close");
		return EXIT_FAILURE;
	}

	printf("Device (%s) closed\n", CDEV_NAME);

	return (ret != 0)? EXIT_FAILURE : EXIT_SUCCESS;
}
