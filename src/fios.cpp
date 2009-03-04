/* $Id$ */

/** @file fios.cpp
 * This file contains functions for building file lists for the save/load dialogs.
 */

#include "stdafx.h"
#include "openttd.h"
#include "fios.h"
#include "fileio_func.h"
#include "string_func.h"
#include <sys/stat.h>

#ifdef WIN32
# include <tchar.h>
# ifndef UNICODE
#  include <io.h>
# endif
# define access _taccess
# define unlink _tunlink
#else
# include <unistd.h>
#endif /* WIN32 */

#include "table/strings.h"

/* Variables to display file lists */
SmallVector<FiosItem, 32> _fios_items;
static char *_fios_path;
SmallFiosItem _file_to_saveload;

/* OS-specific functions are taken from their respective files (win32/unix/os2 .c) */
extern bool FiosIsRoot(const char *path);
extern bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb);
extern bool FiosIsHiddenFile(const struct dirent *ent);
extern void FiosGetDrives();
extern bool FiosGetDiskFreeSpace(const char *path, uint64 *tot);

/* get the name of an oldstyle savegame */
extern void GetOldSaveGameName(const char *path, const char *file, char *title, const char *last);

/**
 * Compare two FiosItem's. Used with qsort when sorting the file list.
 * @param a A pointer to the first FiosItem to compare.
 * @param b A pointer to the second FiosItem to compare.
 * @return -1, 0 or 1, depending on how the two items should be sorted.
 */
int CDECL compare_FiosItems(const void *a, const void *b)
{
	const FiosItem *da = (const FiosItem *)a;
	const FiosItem *db = (const FiosItem *)b;
	int r;

	if (_savegame_sort_order & SORT_BY_NAME) {
		r = strcasecmp(da->title, db->title);
	} else {
		r = da->mtime < db->mtime ? -1 : 1;
	}

	if (_savegame_sort_order & SORT_DESCENDING) r = -r;
	return r;
}

/** Clear the list */
void FiosFreeSavegameList()
{
	_fios_items.Clear();
	_fios_items.Compact();
};

/**
 * Get descriptive texts. Returns the path and free space
 * left on the device
 * @param path string describing the path
 * @param total_free total free space in megabytes, optional (can be NULL)
 * @return StringID describing the path (free space or failure)
 */
StringID FiosGetDescText(const char **path, uint64 *total_free)
{
	*path = _fios_path;
	return FiosGetDiskFreeSpace(*path, total_free) ? STR_4005_BYTES_FREE : STR_4006_UNABLE_TO_READ_DRIVE;
}

/* Browse to a new path based on the passed FiosItem struct
 * @param *item FiosItem object telling us what to do
 * @return a string if we have given a file as a target, otherwise NULL */
const char *FiosBrowseTo(const FiosItem *item)
{
	char *path = _fios_path;

	switch (item->type) {
		case FIOS_TYPE_DRIVE:
#if defined(WINCE)
			snprintf(path, MAX_PATH, PATHSEP "");
#elif defined(WIN32) || defined(__OS2__)
			snprintf(path, MAX_PATH, "%c:" PATHSEP, item->title[0]);
#endif
		/* Fallthrough */
		case FIOS_TYPE_INVALID:
			break;

		case FIOS_TYPE_PARENT: {
			/* Check for possible NULL ptr (not required for UNIXes, but AmigaOS-alikes) */
			char *s = strrchr(path, PATHSEPCHAR);
			if (s != NULL && s != path) {
				s[0] = '\0'; // Remove last path separator character, so we can go up one level.
			}
			s = strrchr(path, PATHSEPCHAR);
			if (s != NULL) s[1] = '\0'; // go up a directory
#if defined(__MORPHOS__) || defined(__AMIGAOS__)
			/* On MorphOS or AmigaOS paths look like: "Volume:directory/subdirectory" */
			else if ((s = strrchr(path, ':')) != NULL) s[1] = '\0';
#endif
			break;
		}

		case FIOS_TYPE_DIR:
			strcat(path, item->name);
			strcat(path, PATHSEP);
			break;

		case FIOS_TYPE_DIRECT:
			snprintf(path, MAX_PATH, "%s", item->name);
			break;

		case FIOS_TYPE_FILE:
		case FIOS_TYPE_OLDFILE:
		case FIOS_TYPE_SCENARIO:
		case FIOS_TYPE_OLD_SCENARIO:
		case FIOS_TYPE_PNG:
		case FIOS_TYPE_BMP:
		{
			static char str_buffr[512];
			snprintf(str_buffr, lengthof(str_buffr), "%s%s", path, item->name);
			return str_buffr;
		}
	}

	return NULL;
}

void FiosMakeSavegameName(char *buf, const char *name, size_t size)
{
	const char *extension, *period;

	extension = (_game_mode == GM_EDITOR) ? ".scn" : ".sav";

	/* Don't append the extension if it is already there */
	period = strrchr(name, '.');
	if (period != NULL && strcasecmp(period, extension) == 0) extension = "";
#if  defined(__MORPHOS__) || defined(__AMIGAOS__)
	if (_fios_path != NULL) {
		unsigned char sepchar = _fios_path[(strlen(_fios_path) - 1)];

		if (sepchar != ':' && sepchar != '/') {
			snprintf(buf, size, "%s" PATHSEP "%s%s", _fios_path, name, extension);
		} else {
			snprintf(buf, size, "%s%s%s", _fios_path, name, extension);
		}
	} else {
		snprintf(buf, size, "%s%s", name, extension);
	}
#else
	snprintf(buf, size, "%s" PATHSEP "%s%s", _fios_path, name, extension);
#endif
}

#if defined(WIN32)
# define unlink _tunlink
#endif

bool FiosDelete(const char *name)
{
	char filename[512];

	FiosMakeSavegameName(filename, name, lengthof(filename));
	return unlink(OTTD2FS(filename)) == 0;
}

bool FileExists(const char *filename)
{
#if defined(WINCE)
	/* There is always one platform that doesn't support basic commands... */
	HANDLE hand = CreateFile(OTTD2FS(filename), 0, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hand == INVALID_HANDLE_VALUE) return 1;
	CloseHandle(hand);
	return 0;
#else
	return access(OTTD2FS(filename), 0) == 0;
#endif
}

typedef FiosType fios_getlist_callback_proc(SaveLoadDialogMode mode, const char *filename, const char *ext, char *title, const char *last);

/** Create a list of the files in a directory, according to some arbitrary rule.
 *  @param mode The mode we are in. Some modes don't allow 'parent'.
 *  @param callback_proc The function that is called where you need to do the filtering.
 *  @return Return the list of files. */
static FiosItem *FiosGetFileList(SaveLoadDialogMode mode, fios_getlist_callback_proc *callback_proc)
{
	struct stat sb;
	struct dirent *dirent;
	DIR *dir;
	FiosItem *fios;
	int sort_start;
	char d_name[sizeof(fios->name)];

	_fios_items.Clear();

	/* A parent directory link exists if we are not in the root directory */
	if (!FiosIsRoot(_fios_path) && mode != SLD_NEW_GAME) {
		fios = _fios_items.Append();
		fios->type = FIOS_TYPE_PARENT;
		fios->mtime = 0;
		strecpy(fios->name, "..", lastof(fios->name));
		strecpy(fios->title, ".. (Parent directory)", lastof(fios->title));
	}

	/* Show subdirectories */
	if (mode != SLD_NEW_GAME && (dir = ttd_opendir(_fios_path)) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			strecpy(d_name, FS2OTTD(dirent->d_name), lastof(d_name));

			/* found file must be directory, but not '.' or '..' */
			if (FiosIsValidFile(_fios_path, dirent, &sb) && S_ISDIR(sb.st_mode) &&
					(!FiosIsHiddenFile(dirent) || strncasecmp(d_name, PERSONAL_DIR, strlen(d_name)) == 0) &&
					strcmp(d_name, ".") != 0 && strcmp(d_name, "..") != 0) {
				fios = _fios_items.Append();
				fios->type = FIOS_TYPE_DIR;
				fios->mtime = 0;
				strecpy(fios->name, d_name, lastof(fios->name));
				snprintf(fios->title, lengthof(fios->title), "%s" PATHSEP " (Directory)", d_name);
				str_validate(fios->title);
			}
		}
		closedir(dir);
	}

	/* Sort the subdirs always by name, ascending, remember user-sorting order */
	{
		byte order = _savegame_sort_order;
		_savegame_sort_order = SORT_BY_NAME | SORT_ASCENDING;
		qsort(_fios_items.Begin(), _fios_items.Length(), sizeof(FiosItem), compare_FiosItems);
		_savegame_sort_order = order;
	}

	/* This is where to start sorting for the filenames */
	sort_start = _fios_items.Length();

	/* Show files */
	dir = ttd_opendir(_fios_path);
	if (dir != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			char fios_title[64];
			char *t;
			strecpy(d_name, FS2OTTD(dirent->d_name), lastof(d_name));

			if (!FiosIsValidFile(_fios_path, dirent, &sb) || !S_ISREG(sb.st_mode) || FiosIsHiddenFile(dirent)) continue;

			/* File has no extension, skip it */
			if ((t = strrchr(d_name, '.')) == NULL) continue;
			fios_title[0] = '\0'; // reset the title;

			FiosType type = callback_proc(mode, d_name, t, fios_title, lastof(fios_title));
			if (type != FIOS_TYPE_INVALID) {
				fios = _fios_items.Append();
				fios->mtime = sb.st_mtime;
				fios->type = type;
				strecpy(fios->name, d_name, lastof(fios->name));

				/* Some callbacks want to lookup the title of the file. Allow that.
				 * If we just copy the title from the filename, strip the extension */
				t = (fios_title[0] == '\0') ? *t = '\0', d_name : fios_title;
				strecpy(fios->title, t, lastof(fios->title));
				str_validate(fios->title);
			}
		}
		closedir(dir);
	}

	qsort(_fios_items.Get(sort_start), _fios_items.Length() - sort_start, sizeof(FiosItem), compare_FiosItems);

	/* Show drives */
	if (mode != SLD_NEW_GAME) FiosGetDrives();

	_fios_items.Compact();

	return _fios_items.Begin();
}

/**
 * Callback for FiosGetFileList. It tells if a file is a savegame or not.
 * @param mode Save/load mode.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file; NULL to skip the lookup
 * @param last Last available byte in buffer (to prevent buffer overflows); not used when title == NULL
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a savegame
 * @see FiosGetFileList
 * @see FiosGetSavegameList
 */
FiosType FiosGetSavegameListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show savegame files
	 * .SAV OpenTTD saved game
	 * .SS1 Transport Tycoon Deluxe preset game
	 * .SV1 Transport Tycoon Deluxe (Patch) saved game
	 * .SV2 Transport Tycoon Deluxe (Patch) saved 2-player game */
	if (strcasecmp(ext, ".sav") == 0) return FIOS_TYPE_FILE;

	if (mode == SLD_LOAD_GAME || mode == SLD_LOAD_SCENARIO) {
		if (strcasecmp(ext, ".ss1") == 0 || strcasecmp(ext, ".sv1") == 0 ||
				strcasecmp(ext, ".sv2") == 0) {
			if (title != NULL) GetOldSaveGameName(_fios_path, file, title, last);
			return FIOS_TYPE_OLDFILE;
		}
	}

	return FIOS_TYPE_INVALID;
}

/**
 * Get a list of savegames.
 * @param mode Save/load mode.
 * @return A pointer to an array of FiosItem representing all the files to be shown in the save/load dialog.
 * @see FiosGetFileList
 */
void FiosGetSavegameList(SaveLoadDialogMode mode)
{
	static char *fios_save_path = NULL;

	if (fios_save_path == NULL) {
		fios_save_path = MallocT<char>(MAX_PATH);
		FioGetDirectory(fios_save_path, MAX_PATH, SAVE_DIR);
	}

	_fios_path = fios_save_path;

	FiosGetFileList(mode, &FiosGetSavegameListCallback);
}

/**
 * Callback for FiosGetFileList. It tells if a file is a scenario or not.
 * @param mode Save/load mode.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file
 * @param last Last available byte in buffer (to prevent buffer overflows)
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a scenario
 * @see FiosGetFileList
 * @see FiosGetScenarioList
 */
static FiosType FiosGetScenarioListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show scenario files
	 * .SCN OpenTTD style scenario file
	 * .SV0 Transport Tycoon Deluxe (Patch) scenario
	 * .SS0 Transport Tycoon Deluxe preset scenario */
	if (strcasecmp(ext, ".scn") == 0) return FIOS_TYPE_SCENARIO;

	if (mode == SLD_LOAD_GAME || mode == SLD_LOAD_SCENARIO || mode == SLD_NEW_GAME) {
		if (strcasecmp(ext, ".sv0") == 0 || strcasecmp(ext, ".ss0") == 0 ) {
			GetOldSaveGameName(_fios_path, file, title, last);
			return FIOS_TYPE_OLD_SCENARIO;
		}
	}

	return FIOS_TYPE_INVALID;
}

/**
 * Get a list of scenarios.
 * @param mode Save/load mode.
 * @return A pointer to an array of FiosItem representing all the files to be shown in the save/load dialog.
 * @see FiosGetFileList
 */
void FiosGetScenarioList(SaveLoadDialogMode mode)
{
	static char *fios_scn_path = NULL;

	/* Copy the default path on first run or on 'New Game' */
	if (fios_scn_path == NULL) {
		fios_scn_path = MallocT<char>(MAX_PATH);
		FioGetDirectory(fios_scn_path, MAX_PATH, SCENARIO_DIR);
	}

	_fios_path = fios_scn_path;

	FiosGetFileList(mode, &FiosGetScenarioListCallback);
}

static FiosType FiosGetHeightmapListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show heightmap files
	 * .PNG PNG Based heightmap files
	 * .BMP BMP Based heightmap files
	 */

#ifdef WITH_PNG
	if (strcasecmp(ext, ".png") == 0) return FIOS_TYPE_PNG;
#endif /* WITH_PNG */

	if (strcasecmp(ext, ".bmp") == 0) return FIOS_TYPE_BMP;

	return FIOS_TYPE_INVALID;
}

/* Get a list of Heightmaps */
void FiosGetHeightmapList(SaveLoadDialogMode mode)
{
	static char *fios_hmap_path = NULL;

	if (fios_hmap_path == NULL) {
		fios_hmap_path = MallocT<char>(MAX_PATH);
		FioGetDirectory(fios_hmap_path, MAX_PATH, HEIGHTMAP_DIR);
	}

	_fios_path = fios_hmap_path;

	FiosGetFileList(mode, &FiosGetHeightmapListCallback);
}
