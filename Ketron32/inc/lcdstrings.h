/*
 * lcdstrings.h
 *
 * Created: 24.08.2019 18:50:52
 *  Author: Tomek
 */ 


#ifndef LCDSTRINGS_H_
#define LCDSTRINGS_H_

/* string offsets in eeprom */
#define SD_INS		0	// SD card inserted
#define SD_REM		16	// SD card removed
#define MNT_OK		31	// Mount OK
#define MNT_NO		39	// Mount failed
#define UNM_OK		51	// Unmount OK
#define UNM_NO		61	// Unmount failed
#define OPN_NO		75	// Open failed
#define MID_REC		86	// Midi rec.
#define SND_SEL		95	// Sound select
#define OPN_OK		107	// Open OK
#define NO_SND		114	// No sounds

/* string lengths in eeprom */
#define SD_INS_LEN		16	// SD card inserted
#define SD_REM_LEN		15	// SD card removed
#define MNT_OK_LEN		8	// Mount OK
#define MNT_NO_LEN		12	// Mount failed
#define UNM_OK_LEN		10	// Unmount OK
#define UNM_NO_LEN		14	// Unmount failed
#define OPN_NO_LEN		11	// Open failed
#define MIDI_REC_LEN	9	// Midi rec.
#define SND_SEL_LEN		12	// Sound select
#define OPN_OK_LEN		7	// Open OK
#define NO_SND_LEN		9	// No sounds

#endif /* LCDSTRINGS_H_ */