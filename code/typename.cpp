// code/typename.cpp

#include "typename.h"

#include "inifile.h"
#include "utils/pe_strings.h"

#include <cctype>
#include <cstring>

void displayTypeName(char* name)
{
    const size_t len = strlen(name);
    if (len > 4 && name[len - 4] == '.' && tolower((unsigned char)name[len - 3]) == 'f'
            && tolower((unsigned char)name[len - 2]) == 'i'
            && tolower((unsigned char)name[len - 1]) == 't')
        name[len - 4] = '\0';
}

long typeNameId(FitIniFile& typeFile, long nameId)
{
    if (nameId != 0)
        return nameId;

    char typeName[256];
    if (typeFile.seekBlock("ObjectType") != NO_ERR
            || typeFile.readIdString("Name", typeName, 255) != NO_ERR)
        return nameId;
    displayTypeName(typeName);

    char appearance[256];
    if (typeFile.readIdString("AppearanceName", appearance, 255) != NO_ERR)
        appearance[0] = '\0';

    const uint32_t runtimeId = addRuntimeString(typeName, appearance);
    return runtimeId ? (long)runtimeId : nameId;
}
