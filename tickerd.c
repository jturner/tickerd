/*
 * Copyright (c) 2014 James Turner <james@calminferno.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <tls.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parson.h"

#define OUTPUT_FILE	"/var/www/htdocs/bitcoinxtr/price.txt"
#define PRICE_FMT	"$%.2f USD"

static int cnt = 0;
static double price = 0;

static void
update_ticker(char *host, char *path)
{
	char *sub = NULL, *port = "443";
	char buf[4096];
	struct tls_config *config;
	struct tls *ctx;
	size_t len;
	JSON_Value *root;
	JSON_Object *ticker;

	tls_init();
	config = tls_config_new();

	ctx = tls_client();
	tls_configure(ctx, config);

	tls_connect(ctx, host, port);

	snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path,
		host);
	tls_write(ctx, buf, strlen(buf), &len);
	tls_read(ctx, buf, sizeof(buf), &len);

	tls_close(ctx);
	tls_config_free(config);
	tls_free(ctx);

	sub = strstr(buf, "\r\n\r\n");
	sub = sub + 4;

	root = json_parse_string(sub);

	if (json_value_get_type(root) == JSONObject) {
		ticker = json_value_get_object(root);

		if (json_value_get_type(json_object_get_value(ticker,
			"USD")) == JSONObject) {
			price = price + json_object_dotget_number(ticker,
				"USD.last");
			cnt++;
		} else {
			price = price + json_object_dotget_number(ticker,
				"ticker.last");
			cnt++;
		}
	}

	json_value_free(root);
}

static void
write_price(FILE *fp)
{
	if (cnt == 0)
		return;

	rewind(fp);
	fprintf(fp, PRICE_FMT, (price / cnt));
	fflush(fp);

	cnt = 0, price = 0;
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-d]\n", __progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch, debug = 0;
	FILE *fp;

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	if ((fp = fopen(OUTPUT_FILE, "w")) == NULL)
		err(1, "fopen");

	if (!debug && daemon(0, 0) == -1)
		err(1, "daemon");

	while (1) {
		update_ticker("btc-e.com", "/api/2/btc_usd/ticker");
		update_ticker("blockchain.info", "/ticker");
		write_price(fp);
		sleep(30);
	}

	fclose(fp);
	return (0);
}
