/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLGNAME "plugin-testing"
#include "plugin_helpers.h"
#include <cstdlib>
#include <iostream>
using namespace std;


// This is a dummy plugin for testing DD's reaction to a plugin result
// specify what you want to have returned in the premium-user.
// eg: premium_user = PLUGIN_SUCCESS,allows_multiple,allows_resumption
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	if(inp.premium_user.find("PLUGIN_ERROR") != string::npos)
		return PLUGIN_ERROR;

	if(inp.premium_user.find("PLUGIN_FILE_NOT_FOUND") != string::npos)
		return PLUGIN_FILE_NOT_FOUND;

	if(inp.premium_user.find("PLUGIN_LIMIT_REACHED") != string::npos) {
		set_wait_time(999);
		return PLUGIN_LIMIT_REACHED;
	}

	outp.download_url = "http://cdimage.ubuntu.com/hardy/daily/current/hardy-alternate-amd64.template";
	return PLUGIN_SUCCESS;


}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(inp.premium_user.find("allows_resumption") != string::npos)
		outp.allows_resumption = true;
	else
		outp.allows_resumption = false;

	if(inp.premium_user.find("allows_multiple") != string::npos)
		outp.allows_multiple = true;
	else
		outp.allows_multiple = false;

	outp.offers_premium = true;
}
