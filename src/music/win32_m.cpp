/* $Id$ */

#include "../stdafx.h"
#include "win32_m.h"
#include <windows.h>
#include <mmsystem.h>

static struct {
	bool stop_song;
	bool terminate;
	bool playing;
	int new_vol;
	HANDLE wait_obj;
	HANDLE thread;
	UINT_PTR devid;
	char start_song[MAX_PATH];
} _midi;

static FMusicDriver_Win32 iFMusicDriver_Win32;

void MusicDriver_Win32::PlaySong(const char *filename)
{
	strcpy(_midi.start_song, filename);
	_midi.playing = true;
	_midi.stop_song = false;
	SetEvent(_midi.wait_obj);
}

void MusicDriver_Win32::StopSong()
{
	if (_midi.playing) {
		_midi.stop_song = true;
		_midi.start_song[0] = '\0';
		SetEvent(_midi.wait_obj);
	}
}

bool MusicDriver_Win32::IsSongPlaying()
{
	return _midi.playing;
}

void MusicDriver_Win32::SetVolume(byte vol)
{
	_midi.new_vol = vol;
	SetEvent(_midi.wait_obj);
}

static MCIERROR CDECL MidiSendCommand(const char* cmd, ...)
{
	va_list va;
	char buf[512];

	va_start(va, cmd);
	vsnprintf(buf, lengthof(buf), cmd, va);
	va_end(va);
	return mciSendStringA(buf, NULL, 0, 0);
}

static bool MidiIntPlaySong(const char *filename)
{
	MidiSendCommand("close all");
	if (MidiSendCommand("open \"%s\" type sequencer alias song", filename) != 0) return false;

	return MidiSendCommand("play song from 0") == 0;
}

static void MidiIntStopSong()
{
	MidiSendCommand("close all");
}

static void MidiIntSetVolume(int vol)
{
	DWORD v = (vol * 65535 / 127);
	midiOutSetVolume((HMIDIOUT)_midi.devid, v + (v << 16));
}

static bool MidiIntIsSongPlaying()
{
	char buf[16];
	mciSendStringA("status song mode", buf, sizeof(buf), 0);
	return strcmp(buf, "playing") == 0 || strcmp(buf, "seeking") == 0;
}

static DWORD WINAPI MidiThread(LPVOID arg)
{
	do {
		char *s;
		int vol;

		vol = _midi.new_vol;
		if (vol != -1) {
			_midi.new_vol = -1;
			MidiIntSetVolume(vol);
		}

		s = _midi.start_song;
		if (s[0] != '\0') {
			_midi.playing = MidiIntPlaySong(s);
			s[0] = '\0';

			// Delay somewhat in case we don't manage to play.
			if (!_midi.playing) WaitForMultipleObjects(1, &_midi.wait_obj, FALSE, 5000);
		}

		if (_midi.stop_song && _midi.playing) {
			_midi.stop_song = false;
			_midi.playing = false;
			MidiIntStopSong();
		}

		if (_midi.playing && !MidiIntIsSongPlaying()) _midi.playing = false;

		WaitForMultipleObjects(1, &_midi.wait_obj, FALSE, 1000);
	} while (!_midi.terminate);

	MidiIntStopSong();
	return 0;
}

const char *MusicDriver_Win32::Start(const char * const *parm)
{
	MIDIOUTCAPS midicaps;
	UINT nbdev;
	UINT_PTR dev;
	char buf[16];

	mciSendStringA("capability sequencer has audio", buf, lengthof(buf), 0);
	if (strcmp(buf, "true") != 0) return "MCI sequencer can't play audio";

	memset(&_midi, 0, sizeof(_midi));
	_midi.new_vol = -1;

	/* Get midi device */
	_midi.devid = MIDI_MAPPER;
	for (dev = 0, nbdev = midiOutGetNumDevs(); dev < nbdev; dev++) {
		if (midiOutGetDevCaps(dev, &midicaps, sizeof(midicaps)) == 0 && (midicaps.dwSupport & MIDICAPS_VOLUME)) {
			_midi.devid = dev;
			break;
		}
	}

	if (NULL == (_midi.wait_obj = CreateEvent(NULL, FALSE, FALSE, NULL))) return "Failed to create event";

	/* The lpThreadId parameter of CreateThread (the last parameter)
	 * may NOT be NULL on Windows 95, 98 and ME. */
	DWORD threadId;
	if (NULL == (_midi.thread = CreateThread(NULL, 8192, MidiThread, 0, 0, &threadId))) return "Failed to create thread";

	return NULL;
}

void MusicDriver_Win32::Stop()
{
	_midi.terminate = true;
	SetEvent(_midi.wait_obj);
	WaitForMultipleObjects(1, &_midi.thread, true, INFINITE);
	CloseHandle(_midi.wait_obj);
	CloseHandle(_midi.thread);
}
