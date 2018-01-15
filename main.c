#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct ThreadInfo {
	int threadid;
	int result;
} ThreadInfo;

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void printUsage(char *program) {
	fprintf(stdout, "%s -n <#threads> -t <#secs>\n", program);
	fprintf(stdout, "#threads: number of threads\n");
	fprintf(stdout, "#secs: number of seconds to run the program\n");
	return;
}

void cleanup_handler(void *arg) {
	ThreadInfo *tinfo = (ThreadInfo *)arg;	
	fprintf(stdout, "thread %d found %d primes\n", tinfo->threadid, tinfo->result);
}

void *runThread(void *arg) {
	int i = 0;
	int limit = 1000000;
	ThreadInfo *tinfo = (ThreadInfo *)arg;

	tinfo->threadid = *(int *)arg;
	tinfo->result = 0;

	pthread_cleanup_push(cleanup_handler, (void *)tinfo);

	fprintf(stdout, "t%d> Thread %d waiting for signal from main to start\n", tinfo->threadid, tinfo->threadid);
	pthread_mutex_lock(&lock);
	pthread_cond_wait(&cv, &lock);
	pthread_mutex_unlock(&lock);
	fprintf(stdout, "t%d> Thread %d started ...\n", tinfo->threadid, tinfo->threadid);
	i = (tinfo->threadid * 10000) % limit;

	while (i < limit) {
		int j = 0;
		int prime = 1;

		for (j = 2; j < i; j++) {
			if (i % j == 0) {
				prime = 0;
				break;
			}
		}

		if (prime == 1) {
			tinfo->result += 1;
		}

		if (tinfo->result % 100 == 0) {
			usleep(10000); // sleep for 10 ms
		}

		i += 1;
		if (i == limit) {
			/* restart the loop */
			i = 2;
		}
		/* create a cancellation point */
		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
	return NULL;
}

int main(int argc, char *argv[]) {
	int i = 0;
	int timeToSleep = 30; // default value of 30 secs
	time_t startTime, endTime;
	int numThreads = 10;
	pthread_t *threads = NULL;
	ThreadInfo *tinfo = NULL;
	int total = 0;
	void *ret;

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-n")) {
			numThreads = atoi(argv[i + 1]);
		}
		if (!strcmp(argv[i], "-t")) {
			timeToSleep = atoi(argv[i + 1]);
		}
	}

	fprintf(stdout, "main> Number of threads: %d\n", numThreads);
	threads = malloc(sizeof(pthread_t *) * numThreads);
	if (NULL == threads) {
		perror("malloc:");
		goto _end;
	}
	tinfo = malloc(sizeof(ThreadInfo) * numThreads);
	if (NULL == tinfo) {
		perror("malloc:");
		goto _end;
	}
	for (i = 0; i < numThreads; i++) {
		int rc = 0;

		tinfo[i].threadid = i;
		rc = pthread_create(&threads[i], NULL, runThread, &tinfo[i]);
		if (0 != rc) {
			perror("pthread_create");
			exit(-1);
		}
	}
	sleep(1);
	fprintf(stdout, "main> Starting worker threads\n");
	pthread_cond_broadcast(&cv);
	fprintf(stdout, "main> Main thread sleeping for %d secs\n", timeToSleep);
	startTime = time(NULL);
	sleep(timeToSleep);
	endTime = time(NULL);
	fprintf(stdout, "main> Main thread slept for %ld secs\n", (long)endTime - (long)startTime);

	fprintf(stdout, "main> Canceling threads...\n");
	for (i = 0; i < numThreads; i++) {
		pthread_cancel(threads[i]);
	}

	for (i = 0; i < numThreads; i++) {
		pthread_join(threads[i], &ret);
		if (PTHREAD_CANCELED == ret) {
			fprintf(stdout, "main> Canceled thread %d\n", tinfo[i].threadid);
		}
	}

	for (i = 0; i < numThreads; i++) {
		total += tinfo[i].result;
	}
	fprintf(stdout, "main> Total primes calculated: %d\n", total);

_end:
	if (tinfo) {
		free(tinfo);
		tinfo = NULL;
	}
	if (threads) {
		free(threads);
		threads = NULL;
	}

	return 0;
}

