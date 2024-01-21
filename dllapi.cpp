// vi: set ts=4 sw=4 :
// vim: set tw=75 :

/*
 * Copyright (c) 2001-2003 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifdef _MSC_VER // MSVC
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <stdarg.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <extdll.h>

#include <dllapi.h>
#include <meta_api.h>

extern char gGamedir[];

#define MAX_PRECACHE_COUNT 512
#define MAX_PATH_LEN 256

typedef enum {
	T_GENERIC,
	T_MODEL,
	T_SOUND
} precache_type;

typedef struct {
	char path[MAX_PATH_LEN];
	precache_type type;
} precache_entry;

precache_entry precache_list[MAX_PRECACHE_COUNT];
unsigned short int precache_count = 0;

void replace_path_sep(char* path)
{
#ifdef WIN32
	char *p = path;
	while (*p) {
		if (*p == '/') *p = '\\';
		p++;
	}
#endif
}

bool file_exists(const char* path)
{
	char fullpath[MAX_PATH_LEN];
	strncpy(fullpath, &gGamedir[0], sizeof(fullpath)-1);
	fullpath[sizeof(fullpath)-1] = 0;

	char *p = fullpath;
	char *base = fullpath;
	while (*p) {
		if (*p == '/' || *p == '\\') base = p+1;
		p++;
	}
	*p++ = '/';
	strncpy(p, path, MAX_PATH_LEN-(p-fullpath));
	fullpath[sizeof(fullpath)-1] = 0;

	replace_path_sep(fullpath);
	if (access(fullpath, 4) == 0) return true;

	snprintf(base, MAX_PATH_LEN-(base-fullpath), "valve/%s", path);
	fullpath[sizeof(fullpath)-1] = 0;

	replace_path_sep(fullpath);
	return access(fullpath, 4) == 0;
}

void readConfig() {
	char path[MAX_PATH_LEN];
	precache_count = 0;
	snprintf(path, sizeof(path)-1, "%s/addons/precache/precache.cfg", &gGamedir[0]);
	path[sizeof(path)-1] = 0;
	replace_path_sep(path);
	FILE *fp = fopen(path, "r");
	if (fp) {
		while (fgets(path, sizeof(path), fp) && precache_count < MAX_PRECACHE_COUNT) {
			if (path[0] == '\r' || path[0] == '\n' || path[0] == ';' || path[0] == '#' || (path[0] == '/' && path[1] == '/')) continue;
			char *ext = 0;
			char *p = path;
			while (*p) {
				if (*p == '\r' || *p == '\n') {
					*p = 0;
					break;
				} else if (*p == '.') {
					ext = p;
				}
				p++;
			}
			if (ext && file_exists(path)) {
				strcpy(precache_list[precache_count].path, path);
				if ((ext[1] == 'm' || ext[1] == 'M') &&
					  (ext[2] == 'd' || ext[2] == 'D') &&
					  (ext[3] == 'l' || ext[3] == 'L')) {
					precache_list[precache_count].type = T_MODEL;
				} else if ((ext[1] == 'w' || ext[1] == 'W') &&
					(ext[2] == 'a' || ext[2] == 'A') &&
					(ext[3] == 'v' || ext[3] == 'V')) {
					precache_list[precache_count].type = T_SOUND;
				} else {
					precache_list[precache_count].type = T_GENERIC;
				}
				precache_count++;
			} else {
				LOG_ERROR(PLID, "Could not access entry, ignoring: \"%s\"", path);
			}
		}
		fclose(fp);
	} else {
		LOG_ERROR(PLID, "Could not open configuration file: \"%s\"", path);
	}
}

void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
	readConfig();
	LOG_MESSAGE(PLID, "Precaching %hu items", precache_count);
	for (unsigned int i = 0; i < precache_count; i++) {
		LOG_MESSAGE(PLID, "Precaching item %u: \"%s\" (%s)", i+1, precache_list[i].path,
			((precache_list[i].type == T_MODEL) ? "model" : ((precache_list[i].type == T_SOUND) ? "sound" : "generic")));
		switch (precache_list[i].type) {
		  case T_MODEL:
			  g_engfuncs.pfnPrecacheModel(precache_list[i].path);
			  break;
		  case T_SOUND:
			  g_engfuncs.pfnPrecacheSound(precache_list[i].path);
			  break;
		  case T_GENERIC:
		  default:
			  g_engfuncs.pfnPrecacheGeneric(precache_list[i].path);
			  break;
		}
	}
	RETURN_META(MRES_HANDLED);
}


static DLL_FUNCTIONS gFunctionTable =
{
	NULL,					// pfnGameInit
	NULL,					// pfnSpawn
	NULL,					// pfnThink
	NULL,					// pfnUse
	NULL,					// pfnTouch
	NULL,					// pfnBlocked
	NULL,					// pfnKeyValue
	NULL,					// pfnSave
	NULL,					// pfnRestore
	NULL,					// pfnSetAbsBox

	NULL,					// pfnSaveWriteFields
	NULL,					// pfnSaveReadFields

	NULL,					// pfnSaveGlobalState
	NULL,					// pfnRestoreGlobalState
	NULL,					// pfnResetGlobalState

	NULL,					// pfnClientConnect
	NULL,					// pfnClientDisconnect
	NULL,					// pfnClientKill
	NULL,					// pfnClientPutInServer
	NULL,					// pfnClientCommand
	NULL,					// pfnClientUserInfoChanged
	ServerActivate,				// pfnServerActivate
	NULL,					// pfnServerDeactivate

	NULL,					// pfnPlayerPreThink
	NULL,					// pfnPlayerPostThink

	NULL,					// pfnStartFrame
	NULL,					// pfnParmsNewLevel
	NULL,					// pfnParmsChangeLevel

	NULL,					// pfnGetGameDescription
	NULL,					// pfnPlayerCustomization

	NULL,					// pfnSpectatorConnect
	NULL,					// pfnSpectatorDisconnect
	NULL,					// pfnSpectatorThink

	NULL,					// pfnSys_Error

	NULL,					// pfnPM_Move
	NULL,					// pfnPM_Init
	NULL,					// pfnPM_FindTextureType

	NULL,					// pfnSetupVisibility
	NULL,					// pfnUpdateClientData
	NULL,					// pfnAddToFullPack
	NULL,					// pfnCreateBaseline
	NULL,					// pfnRegisterEncoders
	NULL,					// pfnGetWeaponData
	NULL,					// pfnCmdStart
	NULL,					// pfnCmdEnd
	NULL,					// pfnConnectionlessPacket
	NULL,					// pfnGetHullBounds
	NULL,					// pfnCreateInstancedBaselines
	NULL,					// pfnInconsistentFile
	NULL,					// pfnAllowLagCompensation
};

C_DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable,
		int *interfaceVersion)
{
	if(!pFunctionTable) {
		//UTIL_LogPrintf("GetEntityAPI2 called with null pFunctionTable");
		return(FALSE);
	}
	else if(*interfaceVersion != INTERFACE_VERSION) {
		//UTIL_LogPrintf("GetEntityAPI2 version mismatch; requested=%d ours=%d", *interfaceVersion, INTERFACE_VERSION);
		//! Tell metamod what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return(FALSE);
	}
	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return(TRUE);
}

#ifdef WIN32
extern "C" int WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved) {
	return 1;
}
#endif
