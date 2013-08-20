/*
	Copyright (C) 2010-2013 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SLOT1_H__
#define __SLOT1_H__

#include <string>
#include "common.h"
#include "types.h"
#include "debug.h"

class EMUFILE;

class Slot1Info
{
public:
	virtual const char* name() const = 0;
	virtual const char* descr()const  = 0;
};

class Slot1InfoSimple : public Slot1Info
{
public:
	Slot1InfoSimple(const char* _name, const char* _descr)
		: mName(_name)
		, mDescr(_descr)
	{
	}
	virtual const char* name() const { return mName; }
	virtual const char* descr() const { return mDescr; }
private:
	const char* mName, *mDescr;
};

class ISlot1Interface
{
public:
	//called to get info about device (description)
	virtual Slot1Info const* info() = 0;

	//called once when the emulator starts up, or when the device springs into existence
	virtual bool init() { return true; }
	
	//called when the emulator connects the device
	virtual void connect() { }

	//called when the emulator disconnects the device
	virtual void disconnect() { }
	
	//called when the emulator shuts down, or when the device disappears from existence
	virtual void shutdown() { }

	//called when the emulator write to the slot (TODO - refactors necessary)
	void write08(u8 PROCNUM, u32 adr, u8 val) { printf("WARNING: 8bit write to slot-1\n"); }
	void write16(u8 PROCNUM, u32 adr, u16 val)  { printf("WARNING: 16bit write to slot-1\n"); }
	virtual void write32(u8 PROCNUM, u32 adr, u32 val) { }

	//called when the emulator reads from the slot (TODO - refactors necessary)
	u8  read08(u8 PROCNUM, u32 adr) { printf("WARNING: 8bit read from slot-1\n"); return 0xFF; }
	u16 read16(u8 PROCNUM, u32 adr) { printf("WARNING: 16bit read from slot-1\n"); return 0xFFFF; }
	virtual u32 read32(u8 PROCNUM, u32 adr) { return 0xFFFFFFFF; }

	//transfers a byte to the slot-1 device via auxspi, and returns the incoming byte
	//cpu is provided for diagnostic purposes only.. the slot-1 device wouldn't know which CPU it is.
	virtual u8 auxspi_transaction(int PROCNUM, u8 value) { return 0x00; }

	//called when the auxspi burst is ended (SPI chipselect in is going low)
	virtual void auxspi_reset(int PROCNUM) {}
}; 

typedef ISlot1Interface* TISlot1InterfaceConstructor();

enum NDS_SLOT1_TYPE
{
	NDS_SLOT1_NONE,
	NDS_SLOT1_RETAIL_AUTO, //autodetect which kind of retail card to use 
	NDS_SLOT1_R4, //R4 flash card
	NDS_SLOT1_RETAIL_NAND, //Made in Ore/WarioWare D.I.Y.
	NDS_SLOT1_RETAIL_MCROM, //a standard MC (eeprom, flash, fram) -bearing retail card. Also supports motion, for now, because that's the way we originally coded it
	NDS_SLOT1_COUNT		//use to count addons - MUST BE LAST!!!
};

extern ISlot1Interface* slot1_device;						//the current slot1 device instance
extern ISlot1Interface* slot1_List[NDS_SLOT1_COUNT];

void slot1_Init();
bool slot1_Connect();
void slot1_Disconnect();
void slot1_Shutdown();

//just disconnects and reconnects the device. ideally, the disconnection and connection would be called with sensible timing
void slot1_Reset();

//change the current device
bool slot1_Change(NDS_SLOT1_TYPE type);

//check on the current device
NDS_SLOT1_TYPE slot1_GetCurrentType();

void slot1_SetFatDir(const std::string& dir);
std::string slot1_GetFatDir();
EMUFILE* slot1_GetFatImage();


#endif //__SLOT1_H__
