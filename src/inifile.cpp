/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "inifile.h"
#include "common.h"

#include <stdio.h>
#include <fstream.h>
#include <string.h>

char const * const
    INIFile::ini_filename = "PFWALLCL.INI";

INIFile::INIFile(
        Timer::DaytimeHHMM const * const pon_dayt_p,
        Timer::DaytimeHHMM const * const poff_dayt_p,
        Timer::DaytimeHHMM const * const kbhit_poff_delay_dayt_p) :
        pon_dayt_p(pon_dayt_p),
        poff_dayt_p(poff_dayt_p),
        kbhit_poff_delay_dayt_p(kbhit_poff_delay_dayt_p)
{
#ifdef NTVDM
        cout
            << "INIFile: Power on time [HH:MM]: "
            << *pon_dayt_p
            << "\n"
            << "INIFile: Power off time [HH:MM]: "
            << *poff_dayt_p
            << "\n"
            << "INIFile: Power off delay on kbhit [HH:MM]: "
            << *kbhit_poff_delay_dayt_p
            << "\n";
#endif
}

INIFile::~INIFile()
{
    if (pon_dayt_p)
        delete (Timer::DaytimeHHMM *) /* casting away const-ness */
            pon_dayt_p;
    if (poff_dayt_p)
        delete (Timer::DaytimeHHMM *) /* casting away const-ness */
            poff_dayt_p;
    if (kbhit_poff_delay_dayt_p)
        delete (Timer::DaytimeHHMM *) /* casting away const-ness */
        kbhit_poff_delay_dayt_p;
}

INIFile *
    INIFile::parse()
{
    Timer::DaytimeHHMM const
        * pon_dayt_p = NULL,
        * poff_dayt_p = NULL,
        * kbhit_poff_delay_dayt_p = NULL;

    ifstream inifile (ini_filename, ios::in);

    if (inifile.rdbuf()->is_open())
    {
        unsigned int inifile_error = FALSE;
        unsigned int issection_timer = FALSE;
        char * curr_section = NULL;
        unsigned char line_buf[128] = { '\0' };
        char * str_tok;

        while (inifile.eof() == 0)
        {
            inifile.getline(line_buf, '\r\n');

            if (strlen(line_buf) == 0) continue; // ignore empty lines
            if (line_buf[0] == ';') continue; // ignore comments

            if (line_buf[0] == '['
                && line_buf[strlen(line_buf) - 1] == ']')
            {
                line_buf[strlen(line_buf) - 1] = '\0';
                if (strcmpi(line_buf + 1, "Timer") == 0)
                {
                    curr_section = "Timer";
                    issection_timer = TRUE;
                }
                else
                {
                    char s[85];
                    strcpy(s, ini_filename); // iostream is too big for POFO, use string functions
                    strcat(s, ": Unknown section: '");
                    strcat(s, line_buf + 1);
                    strcat(s, "'\r\n$");
                    PFBios::show_message_earlystage(s);

                    inifile_error = TRUE;
                }

                continue;
            }

            str_tok = strtok(line_buf, "=");
            if (str_tok)
            {
                if (issection_timer)
                {
                    int iskey_TriggerPowerOnAt = FALSE;
                    int iskey_TriggerPowerOffAt = FALSE;
                    int iskey_PowerOffDelayKbhit = FALSE;

                    if (strcmpi(str_tok, "TriggerPowerOnAt") == 0)
                    {
                        iskey_TriggerPowerOnAt = TRUE;
                    }
                    else if (strcmpi(str_tok, "TriggerPowerOffAt") == 0)
                    {
                        iskey_TriggerPowerOffAt = TRUE;
                    }
                    else if (strcmpi(str_tok, "PowerOffDelayKbhit") == 0)
                    {
                        iskey_PowerOffDelayKbhit = TRUE;
                    }
                    else
                    {
                        char s[85];
                        strcpy(s, ini_filename);
                        strcat(s, ": Unknown key in section '");
                        strcat(s, curr_section);
                        strcat(s, "': '");
                        strcat(s, line_buf);
                        strcat(s, "'\r\n$");
                        PFBios::show_message_earlystage(s);

                        inifile_error = TRUE;
                        continue;
                    }

                    str_tok = strtok(NULL, "");
                    if (str_tok)
                    {
                        if (iskey_TriggerPowerOnAt
                            || iskey_TriggerPowerOffAt
                            || iskey_PowerOffDelayKbhit)
                        {
                            int nfields_ok = -1,
                                inival_hour = -1,
                                inival_minute = -1;

                            nfields_ok = sscanf(str_tok, "%2u:%2u",
                                &inival_hour, &inival_minute);

                            if (nfields_ok == 2 &&
                                inival_hour >= 0 && inival_hour <= 23 &&
                                inival_minute >= 0 && inival_minute <= 59)
                            {
                                if (iskey_TriggerPowerOnAt)
                                {
                                    pon_dayt_p = new Timer::DaytimeHHMM
                                        (inival_hour, inival_minute);
                                }
                                else if (iskey_TriggerPowerOffAt)
                                {
                                    poff_dayt_p = new Timer::DaytimeHHMM
                                        (inival_hour, inival_minute);
                                }
                                else if (iskey_PowerOffDelayKbhit)
                                {
                                    kbhit_poff_delay_dayt_p = new Timer::DaytimeHHMM
                                        (inival_hour, inival_minute);
                                }
                            }
                            else
                            {
                                char s[85];
                                strcpy(s, ini_filename);
                                strcat(s, ": Bad value in section '");
                                strcat(s, curr_section);
                                strcat(s, "', key '");
                                strcat(s, line_buf);
                                strcat(s, "': '");
                                strcat(s, str_tok);
                                strcat(s, "'\r\n$");
                                PFBios::show_message_earlystage(s);

                                inifile_error = TRUE;
                                continue;
                            }
                        }
                    }
                }
            }
        }

        if (inifile_error)
            return NULL;
    }

    return new INIFile(
        pon_dayt_p, // moving ownership to new object
        poff_dayt_p,
        kbhit_poff_delay_dayt_p);
}
