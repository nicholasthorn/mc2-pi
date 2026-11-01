//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
//***************************************************************************
//
//								ERR.H
//
//***************************************************************************

#ifndef ERR_H
#define	ERR_H

//***************************************************************************

// Logs the error and ends the game (gos_FatalExit); never returns.
[[noreturn]] void Fatal (long errCode, const char* errMessage);

void Assert (bool test, long errCode, const char* errMessage);

//***************************************************************************

#endif
