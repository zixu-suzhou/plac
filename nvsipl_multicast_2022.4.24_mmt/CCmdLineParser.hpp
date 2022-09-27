/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/* STL Headers */
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <getopt.h>
#include <iomanip>

#include "NvSIPLTrace.hpp" // NvSIPLTrace to set library trace level

#ifndef CCMDLINEPARSER_HPP
#define CCMDLINEPARSER_HPP

using namespace std;
using namespace nvsipl;

class CCmdLineParser
{
 public:
    // Command line options
    uint32_t verbosity = 1u;
    uint32_t consumerId = 0;
    string sNitoFolderPath = "";
    string sPlatformCfgName = "F008A120RM0A_CPHY_x4";
    bool bMultiProcess = false;
    bool bIsProducer = false;
    bool bIsConsumer = false;
    string sConsumerType = "";
    bool bIgnoreError = false;
    vector<uint32_t> vMasks;

    static void ShowUsage(void)
    {
        cout << "Usage:\n";
        cout << "-h or --help                               :Prints this help\n";
        cout << "-v or --verbosity <level>                  :Set verbosity\n";
#if !NV_IS_SAFETY
        cout << "                                           :Supported values (default: 1)\n";
        cout << "                                           : " << INvSIPLTrace::LevelNone << " (None)\n";
        cout << "                                           : " << INvSIPLTrace::LevelError << " (Errors)\n";
        cout << "                                           : " << INvSIPLTrace::LevelWarning << " (Warnings and above)\n";
        cout << "                                           : " << INvSIPLTrace::LevelInfo << " (Infos and above)\n";
        cout << "                                           : " << INvSIPLTrace::LevelDebug << " (Debug and above)\n";
#endif // !NV_IS_SAFETY
        cout << "-t <platformCfgName>                       :Specify platform configuration, default is F008A120RM0A_CPHY_x4\n";
        cout << "--nito <folder>                            :Path to folder containing NITO files\n";
        cout << "-I                                         :Ignore the fatal error\n";
        cout << "-m                                         :camera channel index\n";
        cout << "-u                                         :consumer id\n";
        cout << "-p                                         :producer resides in this process\n";
        cout << "-c 'type'                                  :consumer resides in this process.\n";
        cout << "                                           :Supported type: 'enc': encoder customer, 'cuda': cuda customer.\n";
        return;
    }

    int Parse(int argc, char* argv[])
    {
        const char* const short_options = "hv:t:N:Ipc:m:u:";
        const struct option long_options[] =
        {
            { "help",                 no_argument,       0, 'h' },
            { "verbosity",            required_argument, 0, 'v' },
            { "nito",                 required_argument, 0, 'N' },
            { 0,                      0,                 0,  0 }
        };

        int index = 0;
        auto bShowHelp = false;

        while (1) {
            const auto getopt_ret = getopt_long(argc, argv, short_options , &long_options[0], &index);
            if (getopt_ret == -1) {
                // Done parsing all arguments.
                break;
            }

            switch (getopt_ret) {
            default: /* Unrecognized option */
            case '?': /* Unrecognized option */
                cout << "Invalid or Unrecognized command line option. Specify -h or --help for options\n";
                break;
            case 'h': /* -h or --help */
                bShowHelp = true;
                break;
            case 'v':
                verbosity = atoi(optarg);
                break;
            case 't':
                sPlatformCfgName = string(optarg);
                break;
            case 'N':
                sNitoFolderPath = string(optarg);
                break;
            case 'I':
                bIgnoreError = true;
                break;
            case 'm':
                {
                    char* token = std::strtok(optarg, " ");
                    while(token != NULL) {
                        vMasks.push_back(stoi(token, nullptr, 16));
                        token = std::strtok(NULL, " ");
                    }
                }
                break;
            case 'p': /* set producer resident */
                bIsProducer = true;
                bMultiProcess = true;
                break;
            case 'c': /* set consumer resident */
                bMultiProcess = true;
                bIsConsumer = true;
                sConsumerType = string(optarg);
                break;
            case 'u':
                consumerId = atoi(optarg);
                break;
            }
        }

        if (bShowHelp) {
            ShowUsage();
            return -1;
        }

        return 0;
    }
};

#endif //CCMDPARSER_HPP
