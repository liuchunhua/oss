/*
	author:lch
*/

typedef struct{
	char *authorization;
	char *content_length;
	char *content_type;
	char *date;
	char *host;
}CommonRequest;

typedef struct{
	char *content_length;
	char *connection;
	char *date;
	char *etag;
	char *server;
	char *x-oss-request-id;
}CommonResponse;


