// =====================================================================
//
// xmlrpc_rig.cxx
//
// connect to client xmlrpc server
//
// Copyright (C) 2023
//		Dave Freese, W1HKJ
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// It is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// =====================================================================

#include <string>
#include <iostream>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#include "XmlRpc.h"

#include "config.h"
#include "status.h"
#include "util.h"
#include "threads.h"
#include "tod_clock.h"

#include "xmlrpc_rig.h"
#include "trace.h"

using namespace XmlRpc;

int xmlrpc_verbosity = 0;

XmlRpcClient *flrig_client = (XmlRpcClient *)0;

bool is_binary(std::string s)
{
	for (size_t n = 0; n < s.length(); n++)
		if (!isalnum(s[n]))
			return true;
	return false;
}

void xmlrpc_ptt( int PTT)
{
	XmlRpcValue Args, result;
	Args = (int)PTT;

	try {
		flrig_client->execute("rig.set_ptt", Args, result);
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.set_ptt: %s", err->getMessage().c_str());
	}
}

bool xml_ptt_state()
{
	XmlRpcValue Args, result;

	try {
		flrig_client->execute("rig.get_ptt", Args, result);
		bool ptt = (int)result;
		return ptt;
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.get_ptt: %s", err->getMessage().c_str());
		return false;
	}
}


bool connected_to_client = false;

std::string xml_cat_string( std::string send )
{
	XmlRpcValue Args, result;
	if (is_binary(send))
		Args = (std::string)to_hex(send);
	else
		Args = (std::string)send;

	try {

		xml_trace(2, "rig_client_string send:", to_hex(send).c_str());

		if (flrig_client->execute("rig.client_string", Args, result)) {

			std::string res = (std::string)result;

			xml_trace(2, "rig.client_string recv:", res.c_str());

			if (res.find("x") != std::string::npos) {
				std::string raw;
				size_t p = 0;
				unsigned int val;
				while (( p = res.find("x", p)) != std::string::npos) {
					sscanf(&res[p+1], "%x", &val);
					raw += (unsigned char) val;
					p += 3;
				}
				res = raw;
			}

			return res;
		} else {
			return "?";
		}
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.client_string: %s", err->getMessage().c_str());
		return "?";
	}
}

std::string client_get_xcvr()
{
	std::string _xcvr_name;
	XmlRpcValue result;
	try {
		if (flrig_client->execute("rig.get_xcvr", XmlRpcValue(), result) ) {
			_xcvr_name = std::string(result);
		}
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.get_xcvr: %s", err->getMessage().c_str());
		connected_to_client = false;
	}
	return _xcvr_name;
}

bool client_connection()
{
	XmlRpcValue noArgs, result;
	try {
		if (flrig_client->execute("system.listMethods", noArgs, result)) {
			int nargs = result.size();
			std::string method_str = "\nMethods:\n";
			for (int i = 0; i < nargs; i++)
				method_str.append("    ").append(result[i]).append("\n");

			connected_to_client = true;
			client_get_xcvr();
		} else {
			connected_to_client = false;
		}
	} catch (...) {
		return false;
	}
	return connected_to_client;
}

bool connect_to_client()
{
	XmlRpc::setVerbosity(xmlrpc_verbosity);
	if (flrig_client) {
		delete flrig_client;
		flrig_client = (XmlRpcClient *)0;
	}
	try {
		flrig_client = new XmlRpcClient(
				progStatus.xmlrig_addr.c_str(),
				atol(progStatus.xmlrig_port.c_str()));
		return client_connection();
	} catch (...) {
		xml_trace(3,"Cannot connect to %s : %s", progStatus.xmlrig_addr.c_str(), progStatus.xmlrig_port.c_str());
		return false;
	}
}
