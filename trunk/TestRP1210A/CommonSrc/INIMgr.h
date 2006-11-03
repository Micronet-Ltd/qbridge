#pragma once

class INIMgr {
public:
	INIMgr(void);
	virtual ~INIMgr(void);

	struct StringComp {
		inline bool operator() (const CString &p1, const CString &p2) const { return p1.CompareNoCase(p2) < 0; }
	};
	struct Entry {
		CString base;
		vector <CString> parsedList;
	};
	struct Section {
		map <CString, Entry, StringComp> entries;
	};

	struct Devices {
		Devices() : deviceID(0) {}
		CString dll;
		CString deviceName;
		CString deviceDesc;
		vector <CString> protocolIDs;
		vector <CString> protocolStrings;
		vector <CString> protocolDescs;
		CString vendorName;
		CString msgString;
		CString errString;
		int deviceID;
	};

	static vector<CString> GetRP1210Implementations();
	static vector<Devices> GetDevices();
	static CString GetWindowsPath();
	static char * ToAnsi(const CString &unicodeStr);


	bool Parse (const CString &path);
	bool GetEntry (const CString &section, const CString &item, Entry &e);

private:
	map<CString, Section, StringComp> sections;
};
