//---------------------------------------------------------------------------
// movobj.cpp: MechCommander Omnitech object types past object2.pak.
//---------------------------------------------------------------------------

#include"mclib.h"
#include"dobjtype.h"
#include"movobj.h"

static FitIniFilePtr MovObjFile = NULL;
static bool MovObjOpened = false;

static FitIniFilePtr movObjFile (void) {

	if (!MovObjOpened) {
		MovObjOpened = true;
		FullPathFileName movObjName;
		movObjName.init(objectPath, "movobj", ".fit");
		if (fileExists(movObjName)) {
			MovObjFile = new FitIniFile;
			if (MovObjFile->open(movObjName) != NO_ERR) {
				delete MovObjFile;
				MovObjFile = NULL;
			}
		}
	}
	return(MovObjFile);
}

void closeMovObjFile (void) {

	if (MovObjFile) {
		MovObjFile->close();
		delete MovObjFile;
		MovObjFile = NULL;
	}
	MovObjOpened = false;
}

bool findMovObjType (long objTypeNum, long& typeClass, char* name, long nameSize) {

	FitIniFilePtr movObj = movObjFile();
	if (!movObj)
		return(false);
	char block[32];
	sprintf(block, "%ld", objTypeNum);
	if ((movObj->seekBlock(block) != NO_ERR) ||
		(movObj->readIdLong("ObjectTypeNum", typeClass) != NO_ERR) ||
		(movObj->readIdString("CSVname", name, nameSize - 1) != NO_ERR) || !name[0])
		return(false);
	S_strlwr(name);
	return(true);
}

// Mission and logistics used to seek the pak directly, so Omnitech vehicles
// spawned without their profile and were missing from logistics (and each
// seek logged a STOP).
long openTypeProfile (long objTypeNum, FitIniFile& profile, PacketFile* pak) {

	if ((objTypeNum >= 0) && (objTypeNum < pak->getNumPackets())) {
		if (pak->seekPacket(objTypeNum) != NO_ERR)
			return(-1);
		return(profile.open(pak, pak->getPacketSize()));
	}
	long typeClass = 0;
	char name[64];
	if (!findMovObjType(objTypeNum, typeClass, name, sizeof(name)) || (typeClass == BATTLEMECH_TYPE))
		return(-1);
	FullPathFileName fitName;
	fitName.init(objectPath, name, ".fit");
	return(profile.open((const char*)fitName));
}
