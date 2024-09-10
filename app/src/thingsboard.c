#include "thingsboard.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/net/socket.h>

#include <string.h>

LOG_MODULE_REGISTER(thingsboard, CONFIG_TBAT_LOG_LEVEL);

#define MAX_COAP_URI_PATH_LEN 256

static int sock;
static struct sockaddr_storage server_addr;
static struct coap_client client;
static struct coap_client_request request;

static void ipv4_addr_to_str(struct sockaddr_in *addr, char addr_str[NET_IPV4_ADDR_LEN])
{
	inet_ntop(AF_INET, &addr->sin_addr.s_addr, addr_str, NET_IPV4_ADDR_LEN);
}

static int resolve_host(char *hostname, uint16_t port)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM};
	char ipv4_addr_str[NET_IPV4_ADDR_LEN];
	struct sockaddr_in *server_addr4;

	err = getaddrinfo(hostname, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo failed %d", err);
		return err;
	}

	if (result == NULL) {
		LOG_ERR("Address not found");
		return -ENOENT;
	}

	server_addr4 = ((struct sockaddr_in *)&server_addr);
	server_addr4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server_addr4->sin_family = AF_INET;
	server_addr4->sin_port = htons(port);

	ipv4_addr_to_str(server_addr4, ipv4_addr_str);
	LOG_INF("Resolved %s to %s", hostname, ipv4_addr_str);

	freeaddrinfo(result);

	return 0;
}

static void decode_response_code(int16_t code, int *class, int *detail)
{
	*class = (code & 0xff) >> 5;
	*detail = code & 0x1f;
}

static void response_code_to_str(int16_t code, char str[5])
{
	int class, detail;
	decode_response_code(code, &class, &detail);
	sprintf(str, "%1d.%02d", class, detail);
}

void handle_response(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
		     bool last_block, void *user_data)
{
	ARG_UNUSED(offset);
	ARG_UNUSED(payload);
	ARG_UNUSED(len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(user_data);

	char code_str[15];
	response_code_to_str(result_code, code_str);
	LOG_INF("Received response with code %s", code_str);
}

int tb_init(char *hostname, uint16_t port)
{
	int err;

	LOG_INF("Initializing socket for coap://%s:%d", hostname, port);

	err = resolve_host(hostname, port);
	if (err) {
		return err;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket for CoAP communication, error (%d): %s", errno,
			strerror(errno));
		return -errno;
	}

	/*
	 * Not using connect() here to associate the socket with a server address.
	 * Depending on the server's configuration it can happen that the source address of the
	 * responses might not match the server address. In that case recv() is not able to
	 * receive the message if the socket is connected to that specific server address.
	 */

	err = coap_client_init(&client, NULL);
	if (err) {
		LOG_ERR("Failed initialize CoAP client, error (%d): %s", err, strerror(-err));
		return err;
	}

	return 0;
}

int tb_send_telemetry(char *payload, char *access_token)
{
	int err;
	char path[MAX_COAP_URI_PATH_LEN];

	if (!sock) {
		LOG_ERR("Socket not initialized. tb_init must be called first.");
		return -1;
	}

	err = snprintf(path, sizeof(path), "api/v1/%s/telemetry", access_token);
	if (err < 0 || err == sizeof(path)) {
		LOG_ERR("Failed to create URI string");
		return -EINVAL;
	}

	request.method = COAP_METHOD_POST;
	request.confirmable = false;
	request.path = path;
	request.fmt = COAP_CONTENT_FORMAT_APP_JSON;
	request.payload = (uint8_t *)payload;
	request.len = strlen(payload);
	request.cb = handle_response;

	err = coap_client_req(&client, sock, (struct sockaddr *)&server_addr, &request, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send CoAP request, error (%d): %s", err, strerror(-err));
		return err;
	}

	return 0;
}

void tb_close(void)
{
	coap_client_cancel_requests(&client);
	close(sock);
}
