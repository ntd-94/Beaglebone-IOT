//NAME: TEJASVI KASTURI
//UID: 604778994
//EMAIL: KASTURITEJASVI@GMAIL.COM

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mraa/aio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <poll.h>
#include <errno.h>
#include <ctype.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#define SHUTDOWN 1
#define STOPPED 1
#define BUFFERSIZE 256
#define F 0
#define C 1

/** globals **/
int scale = F;
int period = 1;
FILE *logfile = NULL;
int logOn = 0;
int report = 1;
int is_shutdown = !SHUTDOWN;
int is_stop = !STOPPED;
char* id;
int port;
char *hostname;
int sockfd;
SSL* SSLstruct;
SSL_CTX* SSLcontext;

mraa_aio_context tempSensor;
//mraa_gpio_context button;


void timehandler(char* timereport)
{
	time_t rawtime;
	time(&rawtime);
	if (rawtime == (time_t)-1)
	{
		fprintf(stderr, "time error%s\n", strerror(errno));
	}

	struct tm* local_time = localtime(&rawtime);
	if (local_time == NULL)
	{
		fprintf(stderr, "time local error %s\n", strerror(errno));
	}

	snprintf(timereport, 4, "%d:", local_time->tm_hour);
	if (local_time->tm_hour < 10) {
		timereport[0] = '0';
		snprintf(timereport+1, 3, "%d:", local_time->tm_hour);
	}

	snprintf(timereport+3, 4, "%d:", local_time->tm_min);
	if (local_time->tm_min < 10) {
		timereport[3] = '0';
		snprintf(timereport+4, 3, "%d:", local_time->tm_min);
	}

	snprintf(timereport+6, 3, "%d", local_time->tm_sec);
	if (local_time->tm_sec < 10) {
		timereport[6] = '0';
		snprintf(timereport+7, 2, "%d", local_time->tm_sec);
	}

}

void launch_shutdown_routine()
{
	char shutdown_msg[20];
	timehandler(shutdown_msg);
	//dprintf(sockfd, "SHUTDOWN\n");
	shutdown_msg[8] = ' ';
	shutdown_msg[9] = 'S';
	shutdown_msg[10] = 'H';
	shutdown_msg[11] = 'U';
	shutdown_msg[12] = 'T';
	shutdown_msg[13] = 'D';
	shutdown_msg[14] = 'O';
	shutdown_msg[15] = 'W';
	shutdown_msg[16] = 'N';
	shutdown_msg[17] = '\n';
	shutdown_msg[18] = '\n';
	shutdown_msg[19] = '\0';

	

	//write(sockfd, shutdown_msg, 18);
	//write(sockfd, nl, 2);

	SSL_write(SSLstruct, shutdown_msg, 19);
	//SSL_write(SSLstruct, nl, 2);

	if(logOn)
	{
		fprintf(logfile, "%s", shutdown_msg);
		fflush(logfile);
	}
	mraa_aio_close(tempSensor);
	//mraa_gpio_close(button);
	exit(0);

}

float read_tempsensor()
{
	float reading = mraa_aio_read(tempSensor);
	/*
	int reading = mraa_aio_read(temp_sensor);
	int B = 4275;     //thermistor value
 	float R0 = 100000.0;      //nominal base value
	  float R = 1023.0/((float) reading) - 1.0;
  		R = R0*R;
	*/
	int B = 4275;
	float R0 = 100000.0;
	float R = 1023.0/reading - 1.0;
	R = R0*R;

	float temp = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	if(scale == C)
	{
		return temp;
	}
	else
	{
		return temp * 9/5 + 32;
	}
}
void sample_time()
{
	float temp = read_tempsensor();
	char tempreport[15];
		timehandler(tempreport);
		//fprintf(stdout, "%.1f\n", temp);
		tempreport[8] = ' ';
		snprintf(tempreport+9, 5, "%0.1f", temp);
		tempreport[13] = '\n';
		tempreport[14] = '\0';
		//dprintf(sockfd, "%s\n", tempreport);
		SSL_write(SSLstruct, tempreport, 14);
		if(logOn)
		{
			fprintf(logfile, "%s", tempreport);
			fflush(logfile);
		}
	

	

}

void processCommands(char* buffer)
{
	if(strstr(buffer, "SCALE") != NULL)
	{
		if(strstr(buffer, "F") != NULL)
		{
			scale = F;
			if(logOn)
				fprintf(logfile, "SCALE=F\n");
		}
		else
		{
			scale=C;
			if(logOn)
				fprintf(logfile, "SCALE=C\n");

		}
	}
	if(strstr(buffer, "PERIOD") != NULL)
	{
		int f = 0;
		while(buffer[f] != 'D')
			f++;
		f+=2;
		char* temp = malloc(5*sizeof(char));
		int j = 0;
		while(isdigit(buffer[f]))
		{
			temp[j] = buffer[f];
			f++;
			j++;
		}
		period = atoi(temp);
		if(logOn)
		{
			fprintf(logfile, "PERIOD=%i\n", period);
		}
	}
	if(strstr(buffer, "START") != NULL)
	{
		is_stop = !STOPPED;
		if(logOn)
		{
			fprintf(logfile, "START\n");
		}
	}
	if(strstr(buffer, "STOP") != NULL)
	{
		is_stop = STOPPED;
		if(logOn)
		{
			fprintf(logfile, "STOP\n");
		}
	}
	if(strstr(buffer, "LOG") != NULL)
	{
		int f = 0;
		while(buffer[f] != 'G')
			f++;
		f += 2;
		char *msg = malloc(256*sizeof(char));
		int j = 0;
		while(buffer[f] != '\n')
		{
			msg[j] = buffer[f];
			j++;
			f++;
		}
		if(logOn)
		{
			fprintf(logfile, "LOG %.*s\n", j, msg);
		}
	}
	if(strstr(buffer, "OFF") != NULL)
	{
		if(logOn)
		{
			fprintf(logfile, "OFF\n");
		}
		launch_shutdown_routine();
	}

}

int main(int argc, char** argv)
{

  if (argc < 2)
  {
    fprintf(stderr, "enter a port number! Usage: ./lab4c_tcp portnum\n");
    exit(1);

  }
	struct option longopts[] = {
    	{"period", required_argument,  NULL,   'p'},
    	{"scale",  required_argument,  NULL,   's'},
    	{"log",    required_argument,  NULL,   'l'},
    	{"id", required_argument, NULL, 'i'},
    	{"host", required_argument, NULL, 'h'},
    	{0, 0, 0, 0}
  	};

  	int c;
  	char* filename;

  	while ((c = getopt_long(argc, argv, "p:s:l:i:h", longopts, NULL)) != -1) {
		 switch (c) {
      		  case 'p':
          		period = atoi(optarg);
              break;
          	case 's':
            	if (strcmp("C", optarg) == 0)
	              {
	                scale = C;
	              }
            	break;
          	case 'l':
      	      logOn = 1;
      	      filename = optarg;
              break;
            case 'i':
            	if(strlen(optarg) != 9) 
            	{
		          fprintf(stderr, "Enter a nine digit number!");
		          exit(1);
		        }
		        else
		        {
		        	id = optarg;
		        }
		        break;
		    case 'h':
		    	hostname = optarg;
		    	break;
          	case '?':
              fprintf(stderr, "Wrong Usage. Correct Usage: lab4b [--period=#] [--scale=[C/F]] [--log=FILE]\n");
              exit(1);
    	}
  	}

  	port = atoi(argv[optind]);
  	if(port == 0) {
	    fprintf(stderr, "port num error\n");
	    exit(1);
  	}

  	if(logOn)
  	{
  		logfile = fopen(filename, "a+");
  		if(logfile == NULL)
  		{
  			fprintf(stderr, "fopen error: %s\n", strerror(errno));
  		}
  	}

  	tempSensor = mraa_aio_init(1);
  	if(!tempSensor)
  	{
  		fprintf(stderr, "AIO init error. %s\n", strerror(errno));
  		exit(2);
  	}
  	//button = mraa_gpio_init(60);
  	/*if(!button)
  	{
  		fprintf(stderr, "GPIO init error. %s\n", strerror(errno));
  	}*/
  	//mraa_gpio_dir(button, MRAA_GPIO_IN);
  	//mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &sighandler, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	if(sockfd < 0)
  	{
  		fprintf(stderr, "Socket Error: %s\n", strerror(errno));
  		exit(2);
  	}
  	struct hostent *server = gethostbyname(hostname);
	if (server == NULL)
	{
		fprintf(stderr, "host name error: %s\n", strerror(errno));
		exit(2);
	}

	struct sockaddr_in serv_addr;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)&serv_addr.sin_addr.s_addr,
		   (char *)server->h_addr,
		   server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "error connecting to server: %s\n", strerror(errno));
		exit(2);
	}

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	const SSL_METHOD* ssl_connection = SSLv23_client_method();

	SSLcontext = SSL_CTX_new(ssl_connection);

	if (SSLcontext == NULL)
	{
		fprintf(stderr, "SSL_CTX_new error: %s\n", strerror(errno));
		exit(2);
	}

	SSLstruct = SSL_new(SSLcontext);
	if (SSLstruct == NULL)
	{
		fprintf(stderr, "SSL new error: %s\n", strerror(errno));
		exit(2);
	}

	if (SSL_set_fd(SSLstruct, sockfd) == 0)
	{
		fprintf(stderr,"SSL set error: %s\n", strerror(errno));
		exit(2);
	}

	if (SSL_connect(SSLstruct) < 1)
	{
		fprintf(stderr, "SSL connect error: %s\n", strerror(errno));
		exit(2);
	}


	//dprintf(sockfd, "ID=%d\n", id);
	if(logOn)
	{
		fprintf(logfile, "ID=%s\n", id);
		fflush(logfile);
	}

	char idreport[14];
	idreport[0] = 'I';
	idreport[1] = 'D';
	idreport[2] = '=';
	strcpy(idreport+3, id);
	idreport[12] = '\n';
	idreport[13] = '\0';

	SSL_write(SSLstruct, idreport, 13);



  	struct pollfd pfds[1];
  	pfds[0].fd = sockfd;
  	pfds[0].events = POLLIN;
  	pfds[0].revents = 0;

  	//time_t ctime = time(NULL);
  	//time_t ptime = 0;
  	while(1)
  	{
  		int ret = poll(pfds, 1, 0);
  		if(ret < 0)
  		{
  			fprintf(stderr, "poll error: %s\n", strerror(errno));
  		}

  		//ctime = time(NULL);
  		if(is_stop != STOPPED)
  		{
  			sample_time();
  			//ptime = ctime;
  		}

  		if(pfds[0].revents & POLLIN)
  		{
  			char buffer[BUFFERSIZE];
  			memset(buffer, 0, BUFFERSIZE);
  			int ret = SSL_read(SSLstruct, &buffer, BUFFERSIZE);
  			if(ret == 0)
  			{
  				launch_shutdown_routine();
  				exit(0);
  			}
  			if(ret < 0)
  			{
  				fprintf(stderr, "read error: %s\n", strerror(errno));
  			}
  			processCommands(buffer);
  		}

  		sleep(period);
  	}




  	return 0;


}
