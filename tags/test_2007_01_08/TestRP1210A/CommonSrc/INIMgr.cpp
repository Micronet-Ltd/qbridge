#include "StdAfx.h"
#include "INIMgr.h"

#include <fstream>
#include<algorithm>

#ifndef WINCE
#include <io.h>
#endif

/*******************/
/* INIMgr::INIMgr */
/*****************/
INIMgr::INIMgr(void)
{
}

/********************/
/* INIMgr::~INIMgr */
/******************/
INIMgr::~INIMgr(void)
{
}

/***************************/
/* INIMgr::GetWindowsPath */
/*************************/
CString INIMgr::GetWindowsPath() {
#ifdef WINCE
	TCHAR path[] = { _T("\\HardDisk\\BIN") };
#else 
	TCHAR path[MAX_PATH];

	if (SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, path) != S_OK) {
		return _T("");
	}
#endif
	return path;
}

/*******************/
/* INIMgr::ToAnsi */
/*****************/
char * INIMgr::ToAnsi(const CString &unicodeStr) {
	static char ansiName[256];
	WideCharToMultiByte(CP_ACP, 0, unicodeStr, -1, ansiName, 256, NULL, NULL);
	return ansiName;
}


/*************************************/
/* INIMgr::GetRP1210Implementations */
/***********************************/
vector<CString> INIMgr::GetRP1210Implementations() {
	vector<CString> retVal;

	INIMgr mgr;
	CString thePath = GetWindowsPath();
	if (!mgr.Parse (thePath + _T("\\RP121032.ini"))) {
		return retVal;
	}

	Entry e;
	if (!mgr.GetEntry(_T("RP1210Support"), _T("APIImplementations"), e)) {
		return retVal;
	}

	size_t i;
	for (i = 0; i < e.parsedList.size(); i++) {
		//retVal.push_back(thePath + _T("\\") + e.parsedList[i]);
		retVal.push_back(e.parsedList[i]);
	}
	return retVal;
}

/***********************/
/* INIMgr::GetDevices */
/*********************/
vector<INIMgr::Devices> INIMgr::GetDevices() {
	vector<Devices> retVal;
	vector<CString> impls = GetRP1210Implementations();
	size_t i;
	for (i = 0; i < impls.size(); i++) {
TRACE (_T("Implementation: %s\n"), impls[i]);
		CString fname = GetWindowsPath() + _T("\\") + impls[i] + _T(".ini");
		if (GetFileAttributes(fname) == 0xFFFFFFFF) {
			TRACE (_T("Unable to open %s\n"), fname);
			continue;
		}

		INIMgr ini;
		if (!ini.Parse(fname)) {
			TRACE (_T("Error parsing %s\n"), fname);
			continue;
		}
		Devices coreDev;
		coreDev.dll = impls[i] + _T(".dll");
		Entry e;
		if (ini.GetEntry(_T("VendorInformation"), _T("Name"), e)) {
			coreDev.vendorName = e.base;
		}
		if (ini.GetEntry(_T("VendorInformation"), _T("MessageString"), e)) {
			coreDev.msgString = e.base;
		}
		if (ini.GetEntry(_T("VendorInformation"), _T("ErrorString"), e)) {
			coreDev.errString = e.base;
		}
		if (ini.GetEntry(_T("VendorInformation"), _T("TimestampWeight"), e)) {
			coreDev.timeStampWeight = _ttoi(e.base);
		}

		vector <CString> protocolBaseList;
		if (ini.GetEntry(_T("VendorInformation"), _T("Protocols"), e)) {
			protocolBaseList = e.parsedList;
		}

		//vector <CString> deviceList;
		if (ini.GetEntry(_T("VendorInformation"), _T("Devices"), e)) {
			size_t i;
			for (i = 0; i < e.parsedList.size(); i++) {
				//deviceList.push_back(_T(_T("DeviceInformation")) + e.parsedList[i]);
				Devices dev = coreDev;
				Entry diEnt;
				if (ini.GetEntry (_T("DeviceInformation") + e.parsedList[i], _T("DeviceID"), diEnt)) {
					dev.deviceID = _ttoi(diEnt.base);
				}
				if (ini.GetEntry (_T("DeviceInformation") + e.parsedList[i], _T("DeviceName"), diEnt)) {
					dev.deviceName = diEnt.base;
				}
				if (ini.GetEntry (_T("DeviceInformation") + e.parsedList[i], _T("DeviceDescription"), diEnt)) {
					dev.deviceDesc = diEnt.base;
				}

				for (size_t j = 0; j < protocolBaseList.size(); j++) {
					Entry f;
					if (ini.GetEntry(_T("ProtocolInformation") + protocolBaseList[j], _T("Devices"), f)) {
						if (find(f.parsedList.begin(), f.parsedList.end(), e.parsedList[i]) != f.parsedList.end()) {
							dev.protocolIDs.push_back(protocolBaseList[j]);
							Entry f;
							ini.GetEntry(_T("ProtocolInformation") + protocolBaseList[j], _T("ProtocolString"), f);
							dev.protocolStrings.push_back(f.base);
							f.base = _T("");
							ini.GetEntry(_T("ProtocolInformation") + protocolBaseList[j], _T("ProtocolDescription"), f);
							dev.protocolDescs.push_back(f.base);
						}
					}
				}
TRACE (_T("Device found.  %s (%s, %d)  protSize=%d\n"), dev.deviceName, dev.deviceDesc, dev.deviceID, dev.protocolIDs.size());
				if (dev.deviceID != 0) {
					retVal.push_back(dev);
				}
			}
		}
	}
	return retVal;
}

/******************/
/* INIMgr::Parse */
/****************/
bool INIMgr::Parse (const CString &path) {
	ifstream is;
	is.open(path);
	if (!is.is_open()) {
		return false;
	}

	map<CString, Section, StringComp>::iterator sec = sections.end();
	while (!is.eof()) {
#pragma message ("Watch unicode here")
		char lineBuf[1024];
		is.getline(lineBuf, 1023, '\n');
		CString line (lineBuf);
		line = line.Trim();
		if (line.GetLength() == 0) {
			continue;
		} else if (line.Left(1) == _T('[')) {
			// found a new section
			line = line.Trim(_T("[]"));
			sections[line] = Section();
			sec = sections.find(line);
		} else {
			int pos = line.Find(_T('='));
			if (pos < 0) {
				continue;
			}
			CString item = line.Left(pos).Trim();
			CString value = line.Mid(pos+1).Trim();
			if (sec != sections.end()) {
				Entry theEntry;
				theEntry.base = value;
				int pos, oldPos = 0;
				pos = value.Find(_T(','));
				while (pos >= 0) {
					theEntry.parsedList.push_back(value.Mid(oldPos, pos-oldPos).Trim());
					oldPos = pos+1;
					pos = value.Find(_T(','), oldPos);
				}
				// append end to array
				if (oldPos < value.GetLength()) {
					theEntry.parsedList.push_back(value.Mid(oldPos).Trim());
				}

				sec->second.entries[item] = theEntry;
			}
		}
	}

	return true;
}

/*********************/
/* INIMgr::GetEntry */
/*******************/
bool INIMgr::GetEntry (const CString &section, const CString &item, Entry &e) {
	map<CString, Section, StringComp>::iterator it = sections.find(section);
	if (it == sections.end()) {
		return false;
	}

	map<CString, Entry, StringComp>::iterator eit = it->second.entries.find(item);
	if (eit == it->second.entries.end()) {
		return false;
	}
	e = eit->second;
	return true;
}
