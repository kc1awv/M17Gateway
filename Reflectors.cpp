/*
*   Copyright (C) 2016,2018,2020 by Jonathan Naylor G4KLX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Reflectors.h"
#include "M17Defines.h"
#include "Log.h"

#include <algorithm>
#include <functional>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <cctype>

CReflectors::CReflectors(const std::string& hostsFile1, const std::string& hostsFile2, unsigned int reloadTime) :
m_hostsFile1(hostsFile1),
m_hostsFile2(hostsFile2),
m_reflectors(),
m_timer(1000U, reloadTime * 60U)
{
	if (reloadTime > 0U)
		m_timer.start();
}

CReflectors::~CReflectors()
{
	for (std::vector<CM17Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it)
		delete *it;

	m_reflectors.clear();
}

bool CReflectors::load()
{
	// Clear out the old reflector list
	for (std::vector<CM17Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it)
		delete *it;

	m_reflectors.clear();

	FILE* fp = ::fopen(m_hostsFile1.c_str(), "rt");
	if (fp != NULL) {
		char buffer[100U];
		while (::fgets(buffer, 100U, fp) != NULL) {
			if (buffer[0U] == '#')
				continue;

			char* p1 = ::strtok(buffer, " \t\r\n");
			char* p2 = ::strtok(NULL,   " \t\r\n");
			char* p3 = ::strtok(NULL,   " \t\r\n");

			if (p1 != NULL && p2 != NULL && p3 != NULL) {
				std::string name = std::string(p1);
				name.resize(M17_CALLSIGN_LENGTH - 2U, ' ');

				std::string host = std::string(p2);

				unsigned int port= (unsigned int)::atoi(p3);

				sockaddr_storage addr;
				unsigned int addrLen;
				if (CUDPSocket::lookup(host, port, addr, addrLen) == 0) {
					CM17Reflector* refl = new CM17Reflector;
					refl->m_name    = name;
					refl->m_addr    = addr;
					refl->m_addrLen = addrLen;
					m_reflectors.push_back(refl);
				} else {
					LogWarning("Unable to resolve the address of %s", host.c_str());
				}
			}
		}

		::fclose(fp);
	}

	fp = ::fopen(m_hostsFile2.c_str(), "rt");
	if (fp != NULL) {
		char buffer[100U];
		while (::fgets(buffer, 100U, fp) != NULL) {
			if (buffer[0U] == '#')
				continue;

			char* p1 = ::strtok(buffer, " \t\r\n");
			char* p2 = ::strtok(NULL, " \t\r\n");
			char* p3 = ::strtok(NULL, " \t\r\n");

			if (p1 != NULL && p2 != NULL && p3 != NULL) {
				// Don't allow duplicate reflector ids from the secondary hosts file.
				std::string name = std::string(p1);
				name.resize(M17_CALLSIGN_LENGTH - 2U, ' ');

				if (find(name) == NULL) {
					std::string host  = std::string(p2);
					unsigned int port = (unsigned int)::atoi(p3);

					sockaddr_storage addr;
					unsigned int addrLen;
					if (CUDPSocket::lookup(host, port, addr, addrLen) == 0) {
						CM17Reflector* refl = new CM17Reflector;
						refl->m_name    = name;
						refl->m_addr    = addr;
						refl->m_addrLen = addrLen;
						m_reflectors.push_back(refl);
					} else {
						LogWarning("Unable to resolve the address of %s", host.c_str());
					}
				}
			}
		}

		::fclose(fp);
	}

	size_t size = m_reflectors.size();
	LogInfo("Loaded %u M17 reflectors", size);

	if (size == 0U)
		return false;

	return true;
}

CM17Reflector* CReflectors::find(const std::string& name)
{
	std::string nm = name;
	nm.resize(3U);

	for (std::vector<CM17Reflector*>::iterator it = m_reflectors.begin(); it != m_reflectors.end(); ++it) {
		if (nm == (*it)->m_name)
			return *it;
	}

	return NULL;
}

void CReflectors::clock(unsigned int ms)
{
	m_timer.clock(ms);

	if (m_timer.isRunning() && m_timer.hasExpired()) {
		load();
		m_timer.start();
	}
}
