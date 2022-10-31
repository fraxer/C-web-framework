
enum protocol_t {
	HTTP1_1,
	// HTTP2_0,
	WEBSOCKET,
	// SMTP,
};

typedef struct request {
	protocol_t protocol;
	method_t method;
} request_t;