// code/mechtypefit.cpp

#include "mechtypefit.h"

#include "csvfile.h"

#include <cstdio>

long chassisMoveType(CSVFile& csv)
{
	long moveType = 0;
	if (csv.readLong(1, 3, moveType) != NO_ERR)
		return 0;
	return moveType == 1 ? 1 : 0;
}

void buildMechTypeFit(char* fit, size_t fitSize, long objectTypeNum, const char* name,
					  long tonnage, long moveType)
{
	long destructDamage = 65, explosionDamage = 30;
	if (tonnage < 40) {
		destructDamage = 35;
		explosionDamage = 15;
	}
	else if (tonnage < 60) {
		destructDamage = 45;
		explosionDamage = 20;
	}
	else if (tonnage < 80) {
		destructDamage = 55;
		explosionDamage = 25;
	}
	snprintf(fit, fitSize,
		"FITini\r\n\r\n"
		"[ObjectClass]\r\nl ObjectTypeNum = %ld\r\n\r\n"
		"[ObjectType]\r\nst Name = \"%s\"\r\nl ExplosionObject = 41\r\nl DestroyedObject = 101\r\n"
		"f ExtentRadius = 12\r\nst AppearanceName = \"%s\"\r\n\r\n"
		"[MechProfile]\r\nst ProfileName = \"%s\"\r\nl MoveType = %ld\r\nf destructdamage = %ld\r\n"
		"f explosiondamage = %ld\r\nf explosionradius = 80\r\n\r\nFITend\r\n",
		objectTypeNum, name, name, name, moveType, destructDamage, explosionDamage);
}
