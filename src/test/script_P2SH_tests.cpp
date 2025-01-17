// Copyright (c) 2012-2016 The Prettywomancoin Core developers
// Copyright (c) 2019 Prettywomancoin Association
// Distributed under the Open PWC software license, see the accompanying file
// LICENSE.

#include "chainparams.h"
#include "config.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "policy/policy.h"
#include "script/ismine.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/sign.h"
#include "test/test_prettywomancoin.h"
#include "validation.h"
#include "taskcancellation.h"

#include <vector>

#include <boost/test/unit_test.hpp>

// Helpers:
static std::vector<uint8_t> Serialize(const CScript &s) {
    std::vector<uint8_t> sSerialized(s.begin(), s.end());
    return sSerialized;
}

static bool Verify(const CScript &scriptSig, const CScript &scriptPubKey,
                   bool fStrict, ScriptError &err) {
    const Config& config = GlobalConfig::GetConfig();
    // Create dummy to/from transactions:
    CMutableTransaction txFrom;
    txFrom.vout.resize(1);
    txFrom.vout[0].scriptPubKey = scriptPubKey;

    CMutableTransaction txTo;
    txTo.vin.resize(1);
    txTo.vout.resize(1);
    txTo.vin[0].prevout = COutPoint(txFrom.GetId(), 0);
    txTo.vin[0].scriptSig = scriptSig;
    txTo.vout[0].nValue = Amount(1);

    auto res =
        VerifyScript(
            config, true,
            task::CCancellationSource::Make()->GetToken(),
            scriptSig,
            scriptPubKey,
            (fStrict ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE) |
                SCRIPT_ENABLE_SIGHASH_FORKID,
            MutableTransactionSignatureChecker(&txTo, 0, txFrom.vout[0].nValue),
            &err);

    return res.value();
}

BOOST_FIXTURE_TEST_SUITE(script_P2SH_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sign) {

    testConfig.SetGenesisActivationHeight(10); // arbitrary number, will test at this height and on lower by one
    LOCK(cs_main);
    // Pay-to-script-hash looks like this:
    // scriptSig:    <sig> <sig...> <serialized_script>
    // scriptPubKey: HASH160 <hash> EQUAL

    // Test SignSignature() (and therefore the version of Solver() that signs
    // transactions)
    CBasicKeyStore keystore;
    CKey key[4];
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
    }

    // 8 Scripts: checking all combinations of
    // different keys, straight/P2SH, pubkey/pubkeyhash
    CScript standardScripts[4];
    standardScripts[0] << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    standardScripts[1] = GetScriptForDestination(key[1].GetPubKey().GetID());
    standardScripts[2] << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    standardScripts[3] = GetScriptForDestination(key[2].GetPubKey().GetID());
    CScript evalScripts[4];
    for (int i = 0; i < 4; i++) {
        keystore.AddCScript(standardScripts[i]);
        evalScripts[i] = GetScriptForDestination(CScriptID(standardScripts[i]));
    }

    CMutableTransaction txFrom; // Funding transaction:
    std::string reason;
    txFrom.vout.resize(8);
    for (int i = 0; i < 4; i++) {
        txFrom.vout[i].scriptPubKey = evalScripts[i];
        txFrom.vout[i].nValue = COIN;
        txFrom.vout[i + 4].scriptPubKey = standardScripts[i];
        txFrom.vout[i + 4].nValue = COIN;
    }
    
    BOOST_CHECK(IsStandardTx(
        testConfig, CTransaction(txFrom),
        testConfig.GetGenesisActivationHeight() - 1, reason));
    BOOST_CHECK(!IsStandardTx(
        testConfig, CTransaction(txFrom),
        testConfig.GetGenesisActivationHeight(), reason));

    CMutableTransaction txTo[8]; // Spending transactions
    for (int i = 0; i < 8; i++) {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout = COutPoint(txFrom.GetId(), i);
        txTo[i].vout[0].nValue = Amount(1);
        BOOST_CHECK_MESSAGE(
            IsMine(keystore, txFrom.vout[i].scriptPubKey),
            strprintf("IsMine %d", i));
    }
    for (int i = 0; i < 8; i++) {
        
        bool isP2SH = (i < 4); // first four tx are P2SH, other are P2PKH
        if (isP2SH) {
            // If UTOX is created after Genesis, we do not recognize P2SH outputs and 
            // therefore we are not able to sign them.
            BOOST_CHECK_MESSAGE(!SignSignature(testConfig, keystore, true, true,
                                              CTransaction(txFrom), txTo[i], 0,
                                              SigHashType().withForkId()),
                                strprintf("SignSignature %d", i));
        } else {
            BOOST_CHECK_MESSAGE(SignSignature(testConfig, keystore, true, true,
                                              CTransaction(txFrom), txTo[i], 0,
                                              SigHashType().withForkId()),
                                strprintf("SignSignature %d", i));
        }
        
        BOOST_CHECK_MESSAGE(SignSignature(testConfig, keystore, true, false,
                                          CTransaction(txFrom),
                                          txTo[i], 0,
                                          SigHashType().withForkId()),
                           strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(SignSignature(testConfig, keystore, false, false,
                                          CTransaction(txFrom), txTo[i], 0,
                                          SigHashType().withForkId()),
                            strprintf("SignSignature %d", i));
    }
    // All of the above should be OK, and the txTos have valid signatures
    // Check to make sure signature verification fails if we use the wrong
    // ScriptSig:
    auto source = task::CCancellationSource::Make();
    for (int i = 0; i < 8; i++) {
        CTransaction tx(txTo[i]);
        PrecomputedTransactionData txdata(tx);
        for (int j = 0; j < 8; j++) {
            CScript sigSave = txTo[i].vin[0].scriptSig;
            txTo[i].vin[0].scriptSig = txTo[j].vin[0].scriptSig;
            const CTxOut &output = txFrom.vout[txTo[i].vin[0].prevout.GetN()];
            auto sigOK = CScriptCheck(testConfig, true,
                output.scriptPubKey, output.nValue, CTransaction(txTo[i]), 0,
                SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC |
                    SCRIPT_ENABLE_SIGHASH_FORKID,
                false, txdata)(source->GetToken());
            if (i == j) {
                BOOST_CHECK_MESSAGE(sigOK.value(),
                                    strprintf("VerifySignature %d %d", i, j));
            } else {
                BOOST_CHECK_MESSAGE(!sigOK.value(),
                                    strprintf("VerifySignature %d %d", i, j));
            }
            txTo[i].vin[0].scriptSig = sigSave;
        }
    }
}

BOOST_AUTO_TEST_CASE(norecurse) {
    ScriptError err;
    // Make sure only the outer pay-to-script-hash does the
    // extra-validation thing:
    CScript invalidAsScript;
    invalidAsScript << OP_INVALIDOPCODE << OP_INVALIDOPCODE;

    CScript p2sh = GetScriptForDestination(CScriptID(invalidAsScript));

    CScript scriptSig;
    scriptSig << Serialize(invalidAsScript);

    // Should not verify, because it will try to execute OP_INVALIDOPCODE
    BOOST_CHECK(!Verify(scriptSig, p2sh, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_BAD_OPCODE, ScriptErrorString(err));

    // Try to recur, and verification should succeed because
    // the inner HASH160 <> EQUAL should only check the hash:
    CScript p2sh2 = GetScriptForDestination(CScriptID(p2sh));
    CScript scriptSig2;
    scriptSig2 << Serialize(invalidAsScript) << Serialize(p2sh);

    BOOST_CHECK(Verify(scriptSig2, p2sh2, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
}

BOOST_AUTO_TEST_CASE(set) {

    testConfig.SetGenesisActivationHeight(10); // arbitrary number, will test at this height and on lower by one

    LOCK(cs_main);
    // Test the CScript::Set* methods
    CBasicKeyStore keystore;
    CKey key[4];
    std::vector<CPubKey> keys;
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
        keys.push_back(key[i].GetPubKey());
    }

    CScript inner[4];
    inner[0] = GetScriptForDestination(key[0].GetPubKey().GetID());
    inner[1] = GetScriptForMultisig(
        2, std::vector<CPubKey>(keys.begin(), keys.begin() + 2));
    inner[2] = GetScriptForMultisig(
        1, std::vector<CPubKey>(keys.begin(), keys.begin() + 2));
    inner[3] = GetScriptForMultisig(
        2, std::vector<CPubKey>(keys.begin(), keys.begin() + 3));

    CScript outer[4];
    for (int i = 0; i < 4; i++) {
        outer[i] = GetScriptForDestination(CScriptID(inner[i]));
        keystore.AddCScript(inner[i]);
    }

    // Funding transaction:
    CMutableTransaction txFrom;
    std::string reason;
    txFrom.vout.resize(4);
    for (int i = 0; i < 4; i++) {
        txFrom.vout[i].scriptPubKey = outer[i];
        txFrom.vout[i].nValue = CENT;
    }
    BOOST_CHECK(IsStandardTx(testConfig, CTransaction(txFrom), testConfig.GetGenesisActivationHeight() - 1, reason));
    BOOST_CHECK(!IsStandardTx(testConfig, CTransaction(txFrom), testConfig.GetGenesisActivationHeight(), reason));
    BOOST_CHECK(reason == "scriptpubkey");

    // Spending transactions
    CMutableTransaction txTo[4];
    for (int i = 0; i < 4; i++) {
        txTo[i].vin.resize(1);
        txTo[i].vout.resize(1);
        txTo[i].vin[0].prevout = COutPoint(txFrom.GetId(), i);
        txTo[i].vout[0].nValue = 1 * CENT;
        txTo[i].vout[0].scriptPubKey = inner[i];
        BOOST_CHECK_MESSAGE(
            IsMine(keystore, txFrom.vout[i].scriptPubKey),
            strprintf("IsMine %d", i));
    }
    for (int i = 0; i < 4; i++) {
        BOOST_CHECK_MESSAGE(!SignSignature(testConfig, keystore, true, true, CTransaction(txFrom),
                                           txTo[i], 0,
                                           SigHashType().withForkId()),
                            strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(!SignSignature(testConfig, keystore, false, true, CTransaction(txFrom),
                                           txTo[i], 0,
                                           SigHashType().withForkId()),
                            strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(SignSignature(testConfig, keystore, true, false, CTransaction(txFrom),
                                          txTo[i], 0,
                                          SigHashType().withForkId()),
                            strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(SignSignature(testConfig, keystore, false, false, CTransaction(txFrom),
                                          txTo[i], 0,
                                          SigHashType().withForkId()),
                            strprintf("SignSignature %d", i));
        BOOST_CHECK_MESSAGE(IsStandardTx(testConfig, CTransaction(txTo[i]),
                                         testConfig.GetGenesisActivationHeight(),
                                         reason),
                            strprintf("txTo[%d].IsStandard", i));
        BOOST_CHECK_MESSAGE(IsStandardTx(testConfig, CTransaction(txTo[i]),
                                         testConfig.GetGenesisActivationHeight() - 1,
                                          reason),
                            strprintf("txTo[%d].IsStandard", i));
    }
}

BOOST_AUTO_TEST_CASE(is) {
    // Test IsP2SH()
    uint160 dummy;
    CScript p2sh;
    p2sh << OP_HASH160 << ToByteVector(dummy) << OP_EQUAL;
    BOOST_CHECK(IsP2SH(p2sh));

    // Not considered pay-to-script-hash if using one of the OP_PUSHDATA
    // opcodes:
    static const uint8_t direct[] = {OP_HASH160, 20, 0, 0, 0, 0, 0,       0,
                                     0,          0,  0, 0, 0, 0, 0,       0,
                                     0,          0,  0, 0, 0, 0, OP_EQUAL};
    BOOST_CHECK(IsP2SH(direct));
    static const uint8_t pushdata1[] = {OP_HASH160, OP_PUSHDATA1,
                                        20,         0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          OP_EQUAL};
    BOOST_CHECK(!IsP2SH(pushdata1));
    static const uint8_t pushdata2[] = {OP_HASH160, OP_PUSHDATA2,
                                        20,         0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        OP_EQUAL};
    BOOST_CHECK(!IsP2SH(pushdata2));
    static const uint8_t pushdata4[] = {OP_HASH160, OP_PUSHDATA4,
                                        20,         0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        0,          0,
                                        OP_EQUAL};
    BOOST_CHECK(!IsP2SH(pushdata4));

    CScript not_p2sh;
    BOOST_CHECK(!IsP2SH(not_p2sh));

    not_p2sh.clear();
    not_p2sh << OP_HASH160 << ToByteVector(dummy) << ToByteVector(dummy)
             << OP_EQUAL;
    BOOST_CHECK(!IsP2SH(not_p2sh));

    not_p2sh.clear();
    not_p2sh << OP_NOP << ToByteVector(dummy) << OP_EQUAL;
    BOOST_CHECK(!IsP2SH(not_p2sh));

    not_p2sh.clear();
    not_p2sh << OP_HASH160 << ToByteVector(dummy) << OP_CHECKSIG;
    BOOST_CHECK(!IsP2SH(not_p2sh));
}

BOOST_AUTO_TEST_CASE(switchover) {
    // Test switch over code
    CScript notValid;
    ScriptError err;
    notValid << OP_11 << OP_12 << OP_EQUALVERIFY;
    CScript scriptSig;
    scriptSig << Serialize(notValid);

    CScript fund = GetScriptForDestination(CScriptID(notValid));

    // Validation should succeed under old rules (hash is correct):
    BOOST_CHECK(Verify(scriptSig, fund, false, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    // Fail under new:
    BOOST_CHECK(!Verify(scriptSig, fund, true, err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EQUALVERIFY, ScriptErrorString(err));
}

BOOST_AUTO_TEST_CASE(AreInputsStandard) {
    LOCK(cs_main);
    CCoinsViewEmpty coinsDummy;
    CCoinsViewCache coins(coinsDummy);
    CBasicKeyStore keystore;
    CKey key[6];
    std::vector<CPubKey> keys;
    for (int i = 0; i < 6; i++) {
        key[i].MakeNewKey(true);
        keystore.AddKey(key[i]);
    }
    for (int i = 0; i < 3; i++)
        keys.push_back(key[i].GetPubKey());

    auto& tempConfig = testConfig;
    auto activateGenesis = [&tempConfig]() {tempConfig.SetGenesisActivationHeight(1); };
    auto deactivateGenesis = [&tempConfig]() {tempConfig.SetGenesisActivationHeight(1000); };

    CMutableTransaction txFrom;
    txFrom.vout.resize(7);

    // First three are standard:
    CScript pay1 = GetScriptForDestination(key[0].GetPubKey().GetID());
    keystore.AddCScript(pay1);
    CScript pay1of3 = GetScriptForMultisig(1, keys);

    // P2SH (OP_CHECKSIG)
    txFrom.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(pay1));
    txFrom.vout[0].nValue = Amount(1000);
    // ordinary OP_CHECKSIG
    txFrom.vout[1].scriptPubKey = pay1;
    txFrom.vout[1].nValue = Amount(2000);
    // ordinary OP_CHECKMULTISIG
    txFrom.vout[2].scriptPubKey = pay1of3;
    txFrom.vout[2].nValue = Amount(3000);

    // vout[3] is complicated 1-of-3 AND 2-of-3
    // ... that is OK if wrapped in P2SH:
    CScript oneAndTwo;
    oneAndTwo << OP_1 << ToByteVector(key[0].GetPubKey())
              << ToByteVector(key[1].GetPubKey())
              << ToByteVector(key[2].GetPubKey());
    oneAndTwo << OP_3 << OP_CHECKMULTISIGVERIFY;
    oneAndTwo << OP_2 << ToByteVector(key[3].GetPubKey())
              << ToByteVector(key[4].GetPubKey())
              << ToByteVector(key[5].GetPubKey());
    oneAndTwo << OP_3 << OP_CHECKMULTISIG;
    keystore.AddCScript(oneAndTwo);
    txFrom.vout[3].scriptPubKey = GetScriptForDestination(CScriptID(oneAndTwo));
    txFrom.vout[3].nValue = Amount(4000);

    // vout[4] is max sigops:
    CScript fifteenSigops;
    fifteenSigops << OP_1;
    for (unsigned i = 0; i < MAX_P2SH_SIGOPS; i++)
        fifteenSigops << ToByteVector(key[i % 3].GetPubKey());
    fifteenSigops << OP_15 << OP_CHECKMULTISIG;
    keystore.AddCScript(fifteenSigops);
    txFrom.vout[4].scriptPubKey =
        GetScriptForDestination(CScriptID(fifteenSigops));
    txFrom.vout[4].nValue = Amount(5000);

    // vout[5/6] are non-standard because they exceed MAX_P2SH_SIGOPS
    CScript sixteenSigops;
    sixteenSigops << OP_16 << OP_CHECKMULTISIG;
    keystore.AddCScript(sixteenSigops);
    txFrom.vout[5].scriptPubKey =
        GetScriptForDestination(CScriptID(fifteenSigops));
    txFrom.vout[5].nValue = Amount(5000);
    CScript twentySigops;
    twentySigops << OP_CHECKMULTISIG;
    keystore.AddCScript(twentySigops);
    txFrom.vout[6].scriptPubKey =
        GetScriptForDestination(CScriptID(twentySigops));
    txFrom.vout[6].nValue = Amount(6000);

    AddCoins(coins, CTransaction(txFrom), false, 10, 0);

    CMutableTransaction txTo;
    txTo.vout.resize(1);
    txTo.vout[0].scriptPubKey =
        GetScriptForDestination(key[1].GetPubKey().GetID());

    txTo.vin.resize(5);
    for (int i = 0; i < 5; i++) {
        txTo.vin[i].prevout = COutPoint(txFrom.GetId(), i);
    }

    BOOST_CHECK(!SignSignature(testConfig, keystore, true, true, CTransaction(txFrom), txTo, 0,
                               SigHashType().withForkId())); // is P2SH
    BOOST_CHECK(SignSignature(testConfig, keystore, true, true, CTransaction(txFrom), txTo, 1,
                               SigHashType().withForkId())); // is not P2SH
    BOOST_CHECK(SignSignature(testConfig, keystore, true, true, CTransaction(txFrom), txTo, 2,
                               SigHashType().withForkId())); // is not P2SH

    BOOST_CHECK(SignSignature(testConfig, keystore, true, false, CTransaction(txFrom), txTo, 0,
                              SigHashType().withForkId()));
    BOOST_CHECK(SignSignature(testConfig, keystore, true, false, CTransaction(txFrom), txTo, 1,
                              SigHashType().withForkId()));
    BOOST_CHECK(SignSignature(testConfig, keystore, true, false, CTransaction(txFrom), txTo, 2,
                              SigHashType().withForkId()));

    BOOST_CHECK(SignSignature(testConfig, keystore, false, false, CTransaction(txFrom), txTo, 0,
                              SigHashType().withForkId()));
    BOOST_CHECK(SignSignature(testConfig, keystore, false, false, CTransaction(txFrom), txTo, 1,
                              SigHashType().withForkId()));
    BOOST_CHECK(SignSignature(testConfig, keystore, false, false, CTransaction(txFrom), txTo, 2,
                              SigHashType().withForkId()));

    // SignSignature doesn't know how to sign these. We're not testing
    // validating signatures, so just create dummy signatures that DO include
    // the correct P2SH scripts:
    txTo.vin[3].scriptSig << OP_11 << OP_11
                          << std::vector<uint8_t>(oneAndTwo.begin(),
                                                  oneAndTwo.end());
    txTo.vin[4].scriptSig << std::vector<uint8_t>(fifteenSigops.begin(),
                                                  fifteenSigops.end());


    auto source = task::CCancellationSource::Make();

    bool sigOpCountError;
    activateGenesis();
    BOOST_CHECK(!::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txTo), coins, 0).value());
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txTo), coins, sigOpCountError), 0U);
    
    deactivateGenesis();
    BOOST_CHECK(::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txTo), coins, 0).value());
    // 22 P2SH sigops for all inputs (1 for vin[0], 6 for vin[3], 15 for vin[4]
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txTo), coins, sigOpCountError), 22U);

    CMutableTransaction txToNonStd1;
    txToNonStd1.vout.resize(1);
    txToNonStd1.vout[0].scriptPubKey =
        GetScriptForDestination(key[1].GetPubKey().GetID());
    txToNonStd1.vout[0].nValue = Amount(1000);
    txToNonStd1.vin.resize(1);
    txToNonStd1.vin[0].prevout = COutPoint(txFrom.GetId(), 5);
    txToNonStd1.vin[0].scriptSig
        << std::vector<uint8_t>(sixteenSigops.begin(), sixteenSigops.end());

    activateGenesis();
    BOOST_CHECK(!::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txToNonStd1), coins, 0).value());
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txToNonStd1), coins, sigOpCountError), 0U);
    
    deactivateGenesis();
    BOOST_CHECK(!::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txToNonStd1), coins, 0).value());
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txToNonStd1), coins, sigOpCountError), 16U);

    CMutableTransaction txToNonStd2;
    txToNonStd2.vout.resize(1);
    txToNonStd2.vout[0].scriptPubKey =
        GetScriptForDestination(key[1].GetPubKey().GetID());
    txToNonStd2.vout[0].nValue = Amount(1000);
    txToNonStd2.vin.resize(1);
    txToNonStd2.vin[0].prevout = COutPoint(txFrom.GetId(), 6);
    txToNonStd2.vin[0].scriptSig
        << std::vector<uint8_t>(twentySigops.begin(), twentySigops.end());

    activateGenesis();
    BOOST_CHECK(!::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txToNonStd2), coins, 0).value());
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txToNonStd2), coins, sigOpCountError), 0U);

    deactivateGenesis();
    BOOST_CHECK(!::AreInputsStandard(source->GetToken(), testConfig, CTransaction(txToNonStd2), coins, 0).value());
    BOOST_CHECK_EQUAL(GetP2SHSigOpCount(testConfig, CTransaction(txToNonStd2), coins, sigOpCountError), 20U);
}

BOOST_AUTO_TEST_SUITE_END()
