
#include "common.h"








void GetDataFromMessage(char *msg, char *data)
{
    int i;
    int j = 0;
    
    i = MSGTYPE_LEN + CHUNK_LEN;
    j = 0;
    
    while(msg[i] != '\0') data[j++] = msg[i++];
    
    data[j] = '\0';
}





/* returns number of chunks if not ERROR */
int ProcessCTLMessage(char *msg)
{
    char ctltype[CTLTYPE_LEN + 1];
    int i;
    int j = 0;
    char schunks[CHUNK_LEN + 1];
    
    for(i = MSGTYPE_LEN; i < CTLTYPE_LEN + 1; i++)
    {
        ctltype[j++] = msg[i];
    }
    ctltype[--i] = '\0';

    i = MSGTYPE_LEN + CTLTYPE_LEN;
    j = 0;
    
    if(strcmp(ctltype, "ERROR") == 0)
    {
        while(i < strlen(msg)) fprintf(stderr, "%c", msg[i++]);
        fprintf(stderr, "\n");
        exit(1);
    }
    
    if(strcmp(ctltype, "CHNKS") == 0)
    {
        while(i < strlen(msg)) schunks[j++] = msg[i++];
        schunks[j] = '\0'; 
        return atoi(schunks);
    }
    
    /* should not reach here */
    fprintf(stderr, "Error Processing CTL Message\n");
    
    exit(1);
}





void
GetMyIP(char *ip)
{
    struct hostent *he;
    struct sockaddr_in sin;
    char machine_name[128];
    
    if (gethostname (machine_name, sizeof (machine_name)))
	{
        perror ("gethostname");
        exit(1);
	}
    
    do
	{
		he = gethostbyname (machine_name);
	}
    while(he == NULL);
    
    bzero (&sin, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (80);
	bcopy (he->h_addr_list[ 0 ], &sin.sin_addr, he->h_length);
    
    strcpy(ip, (char *)inet_ntoa(sin.sin_addr));
}






void
CreateRequestMessage(char *filename, int chunkno, char *msg)
{
    char temp[128];
    int i;
    char myip[CADDRESS_LEN + 1];
	
    /* request type */
    strcpy(msg, "R");
   
    /* filename */
    if(strlen(filename) < FILENAME_LEN)
    {
        for(i = 0; i < FILENAME_LEN - strlen(filename); i++)
        {
            strcat(msg, " ");
        }
    }
    strcat(msg, filename);
     
    /* chunk number */
    sprintf(temp, "%d", chunkno);
    if(strlen(temp) < CHUNK_LEN)
    {
        for(i = 0; i < CHUNK_LEN - strlen(temp); i++)
        {
            strcat(msg, " ");
        }
    }
    strcat(msg, temp);
    
	/* ip address */
	GetMyIP(myip);
    if(strlen(myip) < CADDRESS_LEN)
    {
		for(i = 0; i < CADDRESS_LEN - strlen(myip); i++)
		{
			strcat(msg, " ");
		}
    }
    strcat(msg, myip);

    /* port */
    sprintf(temp, "%d", htons(CLIENT_RECV_PORT));
    if(strlen(temp) < CPORT_LEN)
    {
        for(i = 0; i < CPORT_LEN - strlen(temp); i++)
        {
            strcat(msg, " ");
        }
    }
    strcat(msg, temp);
    
    msg[strlen(msg)] = '\0';
}





/* ************************************************************************** */

int main(int argc, char *argv[])
{
    /* socket related */
    int send_sock;
    int recv_sock;
    struct sockaddr_in send_name;
    struct sockaddr_in recv_name;
    struct hostent *hp, *gethostbyname();
    int length;
    struct timeval tv;
    
    /* messages */
    char recvd_msg[DMSGSIZE];
    char req_msg[RMSGSIZE];
    long num_chunks;
	char data[DATA_LEN + 1];
    
	/* time related */
	long long start_time, end_time;
	
    int i, j;
    int num_tries = 0;
    
    char *temp;
    
	
    /* confirm correct usage */
    if(argc != 4)
    {
        fprintf(stderr, "Usage :\n\t%s <remote address> <port> <file>\n", argv[0]);
        exit(1);
    }


    /* create socket to send on */
    send_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(send_sock < 0)
    {
        perror("Opening socket");
        exit(1);
    }
    hp = gethostbyname(argv[1]);
    if(hp == 0)
    {
        fprintf(stderr, "Unknown host : %s\n", argv[1]);
        exit(1);
    }
    bcopy(hp->h_addr, &send_name.sin_addr, hp->h_length);
    send_name.sin_family = AF_INET;
    send_name.sin_port = htons(atoi(argv[2]));
    
	
    /* create a socket to receive on */
    recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(recv_sock < 0)
    {
        perror("Opening receiving socket");
        exit(1);
    }
	tv.tv_sec = SOCK_TIMEOUT;
    tv.tv_usec = 0;
	setsockopt(recv_sock,SOL_SOCKET,  SO_RCVTIMEO,(struct timeval*)&tv,sizeof(struct timeval));
    recv_name.sin_family = AF_INET;
    recv_name.sin_addr.s_addr = INADDR_ANY;
    recv_name.sin_port = CLIENT_RECV_PORT;
    if(bind(recv_sock, (struct sockaddr *)&recv_name, sizeof(recv_name)))
    {
        perror("Binding receiving socket");
        exit(1);
    }
    length = sizeof(recv_name);
    if(getsockname(recv_sock, (struct sockaddr *)&recv_name, &length))
    {
        perror("Getting receiving socket name");
        exit(1);
    }
	
	
	/* record time to calculate execution time */
	start_time = time(NULL);
    
	
    /* create request msg for number of chunks and send */
    CreateRequestMessage(argv[3], 0, req_msg);
    if(sendto(send_sock, req_msg, strlen(req_msg) + 1, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0) perror("Sending message");
    
    /* get number of chunks from server */
    do
    {
        if(read(recv_sock, recvd_msg, DMSGSIZE) < 0) perror("Reading from socket");
        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) || (strlen(recvd_msg) == 0) ) fprintf(stderr, "Request timed out.\n");
        if(num_tries++ == MAX_TRIES - 1)
        {
            fprintf(stderr, "MAX_TRIES reached. No response.\n");
            exit(1);
        }
        if( (strlen(recvd_msg) != 0) && (recvd_msg[0] != 'C') )
        {
            recvd_msg[0] = '\0';
            fprintf(stderr, "Bad CTLMSG received, re-requesting. . .\n");
            if(sendto(send_sock, req_msg, strlen(req_msg) + 1, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0) perror("Sending message");
        }
    }
    while( strlen(recvd_msg) == 0 );
    if(recvd_msg[0] == 'C') num_chunks = ProcessCTLMessage(recvd_msg);

    
    /* start requesting chunks */
    for(i = 1; i <= num_chunks; i++)
    {
        num_tries = 0;
        for(j = 0; j < DMSGSIZE; j++) recvd_msg[j] = '\0';
        
        CreateRequestMessage(argv[3], i, req_msg);

        if(sendto(send_sock, req_msg, strlen(req_msg) + 1, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0) perror("Sending message");
        do
        {
            if(read(recv_sock, recvd_msg, DMSGSIZE) < 0) perror("Reading from socket");
            if( (errno == EAGAIN) || (errno == EWOULDBLOCK) || (strlen(recvd_msg) == 0) ) fprintf(stderr, "Request timed out.\n");
            if(num_tries++ == MAX_TRIES - 1)
            {
                fprintf(stderr, "MAX_TRIES reached. No response.\n");
                exit(1);
            }
            /* if bad message received */
            if( (strlen(recvd_msg) != 0) && (recvd_msg[0] != 'D') )
            {
                recvd_msg[0] = '\0'; /* set message to null */
                fprintf(stderr, "Bad DATMSG received, re-requesting. . .\n");
                if(sendto(send_sock, req_msg, strlen(req_msg) + 1, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0) perror("Sending message");
            }
        }
        while(strlen(recvd_msg) == 0);

		/* extract and display received data */
		GetDataFromMessage(recvd_msg, data);
		printf("%s", data);
    }
    
    
    /* close send and receive sockets */
    close(send_sock);
    close(recv_sock);
    
	
	/* calculate and display execution time */
	end_time = time(NULL);
	fprintf(stderr, "%s received (%lld seconds)\n", argv[3], end_time - start_time);
	

    return 0;
}
    
/* ************************************************************************** */    


    
    


