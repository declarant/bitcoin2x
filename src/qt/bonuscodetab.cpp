#include "bonuscodetab.h"
#include "../chainparams.h"
#include "../consensus/validation.h"
#include "../key.h"
#include "../net.h"
#include "../script/standard.h"
#include "../validation.h"
#include "../wallet/coincontrol.h"
#include "../wallet/wallet.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "previewcodedialog.h"
#include "transactiondescdialog.h"
#include "transactiontablemodel.h"
#include "ui_bonuscodetab.h"
#include <QMessageBox>
#include <QTime>
#include <ctime>

BonusCodeTab::BonusCodeTab(WalletModel *wmodel_,
                           const PlatformStyle *platformStyle, QWidget *parent)
    : QWidget(parent), ui(new Ui::BonusCodeTab) {

    ui->setupUi(this);

    wmodel = wmodel_;
    this->platformStyle = platformStyle;
    this->setWindowFlags(this->windowFlags() &
                         ~Qt::WindowContextHelpButtonHint);

    tableInit(ui->CouponList);
    tableInit(ui->usedCouponList);

    ui->tab1->setCurrentIndex(0);

    ui->BCreate->setIcon(platformStyle->SingleColorIcon(":/icons/c_coupon"));
    ui->BReceive->setIcon(platformStyle->SingleColorIcon(":/icons/r_coupon"));
    ui->CouponId->setText(ui->CouponId->text() + ":");

    connect(ui->BCreate, SIGNAL(clicked(bool)), this, SLOT(createClick(bool)));
    connect(ui->BReceive, SIGNAL(clicked(bool)), this,
            SLOT(getBonusClick(bool)));
    connect(ui->tab1, SIGNAL(currentChanged(int)), this,
            SLOT(updateBonusList()));
}

void BonusCodeTab::clicked(QModelIndex i) {

    PreviewCodeDialog(i.model(), i.row(), this).exec();
}

void BonusCodeTab::resizeEvent(QResizeEvent *) {

    ui->tab1->setStyleSheet(
        QString("QTabBar::tab {width:%0;}").arg(this->width() / 2.1));
}

void BonusCodeTab::tableInit(QTableView *sourceTable) {

    QSortFilterProxyModel *model;
    sourceTable->setModel(model = new QSortFilterProxyModel(this));
    sourceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QStandardItemModel *couponModel = new QStandardItemModel;
    couponModel->setHorizontalHeaderLabels(
        QStringList() << tr("Date") << tr("Amount") << tr("Transaction hash")
                      << tr("KeyWord") << tr("status"));

    model->setSourceModel(couponModel);
    model->setDynamicSortFilter(true);
    model->setSortCaseSensitivity(Qt::CaseInsensitive);
    model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    model->setSortRole(Qt::EditRole);

    sourceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sourceTable->setAlternatingRowColors(true);
    sourceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    sourceTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    sourceTable->setSortingEnabled(true);
    sourceTable->sortByColumn(0, Qt::DescendingOrder);
    sourceTable->verticalHeader()->hide();
    sourceTable->horizontalHeader()->setSectionResizeMode(0,
                                                          QHeaderView::Fixed);
    sourceTable->horizontalHeader()->setSectionResizeMode(1,
                                                          QHeaderView::ResizeToContents);
    sourceTable->horizontalHeader()->setSectionResizeMode(2,
                                                          QHeaderView::Stretch);
    sourceTable->horizontalHeader()->setSectionResizeMode(3,
                                                          QHeaderView::Stretch);
    sourceTable->horizontalHeader()->setSectionResizeMode(4,
                                                          QHeaderView::Fixed);
    sourceTable->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter |
                                                         Qt::AlignVCenter);

    sourceTable->setColumnWidth(0, 140);
    sourceTable->setColumnWidth(1, 130);
    sourceTable->setColumnHidden(2, true);
    sourceTable->setColumnWidth(4, 130);
    sourceTable->setShowGrid(false);

    connect(sourceTable, SIGNAL(doubleClicked(QModelIndex)), this,
            SLOT(Clicked(QModelIndex)));
}

void BonusCodeTab::updateBonusList() {

    QStandardItemModel *model = static_cast<QStandardItemModel *>(
        (static_cast<QSortFilterProxyModel *>(ui->CouponList->model()))
            ->sourceModel());

    QStandardItemModel *usedmodel = static_cast<QStandardItemModel *>(
        (static_cast<QSortFilterProxyModel *>(ui->usedCouponList->model()))
            ->sourceModel());

    CWallet *pwalletMain = wmodel->getWallet();

    int unit = wmodel->getOptionsModel()->getDisplayUnit();

    model->removeRows(0, model->rowCount());
    usedmodel->removeRows(0, usedmodel->rowCount());
    for (Bonusinfoset::iterator i = pwalletMain->GetListOfBonusCodes().begin();
         i != pwalletMain->GetListOfBonusCodes().end(); i++) {

        bool fAvailable =
            pcoinsTip->HaveCoin(COutPoint(i->hashTx, i->getnVout()));

        const CWalletTx *tx(pwalletMain->GetWalletTx(i->hashTx));
        if (tx) {
            if (i->isUsed()) {
                usedmodel->insertRow(0);
                if (fAvailable)
                    usedmodel->setData(usedmodel->index(0, 4),
                                       tr("Unconfirmed"), Qt::DisplayRole);
                else
                    usedmodel->setData(usedmodel->index(0, 4), tr("Used"),
                                       Qt::DisplayRole);


                usedmodel->setData(usedmodel->index(0, 3),
                                   QString::fromStdString(i->key));

                usedmodel->setData(
                    usedmodel->index(0, 2),
                    QString::fromStdString(tx->GetHash().ToString()));

                usedmodel->setData(usedmodel->index(0, 1),
                                   BitcoinUnits::formatWithUnit(
                                       unit, tx->tx->vout[i->getnVout()].nValue,
                                       true, BitcoinUnits::separatorAlways));

                usedmodel->setData(usedmodel->index(0, 0),
                                   QDateTime::fromTime_t(tx->nTimeReceived));

                usedmodel->setData(usedmodel->index(0, 0), Qt::AlignCenter,
                                   Qt::TextAlignmentRole);
                usedmodel->setData(usedmodel->index(0, 1), Qt::AlignCenter,
                                   Qt::TextAlignmentRole);
                usedmodel->setData(usedmodel->index(0, 2), Qt::AlignCenter,
                                   Qt::TextAlignmentRole);
                usedmodel->setData(usedmodel->index(0, 3), Qt::AlignCenter,
                                   Qt::TextAlignmentRole);
                usedmodel->setData(usedmodel->index(0, 4), Qt::AlignCenter,
                                   Qt::TextAlignmentRole);

            } else {
                model->insertRow(0);
                if (fAvailable) {
                    model->setData(model->index(0, 4), tr("Unused"),
                                   Qt::DisplayRole);
                } else {

                    if (mempool.exists(tx->GetHash())) {
                        model->setData(model->index(0, 4), tr("Unconfirmed"),
                                       Qt::DisplayRole);
                    } else {
                        model->setData(model->index(0, 4), tr("Used"),
                                       Qt::DisplayRole);
                    }
                }

                model->setData(model->index(0, 3),
                               QString::fromStdString(i->key));
                model->setData(
                    model->index(0, 2),
                    QString::fromStdString(tx->GetHash().ToString()));
                model->setData(model->index(0, 1),
                               BitcoinUnits::formatWithUnit(
                                   unit, tx->tx->vout[i->getnVout()].nValue,
                                   true, BitcoinUnits::separatorAlways));
                model->setData(model->index(0, 0),
                               QDateTime::fromTime_t(tx->nTimeReceived));

                model->setData(model->index(0, 0), Qt::AlignCenter,
                               Qt::TextAlignmentRole);
                model->setData(model->index(0, 1), Qt::AlignCenter,
                               Qt::TextAlignmentRole);
                model->setData(model->index(0, 2), Qt::AlignCenter,
                               Qt::TextAlignmentRole);
                model->setData(model->index(0, 3), Qt::AlignCenter,
                               Qt::TextAlignmentRole);
                model->setData(model->index(0, 4), Qt::AlignCenter,
                               Qt::TextAlignmentRole);
            }
        }
    }
}

void BonusCodeTab::setWalletModel(WalletModel *wmodel) {

    this->wmodel = wmodel;
    connect(wmodel->getOptionsModel(), SIGNAL(displayUnitChanged(int)),
            SLOT(updateBonusList()));
}

void BonusCodeTab::setClientModel(ClientModel *clientmodel) {

    clientModel = clientmodel;
    connect(clientModel, SIGNAL(numBlocksChanged(int, QDateTime, double, bool)),
            this, SLOT(updateBonusList()));
}

void BonusCodeTab::getBonusClick(bool) {

    CWallet *pwalletMain = wmodel->getWallet();

    int unit = wmodel->getOptionsModel()->getDisplayUnit();

    std::string key_text = ui->EKey->text().toStdString();
    key_text.erase(std::remove(key_text.begin(), key_text.end(), ' '),
                   key_text.end());
    key_text.erase(0, 4);
    key_text.erase(std::remove(key_text.begin(), key_text.end(), '-'),
                   key_text.end());


    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(key_text);
    if (!fGood) {
        QMessageBox::warning(this, tr("Invalid key"),
                             tr("Check the key and try again."));
        ui->EKey->clear();
        return;
    }

    CKey key = vchSecret.GetKey();
    if (!key.IsValid()) {
        QMessageBox::warning(this, tr("Invalid key"),
                             tr("Check the key and try again."));
        ui->EKey->clear();
        return;
    }

    WalletModel::UnlockContext ctx(wmodel->requestUnlock());
    if (!ctx.isValid()) {
        // Unlock wallet was cancelled
        return;
    }


    CPubKey pubkey = key.GetPubKey();
    COutPoint point = pwalletMain->isAvailableCode(
        GetScriptForDestination(CTxDestination(pubkey.GetID())));
    if (point.IsNull()) {
        QMessageBox::information(this, tr("Code redemption"),
                                 tr("Bonus code is not available."));
        return;
    }
    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->AddKeyPubKey(key, pubkey)) {
            QMessageBox::warning(this, tr("Adding code"),
                                 tr("Error adding key to wallet."));
            return;
        }
    }

    CTransactionRef tx;
    uint256 hashBlock;

    bool isAccept;
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        isAccept = GetTransaction(point.hash, tx, Params().GetConsensus(),
                                  hashBlock, true) &&
                   pwalletMain->AddToWalletIfInvolvingMe(
                       tx, mapBlockIndex[hashBlock], 0, true);
    }

    if (isAccept) {
        QMessageBox::information(
            this, tr("Adding code"),
            tr("%0 Btx were received with this code.\n"
               "We recommend waiting for 3 transaction confirmations.")
                .arg(BitcoinUnits::formatWithUnit(
                    unit, tx->vout[point.n].nValue, true,
                    BitcoinUnits::separatorAlways)));

        confirmation(CBonusinfo(ui->EKey->text().toStdString(), tx->GetHash(),
                                point.n, true),
                     tx);
        updateBonusList();
    } else {

        QMessageBox::information(this, tr("Adding code"),
                                 tr("The bonus key was added to your wallet,"
                                    " but it was not possible to scan it."));
    }
    ui->EKey->clear();
}
void BonusCodeTab::createClick(bool) {

    CWallet *wallet = wmodel->getWallet();

    if (wallet->GetBalance() <= ui->SAmount->value()) {

        QMessageBox::information(
            this, tr("Creating code"),
            tr("You do not have the right amount in your account."));
        return;
    }

    WalletModel::UnlockContext ctx(wmodel->requestUnlock());
    if (!ctx.isValid()) {
        // Unlock wallet was cancelled
        return;
    }

    /***********************generate a key ******************************/

    std::string showkey = "BTX";

    CKey Key;
    Key.MakeNewKey(false);
    if (!Key.IsValid()) {
        QMessageBox::warning(this, tr("Creating code"), tr("Key create fail"));
        return;
    }

    std::string temp = CBitcoinSecret(Key).ToString();
    for (unsigned int i = 0; i < temp.size(); i += 10) {
        temp.insert(i, "-");
    }
    showkey += temp;

    /********************create a new transaction*************************/
    std::vector<CRecipient> Recipient;
    CRecipient rec;
    rec.scriptPubKey =
        GetScriptForDestination(CTxDestination(Key.GetPubKey().GetID()));
    rec.nAmount = ui->SAmount->value();
    rec.fSubtractFeeFromAmount = false;
    Recipient.push_back(rec);

    CWalletTx wtx;
    CReserveKey Rkey(wallet);
    std::string fall;
    CAmount nFeeRet = 1;
    CCoinControl control;
    int nChangePosInOut = 0;
    if (wallet->CreateTransaction(Recipient, wtx, Rkey, nFeeRet,
                                  nChangePosInOut, fall, control)) {

        CValidationState state;
        if (wallet->CommitTransaction(wtx, Rkey, g_connman.get(), state)) {

            unsigned int i = 0;
            while (wtx.tx->vout.size() != i &&
                   wtx.tx->vout[i].scriptPubKey != rec.scriptPubKey)
                ++i;
            if (i == wtx.tx->vout.size()) {
                QMessageBox::warning(this, tr("Creating code"),
                                     tr("Code create fail"));
                return;
            }
            wallet->AddBonusKey(CBonusinfo(showkey, wtx.GetHash(), i));
            updateBonusList();
            QMessageBox::information(this, tr("Creating code"),
                                     tr("Your code is created. The code will "
                                        "be available after confirmation."));

        } else {

            QMessageBox::warning(
                this, tr("Creating code"),
                tr("The transaction was rejected! This might happen if some of "
                   "the coins in your wallet were already spent, such as if "
                   "you used a copy of wallet.dat and coins were spent in the "
                   "copy but not marked as spent here."));

            if (QMessageBox::Yes ==
                QMessageBox::question(
                    this, tr("Solution"),
                    tr("In order to solve this problem, you need to rescan "
                       "your wallet. Scanning a wallet will take some time "
                       "want to continue?"))) {
                wallet->ScanForWalletTransactions(chainActive.Genesis(), true);
            }
        }
    } else {
        QMessageBox::warning(this, tr("Creating code"),
                             tr("Code create fail."));
    }
}

void BonusCodeTab::confirmation(const CBonusinfo &info,
                                const CTransactionRef &prevtx) {

    CWallet *wallet = wmodel->getWallet();

    std::vector<CRecipient> Recipient;
    CRecipient rec;
    CPubKey key;
    CCoinControl control;
    control.Select(COutPoint(info.hashTx, info.getnVout()));
    wallet->GetKeyFromPool(key);

    rec.scriptPubKey =
        GetScriptForDestination(CBitcoinAddress(key.GetID()).Get());
    rec.nAmount = prevtx->vout[info.getnVout()].nValue;
    rec.fSubtractFeeFromAmount = true;
    Recipient.push_back(rec);

    CWalletTx wtx;
    CReserveKey Rkey(wallet);
    std::string fall;
    CAmount nFeeRet = 1;
    int nChangePosInOut = 0;
    wtx.mapValue["comment"] =
        tr("Commission for the confirmation of the bonus code.").toStdString();
    wtx.mapValue["bonusConfirmation"] = info.key;
    CValidationState state;

    if (!(wallet->CreateTransaction(Recipient, wtx, Rkey, nFeeRet,
                                    nChangePosInOut, fall, control) &&
          wallet->CommitTransaction(wtx, Rkey, g_connman.get(), state))) {

        QMessageBox::warning(this, tr("confirmation"),
                             tr("The key is no confirmed."));
        return;
    }
    wallet->AddBonusKey(info);
}

BonusCodeTab::~BonusCodeTab() { delete ui; }
