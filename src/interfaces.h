/*
 * ConnMan GTK GUI
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 * Author: Jaakko Hannikainen <jaakko.hannikainen@intel.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CONNMAN_GTK_INTERFACES_H
#define _CONNMAN_GTK_INTERFACES_H

#define CONNMAN_PATH "net.connman"
#define CONNMAN_VPN_PATH "net.connman.vpn"

#define MANAGER_NAME CONNMAN_PATH ".Manager"
#define MANAGER_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.Manager\">" \
        "    <method name=\"GetProperties\">" \
        "        <arg name=\"properties\" type=\"a{sv}\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"SetProperty\">" \
        "        <arg name=\"name\" type=\"s\" direction=\"in\"/>" \
        "        <arg name=\"value\" type=\"v\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"GetTechnologies\">" \
        "        <arg name=\"technologies\" type=\"a(oa{sv})\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"GetServices\">" \
        "        <arg name=\"services\" type=\"a(oa{sv})\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"RegisterAgent\">" \
        "        <arg name=\"path\" type=\"o\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"UnregisterAgent\">" \
        "        <arg name=\"path\" type=\"o\" direction=\"in\"/>" \
        "    </method>" \
        "    <signal name=\"PropertyChanged\">" \
        "        <arg name=\"name\" type=\"s\"/>" \
        "        <arg name=\"value\" type=\"v\"/>" \
        "    </signal>" \
        "    <signal name=\"TechnologyAdded\">" \
        "        <arg name=\"path\" type=\"o\"/>" \
        "        <arg name=\"properties\" type=\"a{sv}\"/>" \
        "    </signal>" \
        "    <signal name=\"TechnologyRemoved\">" \
        "        <arg name=\"path\" type=\"o\"/>" \
        "    </signal>" \
        "    <signal name=\"ServicesChanged\">" \
        "        <arg name=\"changed\" type=\"a(oa{sv})\"/>" \
        "        <arg name=\"removed\" type=\"ao\"/>" \
        "    </signal>" \
        "</interface>" \
        "</node>"

#define TECHNOLOGY_NAME CONNMAN_PATH ".Technology"
#define TECHNOLOGY_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.Technology\">" \
        "    <method name=\"SetProperty\">" \
        "        <arg name=\"name\" type=\"s\" direction=\"in\"/>" \
        "        <arg name=\"value\" type=\"v\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"GetProperties\">" \
        "        <arg name=\"properties\" type=\"a{sv}\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"Scan\"></method>" \
        "    <signal name=\"PropertyChanged\">" \
        "        <arg name=\"name\" type=\"s\"/>" \
        "        <arg name=\"value\" type=\"v\"/>" \
        "    </signal>" \
        "</interface>" \
        "</node>"

#define SERVICE_NAME CONNMAN_PATH ".Service"
#define SERVICE_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.Service\">" \
        "    <method name=\"SetProperty\">" \
        "        <arg name=\"name\" type=\"s\" direction=\"in\"/>" \
        "        <arg name=\"value\" type=\"v\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"Connect\"></method>" \
        "    <method name=\"Disconnect\"></method>" \
        "    <signal name=\"PropertyChanged\">" \
        "        <arg name=\"name\" type=\"s\"/>" \
        "        <arg name=\"value\" type=\"v\"/>" \
        "    </signal>" \
        "</interface>" \
        "</node>"

#define AGENT_NAME CONNMAN_PATH ".Agent"
#define AGENT_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.Agent\">" \
        "    <method name=\"Release\"></method>" \
        "    <method name=\"ReportError\">" \
        "        <arg name=\"service\" type=\"o\" direction=\"in\"/>" \
        "        <arg name=\"error\" type=\"s\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"RequestBrowser\">" \
        "        <arg name=\"service\" type=\"o\" direction=\"in\"/>" \
        "        <arg name=\"url\" type=\"s\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"RequestInput\">" \
        "        <arg name=\"service\" type=\"o\" direction=\"in\"/>" \
        "        <arg name=\"fields\" type=\"a{sv}\" direction=\"in\"/>" \
        "        <arg name=\"values\" type=\"a{sv}\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"Cancel\"></method>" \
        "</interface>" \
        "</node>"

#define VPN_MANAGER_NAME CONNMAN_VPN_PATH ".Manager"
#define VPN_MANAGER_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.vpn.Manager\">" \
        "    <method name=\"Create\">" \
        "        <arg name=\"settings\" type=\"a{sv}\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"Remove\">" \
        "        <arg name=\"vpn\" type=\"o\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"GetConnections\">" \
        "        <arg name=\"connections\" type=\"a(oa{sv})\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"RegisterAgent\">" \
        "        <arg name=\"path\" type=\"o\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"UnregisterAgent\">" \
        "        <arg name=\"path\" type=\"o\" direction=\"in\"/>" \
        "    </method>" \
        "    <signal name=\"ConnectionAdded\">" \
        "        <arg name=\"path\" type=\"o\"/>" \
        "        <arg name=\"properties\" type=\"a{sv}\"/>" \
        "    </signal>" \
        "    <signal name=\"ConnectionRemoved\">" \
        "        <arg name=\"path\" type=\"o\"/>" \
        "    </signal>" \
        "</interface>" \
        "</node>"

#define VPN_AGENT_NAME CONNMAN_VPN_PATH ".Agent"
#define VPN_AGENT_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.vpn.Agent\">" \
        "    <method name=\"Release\"></method>" \
        "    <method name=\"ReportError\">" \
        "        <arg name=\"service\" type=\"o\" direction=\"in\"/>" \
        "        <arg name=\"error\" type=\"s\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"RequestInput\">" \
        "        <arg name=\"service\" type=\"o\" direction=\"in\"/>" \
        "        <arg name=\"fields\" type=\"a{sv}\" direction=\"in\"/>" \
        "        <arg name=\"values\" type=\"a{sv}\" direction=\"out\"/>" \
        "    </method>" \
        "    <method name=\"Cancel\"></method>" \
        "</interface>" \
        "</node>"

#define VPN_CONNECTION_NAME CONNMAN_VPN_PATH ".Connection"
#define VPN_CONNECTION_INTERFACE \
        "<node>" \
        "<interface name=\"net.connman.vpn.Connection\">" \
        "    <method name=\"SetProperty\">" \
        "        <arg name=\"name\" type=\"s\" direction=\"in\"/>" \
        "        <arg name=\"value\" type=\"v\" direction=\"in\"/>" \
        "    </method>" \
        "    <method name=\"Connect\"></method>" \
        "    <method name=\"Disconnect\"></method>" \
        "    <signal name=\"PropertyChanged\">" \
        "        <arg name=\"name\" type=\"s\"/>" \
        "        <arg name=\"value\" type=\"v\"/>" \
        "    </signal>" \
        "</interface>" \
        "</node>"

#endif /* _CONNMAN_GTK_INTERFACES_H */
