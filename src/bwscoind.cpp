// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bwscoin-config.h"
#endif

#include "chainparams.h"
#include "clientversion.h"
#include "compat.h"
#include "fs.h"
#include "rpc/server.h"
#include "init.h"
#include "noui.h"
#include "scheduler.h"
#include "util.h"
#include "httpserver.h"
#include "httprpc.h"
#include "utilstrencodings.h"

#include <boost/thread.hpp>

#include <stdio.h>

/* Introduction text for doxygen: */

/*! \mainpage Developer documentation
 *
 * \section intro_sec Introduction
 *
 * This is the developer documentation of the reference client for an experimental new digital currency called BWS Coin (https://www.bwscoin.org/),
 * which enables instant payments to anyone, anywhere in the world. BWS Coin uses peer-to-peer technology to operate
 * with no central authority: managing transactions and issuing money are carried out collectively by the network.
 *
 * The software is a community-driven open source project, released under the MIT license.
 *
 * \section Navigation
 * Use the buttons <code>Namespaces</code>, <code>Classes</code> or <code>Files</code> at the top of the page to start navigating the code.
 */

void WaitForShutdown(boost::thread_group* threadGroup)
{
    bool fShutdown = ShutdownRequested();
    // Tell the main threads to shutdown.
    while (!fShutdown)
    {
        MilliSleep(200);
        fShutdown = ShutdownRequested();
    }
    if (threadGroup)
    {
        Interrupt(*threadGroup);
        threadGroup->join_all();
    }
}

#ifdef USE_CHAINPARAMS_CONF

void SaveGenesisConf(const std::string& confPath)
{
    std::ofstream conf;
    conf.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
    conf.open(confPath);

    conf << "MAINNET_GENESIS_BLOCK_NONCE = "          << gGenesisparams.GetArg("MAINNET_GENESIS_BLOCK_NONCE", "") << "\n";
    conf << "MAINNET_CONSENSUS_HASH_GENESIS_BLOCK = " << gGenesisparams.GetArg("MAINNET_CONSENSUS_HASH_GENESIS_BLOCK", "") << "\n";
    conf << "MAINNET_GENESIS_HASH_MERKLE_ROOT = "     << gGenesisparams.GetArg("MAINNET_GENESIS_HASH_MERKLE_ROOT", "") << "\n";

    conf << "TESTNET_GENESIS_BLOCK_NONCE = "          << gGenesisparams.GetArg("TESTNET_GENESIS_BLOCK_NONCE", "") << "\n";
    conf << "TESTNET_CONSENSUS_HASH_GENESIS_BLOCK = " << gGenesisparams.GetArg("TESTNET_CONSENSUS_HASH_GENESIS_BLOCK", "") << "\n";
    conf << "TESTNET_GENESIS_HASH_MERKLE_ROOT = "     << gGenesisparams.GetArg("TESTNET_GENESIS_HASH_MERKLE_ROOT", "") << "\n";

    conf << "REGTEST_GENESIS_BLOCK_NONCE = "          << gGenesisparams.GetArg("REGTEST_GENESIS_BLOCK_NONCE", "") << "\n";
    conf << "REGTEST_CONSENSUS_HASH_GENESIS_BLOCK = " << gGenesisparams.GetArg("REGTEST_CONSENSUS_HASH_GENESIS_BLOCK", "") << "\n";
    conf << "REGTEST_GENESIS_HASH_MERKLE_ROOT = "     << gGenesisparams.GetArg("REGTEST_GENESIS_HASH_MERKLE_ROOT", "") << "\n";

    conf.close();
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char* argv[])
{
    boost::thread_group threadGroup;
    CScheduler scheduler;

    bool fRet = false;

    //
    // Parameters
    //
    // If Qt is used, parameters/bwscoin.conf are parsed in qt/bwscoin.cpp's main()
    gArgs.ParseParameters(argc, argv);

    // Process help and version before taking care about datadir
    if (gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") ||  gArgs.IsArgSet("-help") || gArgs.IsArgSet("-version"))
    {
        std::string strUsage = strprintf(_("%s Daemon"), _(PACKAGE_NAME)) + " " + _("version") + " " + FormatFullVersion() + "\n";

        if (gArgs.IsArgSet("-version"))
        {
            strUsage += FormatParagraph(LicenseInfo());
        }
        else
        {
            strUsage += "\n" + _("Usage:") + "\n" +
                  "  bwscoind [options]                     " + strprintf(_("Start %s Daemon"), _(PACKAGE_NAME)) + "\n";

            strUsage += "\n" + HelpMessage(HMM_BWSCOIND);
        }

        fprintf(stdout, "%s", strUsage.c_str());
        return true;
    }

    try
    {
        if (!fs::is_directory(GetDataDir(false)))
        {
            fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
            return false;
        }
        try
        {
            gArgs.ReadConfigFile(gArgs.GetArg("-conf", BWSCOIN_CONF_FILENAME));
        } catch (const std::exception& e) {
            fprintf(stderr,"Error reading configuration file: %s\n", e.what());
            return false;
        }
#ifdef USE_CHAINPARAMS_CONF
        try
        {
            gChainparams.ReadConfigFile(gArgs.GetArg("-chainparams-conf", BWSCOIN_CHAINPARAMS_CONF_FILENAME));
        } catch (const std::exception& e) {
            fprintf(stderr,"Error reading chainparams configuration file: %s\n", e.what());
            return false;
        }
#endif

        // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
        try {
            SelectParams(ChainNameFromCommandLine());
        } catch (const std::exception& e) {
            fprintf(stderr, "Error: %s\n", e.what());
            return false;
        }

        // Error out when loose non-argument tokens are encountered on command line
        for (int i = 1; i < argc; i++) {
            if (!IsSwitchChar(argv[i][0])) {
                fprintf(stderr, "Error: Command line contains unexpected token '%s', see bwscoind -h for a list of options.\n", argv[i]);
                return false;
            }
        }

        // -server defaults to true for bwscoind but not for the GUI so do this here
        gArgs.SoftSetBoolArg("-server", true);
        // Set this early so that parameter interactions go to console
        InitLogging();
        InitParameterInteraction();
        if (!AppInitBasicSetup())
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (!AppInitParameterInteraction())
        {
            // InitError will have been called with detailed error, which ends up on console
            return false;
        }
        if (!AppInitSanityChecks())
        {
            // InitError will have been called with detailed error, which ends up on console
            exit(EXIT_FAILURE);
        }
#ifdef USE_CHAINPARAMS_CONF
        if (gArgs.IsArgSet("-mine-genesis-block"))
        {
            try
            {
                SaveGenesisConf(GetDataDir().string() + '/' + BWSCOIN_GENESIS_CONF_FILENAME);
            } catch (const std::exception& e) {
                fprintf(stderr,"Error writting genesis configuration file: %s\n", e.what());
                return false;
            }
            exit(0);
        }
#endif
        if (gArgs.GetBoolArg("-daemon", false))
        {
#if HAVE_DECL_DAEMON
            fprintf(stdout, "BWS Coin server starting\n");

            // Daemonize
            if (daemon(1, 0)) { // don't chdir (1), do close FDs (0)
                fprintf(stderr, "Error: daemon() failed: %s\n", strerror(errno));
                return false;
            }
#else
            fprintf(stderr, "Error: -daemon is not supported on this operating system\n");
            return false;
#endif // HAVE_DECL_DAEMON
        }
        // Lock data directory after daemonization
        if (!AppInitLockDataDirectory())
        {
            // If locking the data directory failed, exit immediately
            return false;
        }
        fRet = AppInitMain(threadGroup, scheduler);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

    if (!fRet)
    {
        Interrupt(threadGroup);
        threadGroup.join_all();
    } else {
        WaitForShutdown(&threadGroup);
    }
    Shutdown();

    return fRet;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();

    // Connect bwscoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
