/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#ifndef MODULE_SETTINGS
#define MODULE_SETTINGS

extern const char *tcSettingsMenuEntry(void); /* name of the entry in the main menu */
extern void tcSettingsMenu(void);
extern void tcSaveSettings(void);
extern unsigned char tcChooseGameMode(void); /* new game: original or Game+, 0 = cancelled */

#endif
