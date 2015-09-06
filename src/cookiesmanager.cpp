#include "cookiesmanager.h"

//-------------------------------------------------------------------------------------------------
DateTimeEditEx::DateTimeEditEx(QRadioButton *rb, QWidget *parent) : QDateTimeEdit(parent),
    rbFocus(rb)
{
    this->setDisplayFormat("yyyy/MM/dd hh:mm:ss");
    this->setDateTimeRange(QDateTime::fromMSecsSinceEpoch(0), QDateTime::fromMSecsSinceEpoch(UINT_MAX*1000LL));
}

//-------------------------------------------------------------------------------------------------
void DateTimeEditEx::focusInEvent(QFocusEvent *event)
{
    rbFocus->setChecked(true);
    this->QDateTimeEdit::focusInEvent(event);
}

//-------------------------------------------------------------------------------------------------
SpinBoxEx::SpinBoxEx(const qint64 iValueTime, QRadioButton *rb, QWidget *parent) : QAbstractSpinBox(parent),
    rbFocus(rb),
    iValue(iValueTime)
{
    QLineEdit *le = this->lineEdit();
    le->setText(QString::number(iValueTime));

    //connects
    connect(le, SIGNAL(textChanged(QString)), this, SLOT(slotEdited(QString)));
}

//-------------------------------------------------------------------------------------------------
QAbstractSpinBox::StepEnabled SpinBoxEx::stepEnabled() const
{
    return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
}

//-------------------------------------------------------------------------------------------------
void SpinBoxEx::stepBy(int steps)
{
    iValue += steps;
    if (iValue > UINT_MAX)
        iValue = UINT_MAX;
    else if (iValue < 0)
        iValue = 0;
    this->lineEdit()->setText(QString::number(iValue));
}

//-------------------------------------------------------------------------------------------------
QValidator::State SpinBoxEx::validate(QString &input, int &) const
{
    bool bOk;
    input.toULong(&bOk);
    return (bOk ? QValidator::Acceptable : QValidator::Invalid);
}

//-------------------------------------------------------------------------------------------------
void SpinBoxEx::focusInEvent(QFocusEvent *event)
{
    rbFocus->setChecked(true);
    this->QAbstractSpinBox::focusInEvent(event);
}

//-------------------------------------------------------------------------------------------------
void SpinBoxEx::slotEdited(const QString &strText)
{
#ifdef QT_DEBUG
    bool bOk;
    iValue = strText.toLongLong(&bOk);
    Q_ASSERT(bOk);
    Q_ASSERT(iValue >= 0 && iValue <= UINT_MAX);
#else
    iValue = strText.toLongLong();
#endif
}

//-------------------------------------------------------------------------------------------------
TreeViewEx::StyledItemDelegateEx::StyledItemDelegateEx(QObject *parent) : QStyledItemDelegate(parent),
    penLight(QColor(200, 200, 200)),
    penDark(QColor(84, 84, 84))
{

}

//-------------------------------------------------------------------------------------------------
void TreeViewEx::StyledItemDelegateEx::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column())
    {
        QStyleOptionViewItem optionIconCenter = option;
        optionIconCenter.decorationPosition = QStyleOptionViewItem::Top;
        this->QStyledItemDelegate::paint(painter, optionIconCenter, index);
    }
    else
        this->QStyledItemDelegate::paint(painter, option, index);

    const QRect &rect = option.rect;
    if (index.data(Qt::UserRole + 1) == -1)        //eToggleDisable (if row is Group Name)
    {
        painter->setPen(penLight);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        painter->setPen(penDark);
        painter->drawLine(rect.topLeft(), rect.topRight());
    }
    else
        painter->setPen(penDark);
    painter->drawLine(rect.topRight(), rect.bottomRight());
}

//-------------------------------------------------------------------------------------------------
TreeViewEx::TreeViewEx(CookiesManager *parent) : QTreeView(parent),
    pApplication(parent)
{
    this->setFont(QFont("Trebuchet MS", 9));
    this->setItemDelegate(new StyledItemDelegateEx(this));
    this->setAcceptDrops(true);
}

//-------------------------------------------------------------------------------------------------
void TreeViewEx::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        if (QFileInfo(event->mimeData()->urls().first().toLocalFile()).isFile())
            event->acceptProposedAction();
    }
#ifdef QT_DEBUG
    else
        Q_ASSERT(false);
#endif
}

//-------------------------------------------------------------------------------------------------
void TreeViewEx::dragMoveEvent(QDragMoveEvent *)
{

}

//-------------------------------------------------------------------------------------------------
void TreeViewEx::dropEvent(QDropEvent *event)
{
    Q_ASSERT(event->mimeData()->hasUrls());
    Q_ASSERT(QFileInfo(event->mimeData()->urls().first().toLocalFile()).isFile());
    pApplication->slotNewCfg();
    pApplication->fOpenCfg(event->mimeData()->urls().first().toLocalFile());
}

//-------------------------------------------------------------------------------------------------
CookiesManager::CookiesManager() : QWidget(),
    sitemModel(new QStandardItemModel(this)),
    bDefIsHttpOnly(true),
    bDefIsSecure(false),
    bDefIsSession(false),
    bDefExpandAll(true),
    dtDefExpire(QDate(QDate::currentDate().addYears(5).year(), 1, 1))
{
    Q_ASSERT(sizeof(qint64) == sizeof(qlonglong));

    const QString strAppName = qAppName(),
            strAppDir = qApp->applicationDirPath();
    QTranslator *translator = new QTranslator(this);
    if (translator->load(strAppName, strAppDir) || translator->load(strAppName + '_' + QLocale::system().name(), strAppDir))
        qApp->installTranslator(translator);

    QPushButton *pbNewCfg = new QPushButton(QIcon(":/img/new.png"), 0, this);
    pbNewCfg->setToolTip(tr("Create New File"));
    QPushButton *pbOpenCfg = new QPushButton(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open"), this);
    pbSaveCfg = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), this);
    pbSaveAsCfg = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton), 0, this);
    pbSaveAsCfg->setToolTip(tr("Save As..."));
    lblInfo = new QLabel(this);
    QFont fontSaved = lblInfo->font();
    fontSaved.setItalic(true);
    lblInfo->setFont(fontSaved);
    lblInfo->setFixedWidth(lblInfo->fontMetrics().width(tr("Saved successfully")) + 5);
    QPushButton *pbNewGroup = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogNewFolder), tr("New Group"), this);
    pbNewRecord = new QPushButton(QIcon(":/img/record.png"), tr("New Record"), this);
    pbMoveDown = new QPushButton(style()->standardIcon(QStyle::SP_ArrowDown), 0, this);
    pbMoveDown->setToolTip(tr("Move Down"));
    pbMoveUp = new QPushButton(style()->standardIcon(QStyle::SP_ArrowUp), 0, this);
    pbMoveUp->setToolTip(tr("Move Up"));
    pbDelete = new QPushButton(style()->standardIcon(QStyle::SP_DialogCancelButton), 0, this);
    pbDelete->setToolTip(tr("Delete"));
    pbCollapse = new QPushButton(QIcon(":/img/collapse.png"), 0, this);
    pbCollapse->setToolTip(tr("Collapse All"));
    pbExpand = new QPushButton(QIcon(":/img/expand.png"), 0, this);
    pbExpand->setToolTip(tr("Expand All"));
    QPushButton *pbSettings = new QPushButton(QIcon(":/img/settings.png"), 0, this);
    pbSettings->setToolTip(tr("Settings"));
    fButtonsToggle(false);
    QHBoxLayout *hblTop = new QHBoxLayout;
    hblTop->setSpacing(2);
    hblTop->addWidget(pbNewCfg);
    hblTop->addWidget(pbOpenCfg);
    hblTop->addWidget(pbSaveCfg);
    hblTop->addWidget(pbSaveAsCfg);
    hblTop->addWidget(lblInfo);
    hblTop->addStretch();
    hblTop->addWidget(pbNewGroup);
    hblTop->addWidget(pbNewRecord);
    hblTop->addWidget(pbMoveDown);
    hblTop->addWidget(pbMoveUp);
    hblTop->addWidget(pbDelete);
    hblTop->addSpacing(30);
    hblTop->addStretch();
    hblTop->addWidget(pbCollapse);
    hblTop->addWidget(pbExpand);
    hblTop->addWidget(pbSettings);

    trViewEx = new TreeViewEx(this);
    trViewEx->setModel(sitemModel);

    QVBoxLayout *vblMain = new QVBoxLayout(this);
    vblMain->setContentsMargins(5, 5, 5, 5);
    vblMain->addLayout(hblTop);
    vblMain->addWidget(trViewEx);

    sitemModel->setHorizontalHeaderLabels(QStringList(tr("Host"))
                                          << "IsHttpOnly"
                                          << tr("Path")
                                          << "IsSecure"
                                          << "IsSession"
                                          << tr("Expire")
                                          << tr("Name")
                                          << tr("Value"));
    itemSelModel = trViewEx->selectionModel();
    this->setWindowTitle(strAppName + " [?]");

    //connects
    connect(pbNewCfg, SIGNAL(clicked()), this, SLOT(slotNewCfg()));
    connect(pbOpenCfg, SIGNAL(clicked()), this, SLOT(slotOpenCfg()));
    connect(pbSaveCfg, SIGNAL(clicked()), this, SLOT(slotSaveCfg()));
    connect(pbSaveAsCfg, SIGNAL(clicked()), this, SLOT(slotSaveAsCfg()));
    connect(pbNewGroup, SIGNAL(clicked()), this, SLOT(slotNewGroup()));
    connect(pbNewRecord, SIGNAL(clicked()), this, SLOT(slotNewRecord()));
    connect(pbMoveDown, SIGNAL(clicked()), this, SLOT(slotMoveDown()));
    connect(pbMoveUp, SIGNAL(clicked()), this, SLOT(slotMoveUp()));
    connect(pbDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
    connect(pbCollapse, SIGNAL(clicked()), this, SLOT(slotCollapseAll()));
    connect(pbExpand, SIGNAL(clicked()), this, SLOT(slotExpandAll()));
    connect(pbSettings, SIGNAL(clicked()), this, SLOT(slotShowSettings()));
    connect(trViewEx, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotEdit(QModelIndex)));

    //settings
    QSettings stgIni(strAppDir + '/' + strAppName + ".ini", QSettings::IniFormat);
    if (stgIni.childGroups().contains("Settings"))
    {
        stgIni.beginGroup("Settings");
        if (stgIni.value("IsHttpOnly").toString() == "0")
            bDefIsHttpOnly = false;
        if (stgIni.value("IsSecure").toString() == "1")
            bDefIsSecure = true;
        if (stgIni.value("IsSession").toString() == "1")
            bDefIsSession = true;
        if (stgIni.value("ExpandAll").toString() == "0")
            bDefExpandAll = false;
        const qlonglong iExpire = stgIni.value("Expire", -1).toLongLong();
        if (iExpire >= 0 && iExpire <= UINT_MAX)
            dtDefExpire = QDateTime::fromMSecsSinceEpoch(iExpire*1000LL);
        stgIni.endGroup();
    }

    const QSettings stg("UserPrograms", strAppName);
    if (this->restoreGeometry(stg.value("Geometry").toByteArray()))
    {
        const QStringList slistSizes = stg.value("SizesOfColumns").toString().split('|');
        if (slistSizes.size() == eHeaderValue)
        {
            QHeaderView *headerView = trViewEx->header();
            for (int i = eHeaderHost; i < eHeaderValue; ++i)
            {
                const int iSizeColumn = slistSizes.at(i).toInt();
                if (iSizeColumn > 0)
                    headerView->resizeSection(i, iSizeColumn);
            }
        }
    }

    //arguments
    const QStringList slistArgs = qApp->arguments();
    if (slistArgs.size() > 1 && QFileInfo(slistArgs.at(1)).isFile())
        fOpenCfg(slistArgs.at(1));
}

//-------------------------------------------------------------------------------------------------
CookiesManager::~CookiesManager()
{
    const QHeaderView *headerView = trViewEx->header();
    QString strSizes;
    for (int i = eHeaderHost; i < eHeaderValue; ++i)
        strSizes += QString::number(headerView->sectionSize(i)) + '|';
    strSizes.chop(1);
    QSettings stg("UserPrograms", qAppName());
    stg.setValue("SizesOfColumns", strSizes);
    stg.setValue("Geometry", this->saveGeometry());
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::fButtonsToggle(const bool bToggle) const
{
    pbSaveCfg->setEnabled(bToggle);
    pbSaveAsCfg->setEnabled(bToggle);
    pbNewRecord->setEnabled(bToggle);
    pbMoveDown->setEnabled(bToggle);
    pbMoveUp->setEnabled(bToggle);
    pbDelete->setEnabled(bToggle);
    pbCollapse->setEnabled(bToggle);
    pbExpand->setEnabled(bToggle);
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotNewCfg()
{
    const int iRowCount = sitemModel->rowCount();
    if (iRowCount > 0)
        sitemModel->removeRows(0, iRowCount);
    strCurrentCfg.clear();
    this->setWindowTitle(qAppName() + " [?]");
    fButtonsToggle(false);
}

//-------------------------------------------------------------------------------------------------
bool CookiesManager::fHasErrors(const QString &str) const
{
    const ushort *pStr= str.utf16();
    if (*pStr == L'\0')        //empty string
        return true;
    while (*pStr)
    {
        if (!(*pStr >= L' ' && *pStr <= L'~'))
            return true;
        ++pStr;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::fOpenCfg(const QString &strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, 0, tr("Can't open file:") + '\n' + file.fileName());
        return;
    }

    const QStringList slistRows = QString(file.readAll()).split('\n', QString::SkipEmptyParts);
    if (file.error())
    {
        QMessageBox::warning(this, 0, file.errorString());
        return;
    }

    file.close();
    QString strErrors;
    QStandardItem *sitemGroupName = 0;
    for (int i = 0; i < slistRows.size(); ++i)
    {
        if (slistRows.at(i).at(0) == '#')
        {
            const QString strGroupName = slistRows.at(i).mid(1);
            if (fHasErrors(strGroupName))
            {
                sitemGroupName = 0;
                strErrors += '\n' + tr("Incorrect group name #") + QString::number(i+1);
            }
            else
            {
                QList<QStandardItem*> listGroup;
                sitemGroupName = new QStandardItem(style()->standardIcon(QStyle::SP_FileDialogNewFolder), strGroupName);
                sitemGroupName->setData(eToggleDisable);
                listGroup.append(sitemGroupName);
                for (int j = 0; j < eRecordSize; ++j)
                {
                    QStandardItem *sitemBlank = new QStandardItem;
                    sitemBlank->setData(eToggleDisable);
                    sitemBlank->setEditable(false);
                    listGroup.append(sitemBlank);
                }
                sitemModel->appendRow(listGroup);
            }
        }
        else if (sitemGroupName)
        {
            const QStringList slistFields = slistRows.at(i).split('\t');
            if (slistFields.size() == eRecordSize)
            {
                const QString &strHost = slistFields.at(eHost);
                if (!fHasErrors(strHost) && strHost.lastIndexOf('.') > 0)
                {
                    bool bIsHttpOnly = true,
                            bOk = false;
                    if (slistFields.at(eIsHttpOnly) == "TRUE")
                        bOk = true;
                    else if (slistFields.at(eIsHttpOnly) == "FALSE")
                    {
                        bIsHttpOnly = false;
                        bOk = true;
                    }
                    if (bOk)
                    {
                        const QString &strPath = slistFields.at(ePath);
                        if (!fHasErrors(strPath) && strPath.at(0) == '/')
                        {
                            bool bIsSecure = false;
                            bOk = false;
                            if (slistFields.at(eIsSecure) == "FALSE")
                                bOk = true;
                            else if (slistFields.at(eIsSecure) == "TRUE")
                            {
                                bIsSecure = true;
                                bOk = true;
                            }
                            if (bOk)
                            {
                                const uint iExpire = slistFields.at(eExpire).toUInt(&bOk);
                                if (bOk)
                                {
                                    const QString &strName = slistFields.at(eName);
                                    if (!fHasErrors(strName))
                                    {
                                        const QString &strValue = slistFields.at(eValue);
                                        if (!fHasErrors(strValue))
                                        {
                                            QList<QStandardItem*> listRecord;
                                            listRecord.reserve(eRecordSize);

                                            listRecord.append(new QStandardItem(QIcon(":/img/record.png"), strHost));

                                            QStandardItem *sitemIsHttpOnly = new QStandardItem;
                                            if (bIsHttpOnly)
                                            {
                                                sitemIsHttpOnly->setIcon(QIcon(":/img/http.png"));
                                                sitemIsHttpOnly->setData(eToggleOn);
                                            }
                                            sitemIsHttpOnly->setEditable(false);
                                            listRecord.append(sitemIsHttpOnly);

                                            listRecord.append(new QStandardItem(strPath));

                                            QStandardItem *sitemIsSecure = new QStandardItem;
                                            if (bIsSecure)
                                            {
                                                sitemIsSecure->setIcon(QIcon(":/img/secure.png"));
                                                sitemIsSecure->setData(eToggleOn);
                                            }
                                            sitemIsSecure->setEditable(false);
                                            listRecord.append(sitemIsSecure);

                                            QStandardItem *sitemIsSession = new QStandardItem;
                                            QStandardItem *sitemExpire = new QStandardItem(QDateTime(iExpire ? QDateTime::fromMSecsSinceEpoch(iExpire*1000LL) : dtDefExpire).toString("yyyy/MM/dd hh:mm:ss"));
                                            if (!iExpire)
                                            {
                                                sitemIsSession->setIcon(QIcon(":/img/session.png"));
                                                sitemIsSession->setData(eToggleOn);
                                                sitemExpire->setEnabled(false);
                                            }
                                            sitemIsSession->setEditable(false);
                                            sitemExpire->setEditable(false);
                                            listRecord.append(sitemIsSession);
                                            listRecord.append(sitemExpire);

                                            listRecord.append(new QStandardItem(strName));

                                            listRecord.append(new QStandardItem(strValue));

                                            sitemGroupName->appendRow(listRecord);
                                        }
                                        else
                                            strErrors += '\n' + tr("Incorrect value #") + QString::number(i+1);
                                    }
                                    else
                                        strErrors += '\n' + tr("Incorrect name #") + QString::number(i+1);
                                }
                                else
                                    strErrors += '\n' + tr("Incorrect expire date #") + QString::number(i+1);
                            }
                            else
                                strErrors += '\n' + tr("Incorrect IsSecure value #") + QString::number(i+1);
                        }
                        else
                            strErrors += '\n' + tr("Incorrect path #") + QString::number(i+1);
                    }
                    else
                        strErrors += '\n' + tr("Incorrect IsHttpOnly value #") + QString::number(i+1);
                }
                else
                    strErrors += '\n' + tr("Incorrect host #") + QString::number(i+1);
            }
            else
                strErrors += '\n' + tr("Incorrect number of fields #") + QString::number(i+1);
        }
        else
            strErrors += '\n' + tr("Pass row #") + QString::number(i+1);
    }

    strCurrentCfg = QDir::toNativeSeparators(strPath);
    this->setWindowTitle(qAppName() + " [" + strCurrentCfg + ']');
    if (sitemModel->hasChildren())
    {
        fButtonsToggle(true);
        if (bDefExpandAll)
            trViewEx->expandAll();
    }
    if (!strErrors.isEmpty())
        QMessageBox::warning(this, 0, tr("Following errors:") + strErrors);
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotOpenCfg()
{
    const QString strPath = QFileDialog::getOpenFileName(this, 0, 0, "Config (*.cfg);;All files (*.*)");
    if (!strPath.isEmpty())
    {
        slotNewCfg();
        fOpenCfg(strPath);
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::fSaveCfg(const QString &strPath)
{
    const int iGroupCount = sitemModel->rowCount();
    Q_ASSERT(iGroupCount > 0);
    const QStandardItem *const sitemRoot = sitemModel->invisibleRootItem();
    QVector<const QStandardItem*> vectGroups(iGroupCount);
    QVector<QString> vectGroupNames(iGroupCount);
    for (int i = 0; i < iGroupCount; ++i)
    {
        vectGroupNames[i] = (vectGroups[i] = sitemRoot->child(i))->text();
        const QString &strGroupName = vectGroupNames.at(i);
        if (fHasErrors(strGroupName))
        {
            QMessageBox::warning(this, 0, tr("Incorrect group name #") + QString::number(i+1));
            return;
        }
        for (int j = 0; j < i; ++j)
        {
            if (strGroupName == vectGroupNames.at(j))
            {
                QMessageBox::warning(this, 0, tr("Group name meet twice #") + QString::number(i+1));
                return;
            }
        }
    }

    QString strToFile;
    for (int i = 0; i < iGroupCount; ++i)
    {
        strToFile += '#' + vectGroupNames.at(i) + '\n';
        const QStandardItem *const sitemGroup = vectGroups.at(i);
        for (int j = 0, iRecords = sitemGroup->rowCount(); j < iRecords; ++j)
        {
            const QString strHost = sitemGroup->child(j, eHeaderHost)->text();
            if (fHasErrors(strHost) || strHost.lastIndexOf('.') < 1)
            {
                QMessageBox::warning(this, 0, tr("Incorrect host #") + QString::number(i+1) + '/' + QString::number(j+1));
                return;
            }

            const QString strPath = sitemGroup->child(j, eHeaderPath)->text();
            if (fHasErrors(strPath) || strPath.at(0) != '/')
            {
                QMessageBox::warning(this, 0, tr("Incorrect path #") + QString::number(i+1) + '/' + QString::number(j+1));
                return;
            }

            const QString strName = sitemGroup->child(j, eHeaderName)->text();
            if (fHasErrors(strName))
            {
                QMessageBox::warning(this, 0, tr("Incorrect name #") + QString::number(i+1) + '/' + QString::number(j+1));
                return;
            }

            const QString strValue = sitemGroup->child(j, eHeaderValue)->text();
            if (fHasErrors(strValue))
            {
                QMessageBox::warning(this, 0, tr("Incorrect value #") + QString::number(i+1) + '/' + QString::number(j+1));
                return;
            }

            strToFile += strHost +
                    (sitemGroup->child(j, eHeaderIsHttpOnly)->data().toBool() ? "\tTRUE\t" : "\tFALSE\t") +
                    strPath +
                    (sitemGroup->child(j, eHeaderIsSecure)->data().toBool() ? "\tTRUE\t" : "\tFALSE\t") +
                    (sitemGroup->child(j, eHeaderIsSession)->data().toBool() ? "0" : QString::number(QDateTime::fromString(sitemGroup->child(j, eHeaderExpire)->text(), "yyyy/MM/dd hh:mm:ss").toMSecsSinceEpoch()/1000LL)) + '\t' +
                    strName + '\t' +
                    strValue + '\n';
        }
    }
    strToFile.chop(1);

    QFile file(strPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.write(strToFile.toUtf8());
        if (file.error())
            QMessageBox::warning(this, 0, file.errorString());
        else
        {
            strCurrentCfg = QDir::toNativeSeparators(strPath);
            this->setWindowTitle(qAppName() + " [" + strCurrentCfg + ']');
            lblInfo->setText(tr("Saved successfully"));
            QTimer::singleShot(1000, lblInfo, SLOT(clear()));
        }
    }
    else
        QMessageBox::warning(this, 0, tr("Can't create file"));
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotSaveCfg()
{
    Q_ASSERT(sitemModel->hasChildren());
    if (strCurrentCfg.isEmpty())
        slotSaveAsCfg();
    else
        fSaveCfg(strCurrentCfg);
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotSaveAsCfg()
{
    Q_ASSERT(sitemModel->hasChildren());
    const QString strPath = QFileDialog::getSaveFileName(this, 0, 0, "Config (*.cfg);;All files (*.*)");
    if (!strPath.isEmpty())
        fSaveCfg(strPath);
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotNewGroup()
{
    const QString strNewGroup = QInputDialog::getText(this, 0, tr("New Group:"), QLineEdit::Normal, 0, 0, Qt::WindowCloseButtonHint);
    if (!strNewGroup.isEmpty())
    {
        QList<QStandardItem*> listGroupName;
        QStandardItem *sitemGroupName = new QStandardItem(style()->standardIcon(QStyle::SP_FileDialogNewFolder), strNewGroup);
        sitemGroupName->setData(eToggleDisable);
        listGroupName.append(sitemGroupName);
        for (int i = 0; i < eHeaderValue; ++i)
        {
            QStandardItem *sitemFilled = new QStandardItem;
            sitemFilled->setData(eToggleDisable);
            sitemFilled->setEditable(false);
            listGroupName.append(sitemFilled);
        }
        sitemModel->appendRow(listGroupName);
        fButtonsToggle(true);
        itemSelModel->select(QItemSelection(sitemModel->indexFromItem(sitemGroupName), sitemModel->indexFromItem(listGroupName.at(eHeaderValue))),
                             QItemSelectionModel::ClearAndSelect);
        trViewEx->scrollToBottom();
        trViewEx->setFocus();
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotNewRecord() const
{
    Q_ASSERT(sitemModel->hasChildren());
    const QModelIndexList modIndList = itemSelModel->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        QList<QStandardItem*> listRecord;

        QStandardItem *sitemHost = new QStandardItem(QIcon(":/img/record.png"), 0);
        listRecord.append(sitemHost);

        QStandardItem *sitemIsHttpOnly = new QStandardItem;
        if (bDefIsHttpOnly)
        {
            sitemIsHttpOnly->setIcon(QIcon(":/img/http.png"));
            sitemIsHttpOnly->setData(eToggleOn);
        }
        sitemIsHttpOnly->setEditable(false);
        listRecord.append(sitemIsHttpOnly);

        listRecord.append(new QStandardItem("/"));

        QStandardItem *sitemIsSecure = new QStandardItem;
        if (bDefIsSecure)
        {
            sitemIsSecure->setIcon(QIcon(":/img/secure.png"));
            sitemIsSecure->setData(eToggleOn);
        }
        sitemIsSecure->setEditable(false);
        listRecord.append(sitemIsSecure);

        QStandardItem *sitemIsSession = new QStandardItem;
        QStandardItem *sitemExpire = new QStandardItem(dtDefExpire.toString("yyyy/MM/dd hh:mm:ss"));
        if (bDefIsSession)
        {
            sitemIsSession->setIcon(QIcon(":/img/session.png"));
            sitemIsSession->setData(eToggleOn);
            sitemExpire->setEnabled(false);
        }
        sitemIsSession->setEditable(false);
        sitemExpire->setEditable(false);
        listRecord.append(sitemIsSession);
        listRecord.append(sitemExpire);

        listRecord.append(new QStandardItem);

        listRecord.append(new QStandardItem);

        const QModelIndex &modInd = modIndList.first();
        const QModelIndex modIndParent = modInd.parent();
        if (modIndParent.isValid())
            sitemModel->itemFromIndex(modIndParent)->appendRow(listRecord);
        else
        {
            sitemModel->itemFromIndex(modInd)->appendRow(listRecord);
            trViewEx->expand(modInd);
        }
        const QModelIndex modIndHost = sitemModel->indexFromItem(sitemHost);
        itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listRecord.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
        trViewEx->scrollTo(modIndHost);
        trViewEx->setFocus();
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotMoveDown() const
{
    Q_ASSERT(sitemModel->hasChildren());
    const QModelIndexList modIndList = itemSelModel->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        const QModelIndex &modInd = modIndList.first();
        const QModelIndex modIndParent = modInd.parent();
        const int iRow = modInd.row();
        if (modIndParent.isValid())        //move record
        {
            QStandardItem *sitem = sitemModel->itemFromIndex(modIndParent);
            if (iRow < sitem->rowCount()-1)        //NOT last record in the group
            {
                const QList<QStandardItem*> listTakes = sitem->takeRow(iRow);
                sitem->insertRow(iRow+1, listTakes);
                trViewEx->expand(modIndParent);
                const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                trViewEx->scrollTo(modIndHost);
                trViewEx->setFocus();
            }
            else        //latest entry in the group (moving to the next group)
            {
                const int iGroupRow = modIndParent.row();
                if (iGroupRow < sitemModel->rowCount()-1)        //group didn't last
                {
                    const QModelIndex modIndNextGroup = sitemModel->index(iGroupRow+1, 0);
                    const QList<QStandardItem*> listTakes = sitem->takeRow(iRow);
                    sitemModel->itemFromIndex(modIndNextGroup)->insertRow(0, listTakes);
                    trViewEx->expand(modIndNextGroup);
                    const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                    itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                    trViewEx->scrollTo(modIndHost);
                    trViewEx->setFocus();
                }
            }
        }
        else        //move group
        {
            if (iRow < sitemModel->rowCount()-1)
            {
                const bool bExpand = trViewEx->isExpanded(modInd);
                const QList<QStandardItem*> listTakes = sitemModel->takeRow(iRow);
                sitemModel->insertRow(iRow+1, listTakes);
                const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                if (bExpand)
                    trViewEx->expand(modIndHost);
                itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                trViewEx->scrollTo(modIndHost);
                trViewEx->setFocus();
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotMoveUp() const
{
    Q_ASSERT(sitemModel->hasChildren());
    const QModelIndexList modIndList = itemSelModel->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        const QModelIndex &modInd = modIndList.first();
        const QModelIndex modIndParent = modInd.parent();
        const int iRow = modInd.row();
        if (modIndParent.isValid())        //move record
        {
            QStandardItem *sitem = sitemModel->itemFromIndex(modIndParent);
            if (iRow > 0)        //NOT first record in the group
            {
                const QList<QStandardItem*> listTakes = sitem->takeRow(iRow);
                sitem->insertRow(iRow-1, listTakes);
                trViewEx->expand(modIndParent);
                const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                trViewEx->scrollTo(modIndHost);
                trViewEx->setFocus();
            }
            else        //first entry in the group (moving to the previous group)
            {
                const int iGroupRow = modIndParent.row();
                if (iGroupRow > 0)        //group didn't first
                {
                    const QModelIndex modIndPreviousGroup = sitemModel->index(iGroupRow-1, 0);
                    const QList<QStandardItem*> listTakes = sitem->takeRow(iRow);
                    sitemModel->itemFromIndex(modIndPreviousGroup)->appendRow(listTakes);
                    trViewEx->expand(modIndPreviousGroup);
                    const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                    itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                    trViewEx->scrollTo(modIndHost);
                    trViewEx->setFocus();
                }
            }
        }
        else        //move group
        {
            if (iRow > 0)
            {
                const bool bExpand = trViewEx->isExpanded(modInd);
                const QList<QStandardItem*> listTakes = sitemModel->takeRow(iRow);
                sitemModel->insertRow(iRow-1, listTakes);
                const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                if (bExpand)
                    trViewEx->expand(modIndHost);
                itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(eHeaderValue))), QItemSelectionModel::ClearAndSelect);
                trViewEx->scrollTo(modIndHost);
                trViewEx->setFocus();
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotDelete() const
{
    Q_ASSERT(sitemModel->hasChildren());
    const QModelIndexList modIndList = itemSelModel->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        const QModelIndex &modInd = modIndList.first();
#ifdef QT_DEBUG
        Q_ASSERT(sitemModel->removeRow(modInd.row(), modInd.parent()));
#else
        sitemModel->removeRow(modInd.row(), modInd.parent());
#endif
        if (!sitemModel->hasChildren())
            fButtonsToggle(false);
        trViewEx->setFocus();
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotShowSettings()
{
    QDialog dialSettings(this, Qt::WindowCloseButtonHint);

    QLabel *lblDefault = new QLabel("----- " + tr("Default") + " -----", &dialSettings);
    QCheckBox *chbIsHttpOnly = new QCheckBox("IsHttpOnly", &dialSettings);
    if (bDefIsHttpOnly)
        chbIsHttpOnly->setChecked(true);
    QCheckBox *chbIsSecure = new QCheckBox("IsSecure", &dialSettings);
    if (bDefIsSecure)
        chbIsSecure->setChecked(true);
    QCheckBox *chbIsSession = new QCheckBox("IsSession", &dialSettings);
    if (bDefIsSession)
        chbIsSession->setChecked(true);
    QCheckBox *chbExpandAll = new QCheckBox(tr("Expand All"), &dialSettings);
    if (bDefExpandAll)
        chbExpandAll->setChecked(true);
    QLabel *lblExpire = new QLabel(tr("Expire:"), &dialSettings);
    QDateTimeEdit *dteExpire = new QDateTimeEdit(&dialSettings);
    dteExpire->setDisplayFormat("yyyy/MM/dd hh:mm:ss");
    dteExpire->setDateTimeRange(QDateTime::fromMSecsSinceEpoch(0), QDateTime::fromMSecsSinceEpoch(UINT_MAX*1000LL));
    dteExpire->setDateTime(dtDefExpire);
    QPushButton *pbOk = new QPushButton("OK", &dialSettings);
    QVBoxLayout *vblSettings = new QVBoxLayout(&dialSettings);
    vblSettings->setContentsMargins(5, 5, 5, 5);
    vblSettings->setSpacing(2);
    vblSettings->addWidget(lblDefault, 0, Qt::AlignHCenter);
    vblSettings->addWidget(chbIsHttpOnly);
    vblSettings->addWidget(chbIsSecure);
    vblSettings->addWidget(chbIsSession);
    vblSettings->addWidget(chbExpandAll);
    vblSettings->addWidget(lblExpire);
    vblSettings->addWidget(dteExpire, 0, Qt::AlignLeft);
    vblSettings->addWidget(pbOk, 0, Qt::AlignHCenter);

    dialSettings.setFixedSize(dialSettings.minimumSizeHint());

    //connects
    connect(pbOk, SIGNAL(clicked()), &dialSettings, SLOT(accept()));

    if (dialSettings.exec() == QDialog::Accepted)
    {
        bDefIsHttpOnly = chbIsHttpOnly->isChecked();
        bDefIsSecure = chbIsSecure->isChecked();
        bDefIsSession = chbIsSession->isChecked();
        bDefExpandAll = chbExpandAll->isChecked();
        dtDefExpire = dteExpire->dateTime();

        QSettings stg(qApp->applicationDirPath() + '/' + qAppName() + ".ini", QSettings::IniFormat);
        stg.beginGroup("Settings");
        stg.setValue("IsHttpOnly", bDefIsHttpOnly ? "1" : "0");
        stg.setValue("IsSecure", bDefIsSecure ? "1" : "0");
        stg.setValue("IsSession", bDefIsSession ? "1" : "0");
        stg.setValue("ExpandAll", bDefExpandAll ? "1" : "0");
        stg.setValue("Expire", dtDefExpire.toMSecsSinceEpoch()/1000LL);
        stg.endGroup();
    }
}

//-------------------------------------------------------------------------------------------------
const char* CookiesManager::fGetIconString(const int iColumn) const
{
    switch (iColumn)
    {
    case eHeaderIsHttpOnly: return ":/img/http.png";
    case eHeaderIsSecure:   return ":/img/secure.png";
    case eHeaderIsSession:  return ":/img/session.png";
    default:                return 0;
    }
}

//-------------------------------------------------------------------------------------------------
void CookiesManager::slotEdit(const QModelIndex &modInd)
{
    if (modInd.column() == eHeaderExpire)
    {
        QStandardItem *sitem = sitemModel->itemFromIndex(modInd);
        if (modInd.data(Qt::UserRole+1).toInt() != eToggleDisable)
        {
            QDateTime dt(QDateTime::fromString(modInd.data().toString(), "yyyy/MM/dd hh:mm:ss"));

            QDialog dialogExpire(this, Qt::WindowCloseButtonHint);

            QRadioButton *rbDateTime = new QRadioButton(&dialogExpire);
            rbDateTime->setFocusPolicy(Qt::NoFocus);
            DateTimeEditEx *dtEditExpire = new DateTimeEditEx(rbDateTime, &dialogExpire);
            dtEditExpire->setDateTime(dt);
            QHBoxLayout *hblDateTime = new QHBoxLayout;
            hblDateTime->addWidget(rbDateTime);
            hblDateTime->addWidget(dtEditExpire);

            QRadioButton *rbUnixTime = new QRadioButton(&dialogExpire);
            rbUnixTime->setFocusPolicy(Qt::NoFocus);
            SpinBoxEx *sbEditExpire = new SpinBoxEx(dt.toMSecsSinceEpoch()/1000LL, rbUnixTime, &dialogExpire);
            QHBoxLayout *hblUnixTime = new QHBoxLayout;
            hblUnixTime->addWidget(rbUnixTime);
            hblUnixTime->addWidget(sbEditExpire, 1);

            QPushButton *pbExpire = new QPushButton("OK", this);

            QVBoxLayout *vblExpire = new QVBoxLayout(&dialogExpire);
            vblExpire->addLayout(hblDateTime);
            vblExpire->addLayout(hblUnixTime);
            vblExpire->addWidget(pbExpire, 0, Qt::AlignHCenter);

            dialogExpire.setFixedSize(dialogExpire.minimumSizeHint());

            //connects
            connect(pbExpire, SIGNAL(clicked()), &dialogExpire, SLOT(accept()));
            connect(rbDateTime, SIGNAL(clicked()), dtEditExpire, SLOT(setFocus()));
            connect(rbUnixTime, SIGNAL(clicked()), sbEditExpire, SLOT(setFocus()));

            if (dialogExpire.exec() == QDialog::Accepted)
                sitem->setText(
                            (rbDateTime->isChecked() ?
                                 dtEditExpire->dateTime() :
                                 QDateTime::fromMSecsSinceEpoch(sbEditExpire->value()*1000LL))
                            .toString("yyyy/MM/dd hh:mm:ss"));
        }
    }
    else if (fGetIconString(modInd.column()))
    {
        QStandardItem *sitem = sitemModel->itemFromIndex(modInd);
        const int iToggle = modInd.data(Qt::UserRole+1).toInt();
        if (iToggle == eToggleOff)
        {
            sitem->setIcon(QIcon(fGetIconString(modInd.column())));
            sitem->setData(eToggleOn);
            Q_ASSERT(modInd.parent().isValid());
            if (modInd.column() == eHeaderIsSession)
                sitem->parent()->child(sitem->row(), eHeaderExpire)->setEnabled(false);
        }
        else if (iToggle == eToggleOn)
        {
            sitem->setIcon(QIcon());
            sitem->setData(eToggleOff);
            Q_ASSERT(modInd.parent().isValid());
            if (modInd.column() == eHeaderIsSession)
                sitem->parent()->child(sitem->row(), eHeaderExpire)->setEnabled(true);
        }
    }
}
