/*
 * testplugin.c
 * OpenVPN LDAP Authentication Plugin Test Driver
 *
 * Copyright (c) 2005 Landon Fuller <landonf@threerings.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Landon Fuller nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <openvpn-plugin.h>

int main(int argc, const char *argv[]) {
	openvpn_plugin_handle_t handle;
	const char *config;
	unsigned int type;
	const char *envp[4]; /* username, password, ifconfig_pool_remote_ip, NULL */
	char username[30];
	char *password;
	char dynamic_conf[] = "/tmp/openvpn-auth-ldap-test.XXXXXXXXXXXXX";
	int err;

	if (argc != 2) {
		errx(1, "Usage: %s <config file>", argv[0]);
	} else {
		config = argv[1];
	}

	const char *argp[] = {
		"plugin.so",
		config,
		NULL
	};

	/* Grab username and password */
	printf("Username: ");
	fgets(username, sizeof(username), stdin);
	/* Strip off the trailing \n */
	username[strlen(username) - 1] = '\0';

	password = getpass("Password: ");

	asprintf((char **) &envp[0], "username=%s", username);
	asprintf((char **) &envp[1], "password=%s", password);
	envp[2] = "ifconfig_pool_remote_ip=10.0.50.1";
	envp[3] = NULL;

	handle = openvpn_plugin_open_v1(&type, argp, envp);

	if (!handle)
		errx(1, "Initialization Failed!\n");

	/* Authenticate */
	err = openvpn_plugin_func_v1(handle, OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY, argp, envp);
	if (err != OPENVPN_PLUGIN_FUNC_SUCCESS) {
		errx(1, "Authorization Failed!\n");
	} else {
		printf("Authorization Succeed!\n");
	}

	/* Client Connect */
	if (mkstemp(dynamic_conf) == -1)
		errx(1, "Error creating temporary file %s: %s\n", dynamic_conf, strerror(errno));

	if (unlink(dynamic_conf) != 0)
		errx(1, "Error deleting temporary file %s: %s\n", dynamic_conf, strerror(errno));

	/* Point '$1' to a temporary configuration file path */
	argp[1] = dynamic_conf;
	err = openvpn_plugin_func_v1(handle, OPENVPN_PLUGIN_CLIENT_CONNECT, argp, envp);
	if (err != OPENVPN_PLUGIN_FUNC_SUCCESS) {
		errx(1, "client-connect failed!\n");
	} else {
		printf("client-connect succeed!\n");
	}
	/* Reset argp */
	argp[1] = config;

	/* Client Disconnect */
	err = openvpn_plugin_func_v1(handle, OPENVPN_PLUGIN_CLIENT_DISCONNECT, argp, envp);
	if (err != OPENVPN_PLUGIN_FUNC_SUCCESS) {
		errx(1, "client-disconnect failed!\n");
	} else {
		printf("client-disconnect succeed!\n");
	}

	openvpn_plugin_close_v1(handle);
	free((char *) envp[0]);
	free((char *) envp[1]);

	exit(0);
}
