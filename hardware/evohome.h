/**
 * Evohome class for HGI80 gateway, evohome control, monitoring and integration with Domoticz
 *
 *  Copyright 2014 - fullTalgoRythm https://github.com/fullTalgoRythm/Domoticz-evohome
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 * 
 * based in part on https://github.com/mouse256/evomon
 * and details available at http://www.domoticaforum.eu/viewtopic.php?f=7&t=5806&start=90#p72564
 */


#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define RFX_SETID3(ID,id1,id2,id3) {id1=ID>>16&0xFF;id2=ID>>8&0xFF;id3=ID&0xFF;}
#define RFX_GETID3(id1,id2,id3) ((id1<<16)|(id2<<8)|id3)

class CEvohomeDataType
{
public:
	//uint24_t may occasionally be defined but is not portable etc.
	struct uint24_t {
		uint32_t val : 24;
	};
	
	virtual unsigned char Decode(const unsigned char* msg, unsigned char nOfs)=0;
	virtual unsigned char Encode(unsigned char* msg, unsigned char nOfs) const=0;
	virtual std::string Encode() const=0;
	
	static void Add(const uint24_t &in, unsigned char* msg, unsigned char &nOfs){msg[nOfs++]=(in.val>>16)&0xFF;msg[nOfs++]=(in.val>>8)&0xFF;msg[nOfs++]=in.val&0xFF;}
	static void Add(const uint16_t &in, unsigned char* msg, unsigned char &nOfs){msg[nOfs++]=(in>>8)&0xFF;msg[nOfs++]=in&0xFF;}
	static void Add(const int16_t &in, unsigned char* msg, unsigned char &nOfs){msg[nOfs++]=(in>>8)&0xFF;msg[nOfs++]=in&0xFF;}
	static void Add(const uint8_t &in, unsigned char* msg, unsigned char &nOfs){msg[nOfs++]=in;}
	static void Add(const CEvohomeDataType &in, unsigned char* msg, unsigned char &nOfs){in.Add(msg,nOfs);}
	void Add(unsigned char* msg, unsigned char &nOfs) const{nOfs=Encode(msg,nOfs);}
	
	static void Get(uint24_t &out, const unsigned char* msg, unsigned char &nOfs){out.val=msg[nOfs++]<<16;out.val|=msg[nOfs++]<<8;out.val|=msg[nOfs++];}
	static void Get(uint16_t &out, const unsigned char* msg, unsigned char &nOfs){out=msg[nOfs++]<<8;out|=msg[nOfs++];}
	static void Get(int16_t &out, const unsigned char* msg, unsigned char &nOfs){out=msg[nOfs++]<<8;out|=msg[nOfs++];}
	static void Get(uint8_t &out, const unsigned char* msg, unsigned char &nOfs){out=msg[nOfs++];}
	static void Get(CEvohomeDataType &out, const unsigned char* msg, unsigned char &nOfs){out.Get(msg,nOfs);}
	void Get(const unsigned char* msg, unsigned char &nOfs) {nOfs=Decode(msg,nOfs);}
};

class CEvohomeTemp : public CEvohomeDataType
{
public:
	CEvohomeTemp():m_nTemp(0){}
	CEvohomeTemp(int16_t nTemp):m_nTemp(nTemp){}
	CEvohomeTemp(const unsigned char* msg, unsigned char nOfs){Decode(msg,nOfs);}
	~CEvohomeTemp(){}
	
	static unsigned char DecodeTemp(int16_t &out, const unsigned char* msg, unsigned char nOfs){Get(out,msg,nOfs);return nOfs;}
	static std::string GetHexTemp(int16_t nTemp)
	{
		char szTmp[256];
		sprintf(szTmp,"%04x",nTemp);
		return szTmp;
	}
	
	virtual unsigned char Decode(const unsigned char* msg, unsigned char nOfs){return DecodeTemp(m_nTemp,msg,nOfs);}
	virtual std::string Encode() const{return GetHexTemp(m_nTemp);}
	virtual unsigned char Encode(unsigned char* msg, unsigned char nOfs) const{Add(m_nTemp,msg,nOfs);return nOfs;}
	
	operator int16_t() const{return m_nTemp;}
	double GetTemp(){return m_nTemp/100.0;}
	bool IsValid(){return m_nTemp!=0x7FFF;}
	
	int16_t m_nTemp;//all except DHW set point which is unsigned
};

class CEvohomeID : public CEvohomeDataType
{
public:
	enum devType {
		devController=1,
		devZone=4,
		devSensor=7, //includes DHW and outdoor sensor
		devRelay=13,
		devGateway=18,
		devRemote=30,
	};
	
	CEvohomeID():m_nID(0){}
	CEvohomeID(unsigned int nID) {SetID(nID); }
	CEvohomeID(unsigned char idType, unsigned int idAddr){SetID(idType,idAddr);}
	CEvohomeID(const std::string &szID){SetID(szID);}
	CEvohomeID(const unsigned char* msg, unsigned char nOfs){Decode(msg,nOfs);}
	~CEvohomeID(){}
	
	static unsigned char DecodeID(unsigned int &out, const unsigned char* msg, unsigned char nOfs)
	{
		out=msg[nOfs++] << 16;//operator ++ may run after eval of entire expression
		out|=msg[nOfs++] << 8;
		out|=msg[nOfs++];
		return nOfs;
	}
	
	virtual unsigned char Decode(const unsigned char* msg, unsigned char nOfs){return DecodeID(m_nID,msg,nOfs);}
	
	static unsigned char GetIDType(unsigned int nID){return ((nID>>18)&0x3F);}
	static unsigned int GetIDAddr(unsigned int nID){return (nID&0x3FFFF);}
	static unsigned int GetID(unsigned char idType, unsigned int idAddr){return idType<<18|idAddr;}
	static unsigned int GetID(const std::string &szID)
	{
		unsigned int idType;
		unsigned int idAddr;
		sscanf(szID.c_str(),"%u:%u",&idType,&idAddr);
		return GetID(static_cast<unsigned char>(idType),idAddr);
	}
	static std::string GetStrID(unsigned int nID)
	{
		if(!nID)
			return "-:-";
		char szTmp[256];
		sprintf(szTmp,"%hhu:%u",GetIDType(nID),GetIDAddr(nID));
		return szTmp;
	}
	static std::string GetHexID(unsigned int nID)
	{
		char szTmp[256];
		sprintf(szTmp,"%06x",nID);
		return szTmp;
	}
	
	bool IsValid(){return (m_nID!=0);}
	
	unsigned char GetIDType() const{return GetIDType(GetID());}
	unsigned int GetIDAddr() const{return GetIDAddr(GetID());}
	unsigned int GetID() const {return m_nID;}
	std::string GetStrID() const {return GetStrID(GetID());}
	operator unsigned int() const{return GetID();}
	
	virtual std::string Encode() const{return GetHexID(GetID());}
	virtual unsigned char Encode(unsigned char* msg, unsigned char nOfs) const{msg[nOfs++]=(GetID()>>16)&0xFF;msg[nOfs++]=(GetID()>>8)&0xFF;msg[nOfs++]=GetID()&0xFF;return nOfs;}
	
	void SetID(unsigned int nID){m_nID=nID;}
	void SetID(unsigned char idType, unsigned int idAddr){SetID(GetID(idType,idAddr));}
	void SetID(const std::string &szID){SetID(GetID(szID));}
	
	unsigned int m_nID;
};

class CEvohomeDateTime : public CEvohomeDataType
{
public:
	CEvohomeDateTime():mins(0xFF),hrs(0xFF),day(0xFF),month(0xFF),year(0xFFFF){}
	template <class T> CEvohomeDateTime(const T &in){*this=in;}
	CEvohomeDateTime(const unsigned char* msg, unsigned char nOfs){Decode(msg,nOfs);}
	~CEvohomeDateTime(){}
	
	template <class T> CEvohomeDateTime& operator = (const T &in){year=in.year;month=in.month;day=in.day;hrs=in.hrs;mins=in.mins;return *this;}
	
	template <class T> static unsigned char DecodeTime(T &out, const unsigned char* msg, unsigned char nOfs)
	{
		out.mins=msg[nOfs++];
		out.hrs=msg[nOfs++];
		return nOfs;
	}

	template <class T> static unsigned char DecodeDate(T &out, const unsigned char* msg, unsigned char nOfs)
	{
		out.day=msg[nOfs++];
		out.month=msg[nOfs++];
		out.year=msg[nOfs++] << 8;//operator ++ may run after eval of entire expression
		out.year|=msg[nOfs++];
		return nOfs;
	}

	template <class T> static unsigned char DecodeDateTime(T &out, const unsigned char* msg, unsigned char nOfs)
	{
		DecodeDate(out,msg,DecodeTime(out,msg,nOfs));
		return nOfs;
	}

	template <class T> static std::string GetStrDate(const T &in)
	{
		if(in.year==0xFFFF)
			return "";
		char szTmp[256];
		sprintf(szTmp,std::string("%d-%02d-%02d").append((in.hrs!=0xFF)?" %02d:%02d":"").c_str(),in.year,in.month,in.day,in.hrs,in.mins);
		return szTmp;
	}
	
	template <class T> static std::string GetISODate(const T &in)
	{
		if(in.year==0xFFFF)
			return "";
		char szTmp[256];
		sprintf(szTmp,std::string("%d-%02d-%02d").append((in.hrs!=0xFF)?"T%02d:%02d:00":"T00:00:00").c_str(),in.year,in.month,in.day,in.hrs,in.mins);
		return szTmp;
	}
	
	template <class T> static void DecodeISODate(T &out, const char* str)
	{
		unsigned int y,m,d,h,n;
		sscanf(str,"%04u-%02u-%02uT%02u:%02u:",&y,&m,&d,&h,&n);
		out.year=static_cast<uint16_t>(y);
		out.month=static_cast<uint8_t>(m);
		out.day=static_cast<uint8_t>(d);
		out.hrs=static_cast<uint8_t>(h);
		out.mins=static_cast<uint8_t>(n);
	}
	
	unsigned char DecodeTime(const unsigned char* msg, unsigned char nOfs){return DecodeTime(*this,msg,nOfs);}
	unsigned char DecodeDate(const unsigned char* msg, unsigned char nOfs){return DecodeDate(*this,msg,nOfs);}
	virtual unsigned char Decode(const unsigned char* msg, unsigned char nOfs){return DecodeDateTime(*this,msg,nOfs);}
	
	std::string GetStrDate() const{return GetStrDate(*this);}
	
	virtual std::string Encode() const
	{
		char szTmp[256];
		sprintf(szTmp,"%02hhx%02hhx%02hhx%02hhx%04hx",mins,hrs,day,month,year);
		return szTmp;
	}
	virtual unsigned char Encode(unsigned char* msg, unsigned char nOfs) const{Add(mins,msg,nOfs);Add(hrs,msg,nOfs);Add(day,msg,nOfs);Add(month,msg,nOfs);Add(year,msg,nOfs);return nOfs;}
	
	uint8_t mins;
	uint8_t hrs;
	uint8_t day;
	uint8_t month;
	uint16_t year;
};

class CEvohomeDate : public CEvohomeDateTime
{
public:
	CEvohomeDate(){}
	CEvohomeDate(const unsigned char* msg, unsigned char nOfs){Decode(msg,nOfs);}
	~CEvohomeDate(){}
	
	virtual unsigned char Decode(const unsigned char* msg, unsigned char nOfs){return DecodeDate(*this,msg,nOfs);}
	virtual std::string Encode() const
	{
		char szTmp[256];
		sprintf(szTmp,"%02hhx%02hhx%04hx",day,month,year);
		return szTmp;
	}
	virtual unsigned char Encode(unsigned char* msg, unsigned char nOfs) const{Add(day,msg,nOfs);Add(month,msg,nOfs);Add(year,msg,nOfs);return nOfs;}
};

class CEvohomeZoneFlags
{
public:
	enum evoZoneFlags {
		flgLocalOverrideDisabled=1,
		flgWindowFunctionDisabled=2,
		flgSingleRoomZone=16,
	};
	static std::string GetFlags(int nFlags)
	{
		std::string szRet;
		
		szRet+=std::string("[Local Override ").append((nFlags&flgLocalOverrideDisabled)?"Disabled] ":"Enabled] ");
		szRet+=std::string("[Window Function ").append((nFlags&flgWindowFunctionDisabled)?"Disabled] ":"Enabled] ");
		szRet+=std::string((nFlags&flgSingleRoomZone)?"[Single":"[Multi").append(" Room Zone] ");
		return szRet;
	}
};

class CEvohomeMsg
{
public:
	enum flagtypes{
		flgid1=1,
		flgid2=flgid1<<1,
		flgid3=flgid2<<1,
		flgpkt=flgid3<<1,
		flgts=flgpkt<<1,
		flgcmd=flgts<<1,//not optional but we can signal if we read it at least
		flgps=flgcmd<<1,//not optional but we can signal if we read it at least
		flgpay=flgps<<1,//not optional but we can signal if we read it at least
		flgvalid=flgid1|flgpkt|flgcmd|flgps|flgpay,
	};
	enum packettype{
		pktunk,
		pktinf,
		pktreq,
		pktrsp,
		pktwrt,
	};
	
	CEvohomeMsg():flags(0),type(pktunk),timestamp(0),command(0),payloadsize(0),readofs(0),enccount(0){}
	CEvohomeMsg(const char * rawmsg):flags(0),type(pktunk),timestamp(0),command(0),payloadsize(0),readofs(0),enccount(0){DecodePacket(rawmsg);}
	CEvohomeMsg(packettype nType, int nAddr, int nCommand):flags(0),type(nType),timestamp(0),command(nCommand),payloadsize(0),readofs(0),enccount(0){SetID(1,nAddr);SetFlag(flgpkt|flgcmd);}
	CEvohomeMsg(packettype nType, int nAddr1, int nAddr2, int nCommand):flags(0),type(nType),timestamp(0),command(nCommand),payloadsize(0),readofs(0),enccount(0){SetID(1,nAddr1);SetID(2,nAddr2);SetFlag(flgpkt|flgcmd);}
	CEvohomeMsg(const CEvohomeMsg& src):readofs(0),enccount(0){*this=src;}
	~CEvohomeMsg(){}
	
	CEvohomeMsg& operator = (const CEvohomeMsg& src)
	{
		flags=src.flags;
		type=src.type;
		for(int i=0;i<3;i++)
			id[i]=src.id[i];//maintain flags
		timestamp=src.timestamp;
		command=src.command;
		payloadsize=src.payloadsize;
		memcpy(payload,src.payload,payloadsize);
		
		return *this;
	}
	
	bool operator == (const CEvohomeMsg& other) const
	{
		//ignore flags may not be set correctly atm
		//ignore timestamp as not sure this would change the intent of the packet if present
		if(type!=other.type)
			return false;
		for(int i=1;i<3;i++)//ignore the source address as the special 18:730 addr used for sending will not match the one read back in
			if(id[i]!=other.id[i])
				return false;
		if(command!=other.command)
			return false;
		if(payloadsize!=other.payloadsize)
			return false;
		if(memcmp(payload,other.payload,payloadsize))
			return false;
		return true;
	}
	
	bool IsValid() const {return ((flags&flgvalid)==flgvalid)&&(flags&(flgid2|flgid3));}
	bool DecodePacket(const char * rawmsg);

	template<typename T> CEvohomeMsg& Add(const T &in){CEvohomeDataType::Add(in,payload,payloadsize);return *this;}
	template<typename T> CEvohomeMsg& Add_if(const T &in, bool p){if(p)Add(in);return *this;}
	template<typename T> CEvohomeMsg& Get(T &out){CEvohomeDataType::Get(out,payload,readofs);return *this;}
	template<typename T> CEvohomeMsg& Get(T &out, int nOfs){SetPos(nOfs);CEvohomeDataType::Get(out,payload,readofs);return *this;}
	template<typename T> CEvohomeMsg& Get_if(T &out, bool p){if(p)Get(out);return *this;}
		
	void SetPos(unsigned char nPos){readofs=nPos;}
	unsigned char GetPos(){return readofs;}
	
	std::string Encode();
	
	void SetFlag(int nFlag){flags|=nFlag;}
	bool GetFlag(int nFlag){return ((flags&nFlag)!=0);}
	
	packettype SetPacketType(const std::string &szPkt)
	{
		type=pktunk;
		for (int i = pktinf; i <= pktwrt; i++)
		{
			if (szPkt == szPacketType[i])
			{
				type=static_cast<packettype>(i);
				break;
			}
		}
		return type;
	}
	
	std::string GetStrID(int idx) const
	{
		if(idx>=3)
			return "";
		return id[idx].GetStrID();
	}
	unsigned int GetID(int idx) const
	{
		if(idx>=3)
			return 0;
		return id[idx];
	}
	void SetID(int idx, int nID)
	{
		if(idx>=3)
			return;
		id[idx]=nID;
		SetFlag(flgid1<<idx);
	}
	bool BadMsg(){return (enccount>30);}
	
	static char const szPacketType[5][8];

	unsigned char flags;
	packettype type;
	CEvohomeID id[3];
	unsigned char timestamp;
	unsigned int command;
	unsigned char payloadsize;
	unsigned char readofs;
	static int const m_nBufSize=256;
	unsigned char payload[m_nBufSize];
	unsigned int enccount;
};

class CEvohome : public AsyncSerial, public CDomoticzHardwareBase
{
public:
	CEvohome(const int ID, const std::string &szSerialPort);
	~CEvohome(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool m_bScriptOnly;
	
	typedef boost::function< bool(CEvohomeMsg &msg) > fnc_evohome_decode;
	
	enum zoneModeType{
		zmAuto,
		zmPerm,
		zmTmp,
		zmWind,//window func
		zmLocal,//set locally
		zmRem,//if set by an remote device
		zmNotSp,//not specified if we just get the set point temp on its own
	};
	
	//Basic evohome controller modes
	enum controllerMode{
		cmEvoAuto,// 0x00
		cmEvoAutoWithEco,//  0x01
		cmEvoAway,//  0x02
		cmEvoDayOff,//  0x03
		cmEvoCustom,//  0x04
		cmEvoHeatingOff,//  0x05
	};
	
	enum controllerModeType{
		cmPerm,
		cmTmp
	};
	
	enum evoCommands{
		cmdSysInfo=0x10e0,
		cmdZoneTemp=0x30C9,
		cmdZoneName=0x0004,
		cmdZoneHeatDemand=0x3150,//Heat demand sent by an individual zone
		cmdZoneInfo=0x000A,
		cmdZoneWindow=0x12B0,//Open window/ventilation zone function
		cmdSetPoint=0x2309,
		cmdSetpointOverride=0x2349,
		cmdDHWState=0x1F41,
		cmdDHWTemp=0x1260,
		cmdControllerMode=0x2E04,
		cmdControllerHeatDemand=0x0008,//Heat demand sent by the controller for CH / DHW / Boiler  (F9/FA/FC)
		cmdActuatorState=0x3EF0,
		cmdActuatorCheck=0x3B00,
		cmdBinding=0x1FC9,
		cmdExternalSensor=0x0002,
		cmdDeviceInfo=0x0418,
		cmdBatteryInfo=0x1060,
		//0x10a0 //DHW settings sent between controller and DHW sensor can also be requested by the gateway <1:DevNo><2(uint16_t):SetPoint><1:Overrun?><2:Differential>
		//0x1F09 //Unknown fixed message FF070D
		//0x0005
		//0x0006
		//0x0404
	};
	
	enum msgUpdate{
		updTemp,
		updSetPoint,
		updOverride,
		updBattery,
	};
	static const uint8_t m_nMaxZones=12;
	
	int GetControllerID();
	int GetGatewayID();
	uint8_t GetZoneCount();
	uint8_t GetControllerMode();
	std::string GetControllerName();
	std::string GetZoneName(uint8_t nZone);
		
	void SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	int Bind(uint8_t nDevNo, unsigned char nDevType);//use CEvohomeID::devType atm but there could be additional specialisations
		
	static const char* GetControllerModeName(uint8_t nControllerMode);
	static const char* GetWebAPIModeName(uint8_t nControllerMode);
	static const char* GetZoneModeName(uint8_t nZoneMode);

	static void LogDate();
	static void Log(bool bDebug, int nLogLevel, const char* format, ... );
	static void Log(const char *szMsg, CEvohomeMsg &msg);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	
	bool OpenSerialDevice();
	void Do_Work();
	void ReadCallback(const char *data, size_t len);
	bool HandleLoopData(const char *data, size_t len);
	int ProcessBuf(char * buf, int size);
	void ProcessMsg(const char * rawmsg);
	bool DecodePayload(CEvohomeMsg &msg);
	void RunScript(const char *pdata, const unsigned char length);
	
	void RegisterDecoder(unsigned int cmd, fnc_evohome_decode fndecoder);
	bool DecodeSetpoint(CEvohomeMsg &msg);
	bool DecodeSetpointOverride(CEvohomeMsg &msg);
	bool DecodeZoneTemp(CEvohomeMsg &msg);
	bool DecodeDHWState(CEvohomeMsg &msg);
	bool DecodeDHWTemp(CEvohomeMsg &msg);
	bool DecodeControllerMode(CEvohomeMsg &msg);
	bool DecodeSysInfo(CEvohomeMsg &msg);
	bool DecodeZoneName(CEvohomeMsg &msg);
	bool DecodeHeatDemand(CEvohomeMsg &msg);
	bool DecodeZoneInfo(CEvohomeMsg &msg);
	bool DecodeBinding(CEvohomeMsg &msg);
	bool DecodeActuatorState(CEvohomeMsg &msg);
	bool DecodeActuatorCheck(CEvohomeMsg &msg);
	bool DecodeZoneWindow(CEvohomeMsg &msg);
	bool DecodeExternalSensor(CEvohomeMsg &msg);
	bool DecodeDeviceInfo(CEvohomeMsg &msg);
	bool DecodeBatteryInfo(CEvohomeMsg &msg);
	bool DumpMessage(CEvohomeMsg &msg);
	
	void AddSendQueue(const CEvohomeMsg &msg);
	void PopSendQueue(const CEvohomeMsg &msg);
	void Do_Send();
	
	void RequestCurrentState();
	void RequestZoneState();
	void RequestZoneNames();
	void RequestControllerMode();
	void RequestSysInfo();
	void RequestDHWState();
	void RequestDHWTemp();
	void RequestZoneInfo(uint8_t nZone);
	void RequestZoneTemp(uint8_t nZone);
	void RequestZoneName(uint8_t nZone);
	void RequestZoneStartupInfo(uint8_t nZone);
	void RequestSetPointOverride(uint8_t nZone);
	void RequestDeviceInfo(uint8_t nAddr);
	
	void SendExternalSensor();
	
	void RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID=0);
	void SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	void UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	void CheckRelayHeatDemand();
	void SendRelayKeepAlive();
	
	void SetControllerID(int nID);
	void SetGatewayID(int nID);
	
	bool SetMaxZoneCount(uint8_t nZoneCount);
	bool SetZoneCount(uint8_t nZoneCount);
	bool SetControllerMode(uint8_t nControllerMode);
	
	void InitControllerName();
	void SetControllerName(std::string szName);

	void InitZoneNames();
	void SetZoneName(uint8_t nZone, std::string szName);
		
	static const char m_szControllerMode[7][20];
	static const char m_szWebAPIMode[7][20];
	static const char m_szZoneMode[7][20];
	
	static const int m_evoToDczControllerMode[8];
	static const int m_evoToDczOverrideMode[5];
	static const uint8_t m_dczToEvoZoneMode[3];
	static const uint8_t m_dczToEvoControllerMode[6];
	
	template <typename RT> RT ConvertMode(RT* pArray, uint8_t nIdx){return pArray[nIdx];}
	
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	
	std::vector <zoneModeType> m_ZoneOverrideLocal;	
	
	static const char m_szNameErr[18];
	static const int m_nBufSize=4096;
	char m_buf[m_nBufSize];
	int m_nBufPtr;
	
	std::map < unsigned int, fnc_evohome_decode > m_Decoders;
	std::list < CEvohomeMsg > m_SendQueue;
	boost::mutex m_mtxSend;
	int m_nSendFail;
	
	uint8_t m_nZoneCount;
	boost::mutex m_mtxZoneCount;
	
	uint8_t m_nControllerMode;
	boost::mutex m_mtxControllerMode;
	
	std::string m_szControllerName;
	boost::mutex m_mtxControllerName;
	
	std::vector <std::string> m_ZoneNames;
	boost::mutex m_mtxZoneName;
	
	unsigned int m_nDevID;//controller ID
	boost::mutex m_mtxControllerID;
	
	unsigned int m_nMyID;//gateway ID
	boost::mutex m_mtxGatewayID;
	
	unsigned int m_nBindID;//device ID of bound device
	unsigned char m_nBindIDType;//what type of device to bind
	boost::mutex m_mtxBindNotify;
	boost::condition_variable m_cndBindNotify;
	
	struct _tRelayCheck
	{
		_tRelayCheck():m_stLastCheck(boost::posix_time::min_date_time),m_nDemand(0){}
		_tRelayCheck(uint8_t demand):m_stLastCheck(boost::posix_time::min_date_time),m_nDemand(demand){}
		_tRelayCheck(boost::system_time st, uint8_t demand):m_stLastCheck(st),m_nDemand(demand){}
		boost::system_time m_stLastCheck;
		uint8_t m_nDemand;
	};
	typedef std::map < uint8_t, _tRelayCheck > tmap_relay_check;
	typedef tmap_relay_check::iterator tmap_relay_check_it;
	typedef tmap_relay_check::value_type tmap_relay_check_pair;
	boost::mutex m_mtxRelayCheck;
	tmap_relay_check m_RelayCheck;
	
	bool m_bStartup[2];
	
	static bool m_bDebug;//Debug mode for extra logging
	static std::ofstream *m_pEvoLog;
};

