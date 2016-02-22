/* server receives request messages and sends control/data messages */

#include "common.h"




void
GetClientAddress(char *buf, char *address)
{
    int i;
    int begin;
    char ret[CADDRESS_LEN + 1];
    int j = 0;
    
    begin = MSGTYPE_LEN + FILENAME_LEN + CHUNK_LEN;
    
    for(i = begin; i < begin + CADDRESS_LEN; i++)
    {
        if(buf[i] != ' ') ret[j++] = buf[i];
    }
    ret[j] = '\0';
    
    strcpy(address, ret);
}






void
GetClientPort(char *buf, unsigned short *port)
{
    char ret[CPORT_LEN + 1];
    int begin;
    int i;
    int j = 0;
    
    begin = MSGTYPE_LEN + FILENAME_LEN + CHUNK_LEN + CADDRESS_LEN;
    
    for(i = begin; i < begin + CPORT_LEN; i++)
    {
        if(buf[i] != ' ') ret[j++] = buf[i];
    }
    ret[j] = '\0';
    
    *port = (unsigned short)atoi(ret); /* NEEDED? */
}





void
GetRequestedFileName(char *buf, char *filename)
{
    int i;
    int j = 0;
    
    for(i = MSGTYPE_LEN; i < FILENAME_LEN + 1; i++)
    {
        if(buf[i] != ' ') filename[j++] = buf[i];
    }
    filename[j] = '\0';

}





int
IsValidFileName(char *filename)
{
    if ( (filename[0] == '/') || (filename[0] == '.') ) return 0;
    else return 1;
}






void
GetRequestedChunk(char *buf, int *chunk)
{
    int i;
    int j = 0;
    char requested_chunk[CHUNK_LEN + 1];
    
    for(i = FILENAME_LEN + 1; i < FILENAME_LEN + CHUNK_LEN + 2; i++)
    {
        if(buf[i] != ' ') requested_chunk[j++] = buf[i];
    }
    requested_chunk[j] = '\0';
    
    *chunk = atoi(requested_chunk);
}






void
PrintRequestMessage(char *msg)
{
    char fname[FILENAME_LEN + 1];
    int chunk_num;
    char client_address[CADDRESS_LEN + 1];
    unsigned short client_port;
    
    GetRequestedFileName(msg, fname);
    GetRequestedChunk(msg, &chunk_num);
    GetClientAddress(msg, client_address);
    GetClientPort(msg, &client_port);
    
    fprintf(stderr, "[Request Message - %d]\n", strlen(msg));
    fprintf(stderr, "\t     File Name : %s\n", fname);
    fprintf(stderr, "\t  Chunk Number : %d\n", chunk_num);
    fprintf(stderr, "\tClient Address : %s\n", client_address);
    fprintf(stderr, "\t   Client Port : %d\n", client_port);
    
    return;
}




void
GetResponseMessage(char *inmsg, char *outmsg)
{    
    char filename[FILENAME_LEN + 1];        /* requested file name */
    int requested_chunk;                    /* requested chunk number */
    char client_address[CADDRESS_LEN + 1];  /* address of client */
    unsigned short client_port;             /* port of client */
    int total_chunks;                       /* total # of chunks in file */
    char temp[CHUNK_LEN + 1];               /* requested chunk - string */
    int i;                                  /* loops et al */
    char data[DATA_LEN + 1];                /* file chunk */
    int status;                             /* function returns */
    int datsize;                            /* size of data chunk */

    /* extract info from request message */
    GetRequestedFileName(inmsg, filename);
    GetRequestedChunk(inmsg, &requested_chunk);
    GetClientAddress(inmsg, client_address);
    GetClientPort(inmsg, &client_port);
    
    /* get total number of chunks in file */
    total_chunks = GetTotalChunks(filename);
    
    /* check for errors - construct CTL messages */
    if(!IsValidFileName(filename)) strcpy(outmsg, "CERRORUNAUTHORIZED REQUEST");
    else if(inmsg[0] != 'R') strcpy(outmsg, "CERRORBAD REQUEST FORMAT");
    else if(requested_chunk < 0) strcpy(outmsg, "CERRORBAD CHUNK REQUEST");
    else if(requested_chunk == 0) /* number of chunks requested */
    {
        if(total_chunks == -1) strcpy(outmsg, "CERRORCANNOT STAT FILE");
        else if(total_chunks == 0) strcpy(outmsg, "CERRORFILE SIZE NULL");
        else if(total_chunks > 99999) strcpy(outmsg, "CERRORTOO MANY CHUNKS");
        else
        {
            sprintf(temp, "%d", total_chunks);
            strcpy(outmsg, "CCHNKS");
            strcat(outmsg, temp);
        }
    }
    else /* construct data message */
    {
        status = GetFileChunk(filename, requested_chunk, data, &datsize);
        if(status != 0)
        {
            if(status == -1) strcpy(outmsg, "CERRORFILE SIZE NULL");
            else if (status == -2) strcpy(outmsg, "CERRORCANNOT STAT FILE");
            else if(status == -3) strcpy(outmsg, "CERRORCHUNK OUT OF BOUND");
            else strcpy(outmsg, "CERRORUNKNOWN ERROR");
        }
        else
        {
            /* MSGTYPE */
            strcpy(outmsg, "D");
            /* CHUNK */
            sprintf(temp, "%d", requested_chunk);
            for(i = 0; i < CHUNK_LEN - strlen(temp); i++) strcat(outmsg, " ");
            strcat(outmsg, temp);
            /* data */
            strcat(outmsg, data);
            outmsg[datsize + MSGTYPE_LEN + CHUNK_LEN] = '\0';
        }
    }
}






/* returns 0 if successful */
/* returns -1 if file size null */
/* returns -2 if stat failed */
/* returns -3 if chunk out of bounds */
int
GetFileChunk(char *filename, int req_chunk, char *buf, int *size)
{
    FILE *fp;
    int total_chunks;
    int i;
    long long skipsize;
    char *skipdata;
    
    total_chunks = GetTotalChunks(filename);
    if(total_chunks == 0) return -1;
    if(total_chunks == -1) return -2;
    
    if(req_chunk > total_chunks) return -3;
    
    fp = fopen(filename, "rb");
    if(fp == NULL) return -2;
    
    /*
    for(i = 0; i < req_chunk; i++)
    {
        //*size = fread(buf, 1, DATA_LEN, fp);
        *size = fread(buf, DATA_LEN, 1, fp);
    }
    */
    
    skipsize = DATA_LEN * (req_chunk - 1);
    
    fseek(fp, skipsize, SEEK_SET);
    //i = fread(skipdata, skipsize, 1, fp); printf("i = %d\n", i);
    
    *size = fread(buf, 1, DATA_LEN, fp);
    
    fclose(fp);

    return 0;
}






/* returns 0 if file size null */
/* returns -1 if stat failed */
int
GetTotalChunks(char *filename)
{
    struct stat statbuf;
    int statret;
    
    statret = stat(filename, &statbuf);
    if(statret == 0)
    {
        if(statbuf.st_size != 0) return statbuf.st_size / DATA_LEN + 1;
        else return 0;
    }
    else return -1;
}









int
CreateRecvSocket()
{
    int recv_sock;
	int length;
	struct sockaddr_in recv_name;
    struct timeval tv;
    unsigned short port;
    
    port = SERVER_RECV_PORT;

    /* create a socket to read from */
	recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(recv_sock < 0)
	{
		perror("Opening receiving socket");
		exit(1);
	}
    

	/* create name with wildcards */
	recv_name.sin_family = AF_INET;
	recv_name.sin_addr.s_addr = INADDR_ANY;
	recv_name.sin_port = port;

	if(bind(recv_sock, (struct sockaddr *)&recv_name, sizeof(recv_name)))
	{
		perror("Binding receiving socket");
		exit(1);
	}

	/* acquire and print port number */
	length = sizeof(recv_name);
	if(getsockname(recv_sock, (struct sockaddr *)&recv_name, &length))
	{
		perror("Getting receiving socket name");
		exit(1);
	}
    
	fprintf(stderr, "Listening on port : %d\n", ntohs(recv_name.sin_port));
    
    return recv_sock;
}




int CreateSendSocket()
{
    int send_sock;
    
    send_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(send_sock < 0)
	{
		perror("Opening sending socket");
		exit(1);
	}
    
    return send_sock;
}













/* ************************************************************************** */

int main(int argc, char *argv[])
{
    /* messages */
    char recvd_msg[RMSGSIZE];
    char *response_msg;
    /* sockets */
    int recv_sock;
    int send_sock;
    /* send related */
    struct sockaddr_in send_name;
    struct hostent *hp, *gethostbyname();
    /* client related */
    char client_address[CADDRESS_LEN + 1];
    unsigned short client_port;

    /* create socket to listen on  - no timeout */
    recv_sock = CreateRecvSocket();
    
    /* create socket to send on */
    send_sock = CreateSendSocket();
    
    /* ** start server ** */
    while(1)
    {
        /* read from socket */
        if(read(recv_sock, recvd_msg, RMSGSIZE+1) < 0)
        {
            perror("Reading from socket");
        }
        else
        {
            GetClientAddress(recvd_msg, client_address);
            GetClientPort(recvd_msg, &client_port);

            /* construct socket to send to */
            hp = gethostbyname(client_address);
            if(hp == 0)
            {
                fprintf(stderr, "Unknown host : %s\n", client_address);
                continue;
            }
            bcopy(hp->h_addr, &send_name.sin_addr, hp->h_length);
            send_name.sin_family = AF_INET;
            send_name.sin_port = htons(client_port);
            
            /* send reponse */
            GetResponseMessage(recvd_msg, response_msg);
            if(response_msg[0] == 'C')
            {
                if(sendto(send_sock, response_msg, CMSGSIZE, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0)
                {
                    perror("Sending message");
                }
            }
            if(response_msg[0] == 'D')
            {
                if(sendto(send_sock, response_msg, DMSGSIZE, 0, (struct sockaddr *)&send_name, sizeof(send_name)) < 0)
                {
                    perror("Sending message");
                }
            } 
        }
        
    }
    
    return 0;
}

/* ************************************************************************** */
