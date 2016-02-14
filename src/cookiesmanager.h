#ifndef COOKIESMANAGER_H
#define COOKIESMANAGER_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QTreeView>
#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtGui/QStandardItemModel>
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>

#ifdef __MINGW32__
#pragma GCC diagnostic warning "-Wpedantic"
#endif

class DateTimeEditEx : public QDateTimeEdit
{
    //Q_OBJECT not required
public:
    DateTimeEditEx(QRadioButton *rb, QWidget *parent);

private:
    explicit DateTimeEditEx(const DateTimeEditEx &);
    void operator=(const DateTimeEditEx &);
    virtual void focusInEvent(QFocusEvent *event);
    QRadioButton *const rbFocus;
};

class SpinBoxEx : public QAbstractSpinBox
{
    Q_OBJECT
public:
    explicit SpinBoxEx(const qint64 iValueTime, QRadioButton *rb, QWidget *parent);
    inline qint64 value() const
    {
        return iValue;
    }

private:
    explicit SpinBoxEx(const SpinBoxEx &);
    void operator=(const SpinBoxEx &);
    virtual StepEnabled stepEnabled() const;
    virtual void stepBy(int steps);
    virtual QValidator::State validate(QString &input, int &) const;
    virtual void focusInEvent(QFocusEvent *event);
    QRadioButton *const rbFocus;
    qint64 iValue;

private slots:
    void slotEdited(const QString &strText);
};

class CookiesManager;

class TreeViewEx : public QTreeView
{
    //Q_OBJECT not required
public:
    explicit TreeViewEx(CookiesManager *parent);

private:
    class StyledItemDelegateEx : public QStyledItemDelegate
    {
        //Q_OBJECT not required
    public:
        explicit StyledItemDelegateEx(QObject *parent);

    private:
        explicit StyledItemDelegateEx(const StyledItemDelegateEx &);
        void operator=(const StyledItemDelegateEx &);
        virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        const QPen penLight,
        penDark;
    };

    explicit TreeViewEx(const TreeViewEx &);
    void operator=(const TreeViewEx &);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void dropEvent(QDropEvent *event);
    CookiesManager *const pApplication;
};

class CookiesManager : public QWidget
{
    Q_OBJECT
public:
    CookiesManager();
    ~CookiesManager();
    void fOpenCfg(const QString &strPath);

public slots:
    void slotNewCfg();

private:
    enum
    {
        eHost,
        eIsHttpOnly,
        ePath,
        eIsSecure,
        //eIsSession,
        eExpire,
        eName,
        eValue,
        eRecordSize
    };
    enum
    {
        eHeaderHost,
        eHeaderIsHttpOnly,
        eHeaderPath,
        eHeaderIsSecure,
        eHeaderIsSession,
        eHeaderExpire,
        eHeaderName,
        eHeaderValue,
        eHeaders
    };
    enum
    {
        eToggleDisable = -1,
        eToggleOff,
        eToggleOn
    };

    QPushButton *pbSaveCfg,
    *pbSaveAsCfg;
    QLabel *lblInfo;
    QPushButton *pbNewRecord,
    *pbMoveDown,
    *pbMoveUp,
    *pbDelete,
    *pbCollapse,
    *pbExpand;
    TreeViewEx *trViewEx;
    QStandardItemModel *const sitemModel;
    QItemSelectionModel *itemSelModel;
    QString strCurrentCfg;
    bool bDefIsHttpOnly,
    bDefIsSecure,
    bDefIsSession,
    bDefExpandAll;
    QDateTime dtDefExpire;
    void fButtonsToggle(const bool bToggle) const;
    bool fHasErrors(const QString &str) const;
    void fSaveCfg(const QString &strPath);
    const char* fGetIconString(const int iColumn) const;

private slots:
    void slotOpenCfg();
    void slotSaveCfg();
    void slotSaveAsCfg();
    void slotNewGroup();
    void slotNewRecord() const;
    void slotMoveDown() const;
    void slotMoveUp() const;
    void slotDelete() const;
    inline void slotCollapseAll() const
    {
        trViewEx->collapseAll();
    }
    inline void slotExpandAll() const
    {
        trViewEx->expandAll();
    }
    void slotShowSettings();
    void slotEdit(const QModelIndex &modInd);
};

#endif // COOKIESMANAGER_H
