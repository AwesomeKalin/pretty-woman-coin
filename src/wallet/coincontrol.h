// Copyright (c) 2011-2016 The Prettywomancoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

/** Coin Control Features. */
class CCoinControl {
public:
    CTxDestination destChange;
    //! If false, allows unselected inputs, but requires all selected inputs be
    //! used
    bool fAllowOtherInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE
    //! criteria
    bool fAllowWatchOnly;
    //! Minimum absolute fee (not per kilobyte)
    Amount nMinimumTotalFee;
    //! Override estimated feerate
    bool fOverrideFeeRate;
    //! Feerate to use if overrideFeeRate is true
    CFeeRate nFeeRate;

    CCoinControl() { SetNull(); }

    void SetNull() {
        destChange = CNoDestination();
        fAllowOtherInputs = false;
        fAllowWatchOnly = false;
        setSelected.clear();
        nMinimumTotalFee = Amount(0);
        nFeeRate = CFeeRate(Amount(0));
        fOverrideFeeRate = false;
    }

    bool HasSelected() const { return (setSelected.size() > 0); }

    bool IsSelected(const COutPoint &output) const {
        return (setSelected.count(output) > 0);
    }

    void Select(const COutPoint &output) { setSelected.insert(output); }

    void ListSelected(std::vector<COutPoint> &vOutpoints) const {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

private:
    std::set<COutPoint> setSelected;
};

#endif // BITCOIN_WALLET_COINCONTROL_H
