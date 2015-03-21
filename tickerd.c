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

#include "mjson.h"

#define OUTPUT_FILE	"/var/www/htdocs/bitcoinxtr/price.txt"
#define PRICE_FMT	"$%.2f USD"

static double rate;
static char code[4], name[10];

static const struct json_attr_t json_attrs[] = {
	{"code", t_string, .addr.string = code, .len = sizeof(code)},
	{"name", t_string, .addr.string = name, .len = sizeof(name)},
	{"rate", t_real,   .addr.real = &rate},
	{NULL},
};

static void
update_ticker(char *host, char *path, FILE *fp)
{
	struct tls_config *config;
	struct tls *ctx;
	size_t len;
	char buf[4096];
	char *json, *port = "443";

	config = tls_config_new();

	ctx = tls_client();
	tls_configure(ctx, config);

	tls_connect(ctx, host, port);

	snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path,
		host);
	tls_write(ctx, buf, strlen(buf), &len);
	tls_read(ctx, buf, sizeof(buf), &len);

	tls_close(ctx);
	tls_free(ctx);
	tls_config_free(config);

	json = strstr(buf, "\r\n\r\n");
	json = json + 4;

	if (json_read_object(json, json_attrs, NULL) == 0) {
		rewind(fp);
		fprintf(fp, PRICE_FMT, rate);
		fflush(fp);
	}
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

	tls_init();

	while (1) {
		update_ticker("bitpay.com", "/api/rates/usd", fp);
		sleep(60);
	}

	fclose(fp);
	return (0);
}
